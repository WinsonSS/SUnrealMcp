#include "Mcp/SUnrealMcpTaskRegistry.h"

#include "HAL/PlatformTime.h"
#include "Math/UnrealMathUtility.h"

FString FSUnrealMcpTaskRegistry::EnqueueTask(const TSharedRef<ISUnrealMcpTask>& Task)
{
    const FString TaskId = FString::Printf(TEXT("task-%llu"), NextTaskId++);
    Tasks.Add(TaskId, FRegisteredTask{TaskId, Task});
    Task->Start();
    return TaskId;
}

void FSUnrealMcpTaskRegistry::SetCompletedTaskRetentionSeconds(double InCompletedTaskRetentionSeconds)
{
    CompletedTaskRetentionSeconds = FMath::Max(1.0, InCompletedTaskRetentionSeconds);
}

void FSUnrealMcpTaskRegistry::Tick(float DeltaTime)
{
    const double NowSeconds = FPlatformTime::Seconds();

    for (TPair<FString, FRegisteredTask>& Pair : Tasks)
    {
        FRegisteredTask& RegisteredTask = Pair.Value;
        const ESUnrealMcpTaskState State = RegisteredTask.Task->GetState();
        if (State == ESUnrealMcpTaskState::Pending || State == ESUnrealMcpTaskState::Running)
        {
            RegisteredTask.Task->Tick(DeltaTime);
        }

        const ESUnrealMcpTaskState UpdatedState = RegisteredTask.Task->GetState();
        const bool bCompleted =
            UpdatedState == ESUnrealMcpTaskState::Succeeded ||
            UpdatedState == ESUnrealMcpTaskState::Failed ||
            UpdatedState == ESUnrealMcpTaskState::Cancelled;

        if (bCompleted && !RegisteredTask.bCompletionRecorded)
        {
            RegisteredTask.CompletedAtSeconds = NowSeconds;
            RegisteredTask.bCompletionRecorded = true;
        }
    }

    PruneCompletedTasks(NowSeconds);
}

FSUnrealMcpResponse FSUnrealMcpTaskRegistry::MakeAcceptedResponse(const FString& RequestId, const FString& TaskId) const
{
    const FRegisteredTask* RegisteredTask = FindTask(TaskId);
    if (RegisteredTask == nullptr)
    {
        return MakeMissingTaskResponse(RequestId, TaskId);
    }

    TSharedPtr<FJsonObject> Data = BuildTaskData(*RegisteredTask);
    Data->SetBoolField(TEXT("accepted"), true);
    return FSUnrealMcpResponse::MakeSuccess(RequestId, Data);
}

FSUnrealMcpResponse FSUnrealMcpTaskRegistry::MakeStatusResponse(const FString& RequestId, const FString& TaskId) const
{
    const FRegisteredTask* RegisteredTask = FindTask(TaskId);
    if (RegisteredTask == nullptr)
    {
        return MakeMissingTaskResponse(RequestId, TaskId);
    }

    return FSUnrealMcpResponse::MakeSuccess(RequestId, BuildTaskData(*RegisteredTask));
}

FSUnrealMcpResponse FSUnrealMcpTaskRegistry::CancelTask(const FString& RequestId, const FString& TaskId)
{
    FRegisteredTask* RegisteredTask = FindTask(TaskId);
    if (RegisteredTask == nullptr)
    {
        return MakeMissingTaskResponse(RequestId, TaskId);
    }

    const ESUnrealMcpTaskState State = RegisteredTask->Task->GetState();
    if (State == ESUnrealMcpTaskState::Pending || State == ESUnrealMcpTaskState::Running)
    {
        RegisteredTask->Task->Cancel();
    }

    TSharedPtr<FJsonObject> Data = BuildTaskData(*RegisteredTask);
    Data->SetBoolField(TEXT("cancelRequested"), true);
    return FSUnrealMcpResponse::MakeSuccess(RequestId, Data);
}

const FSUnrealMcpTaskRegistry::FRegisteredTask* FSUnrealMcpTaskRegistry::FindTask(const FString& TaskId) const
{
    return Tasks.Find(TaskId);
}

bool FSUnrealMcpTaskRegistry::IsExpiredTask(const FString& TaskId) const
{
    return ExpiredTaskIds.Contains(TaskId);
}

FSUnrealMcpResponse FSUnrealMcpTaskRegistry::MakeMissingTaskResponse(const FString& RequestId, const FString& TaskId) const
{
    if (IsExpiredTask(TaskId))
    {
        return FSUnrealMcpResponse::MakeError(
            RequestId,
            TEXT("TASK_EXPIRED"),
            FString::Printf(
                TEXT("Task '%s' was completed earlier and has already been pruned from the registry."),
                *TaskId));
    }

    return FSUnrealMcpResponse::MakeError(
        RequestId,
        TEXT("TASK_NOT_FOUND"),
        FString::Printf(TEXT("No task is registered for '%s'."), *TaskId));
}

FSUnrealMcpTaskRegistry::FRegisteredTask* FSUnrealMcpTaskRegistry::FindTask(const FString& TaskId)
{
    return Tasks.Find(TaskId);
}

TSharedPtr<FJsonObject> FSUnrealMcpTaskRegistry::BuildTaskData(const FRegisteredTask& RegisteredTask) const
{
    const TSharedRef<FJsonObject> Data = MakeShared<FJsonObject>();
    const ESUnrealMcpTaskState State = RegisteredTask.Task->GetState();

    Data->SetStringField(TEXT("taskId"), RegisteredTask.TaskId);
    Data->SetStringField(TEXT("taskName"), RegisteredTask.Task->GetTaskName());
    Data->SetStringField(TEXT("status"), LexToString(State));
    Data->SetBoolField(
        TEXT("completed"),
        State == ESUnrealMcpTaskState::Succeeded ||
        State == ESUnrealMcpTaskState::Failed ||
        State == ESUnrealMcpTaskState::Cancelled);

    if (TSharedPtr<FJsonObject> Payload = RegisteredTask.Task->BuildPayload(); Payload.IsValid())
    {
        Data->SetObjectField(TEXT("payload"), Payload);
    }

    if (TOptional<FSUnrealMcpError> Error = RegisteredTask.Task->GetError(); Error.IsSet())
    {
        const TSharedRef<FJsonObject> ErrorObject = MakeShared<FJsonObject>();
        ErrorObject->SetStringField(TEXT("code"), Error->Code);
        ErrorObject->SetStringField(TEXT("message"), Error->Message);
        if (Error->Details.IsValid())
        {
            ErrorObject->SetObjectField(TEXT("details"), Error->Details);
        }

        Data->SetObjectField(TEXT("error"), ErrorObject);
    }

    return Data;
}

void FSUnrealMcpTaskRegistry::PruneCompletedTasks(double NowSeconds)
{
    TArray<FString> TaskIdsToRemove;

    for (const TPair<FString, FRegisteredTask>& Pair : Tasks)
    {
        const FRegisteredTask& RegisteredTask = Pair.Value;
        if (!RegisteredTask.bCompletionRecorded)
        {
            continue;
        }

        if ((NowSeconds - RegisteredTask.CompletedAtSeconds) >= CompletedTaskRetentionSeconds)
        {
            TaskIdsToRemove.Add(Pair.Key);
        }
    }

    for (const FString& TaskId : TaskIdsToRemove)
    {
        ExpiredTaskIds.Add(TaskId, NowSeconds);
        Tasks.Remove(TaskId);
    }

    TArray<FString> ExpiredTaskIdsToRemove;
    for (const TPair<FString, double>& Pair : ExpiredTaskIds)
    {
        if ((NowSeconds - Pair.Value) >= CompletedTaskRetentionSeconds)
        {
            ExpiredTaskIdsToRemove.Add(Pair.Key);
        }
    }

    for (const FString& ExpiredTaskId : ExpiredTaskIdsToRemove)
    {
        ExpiredTaskIds.Remove(ExpiredTaskId);
    }
}

FString FSUnrealMcpTaskRegistry::LexToString(ESUnrealMcpTaskState TaskState)
{
    switch (TaskState)
    {
    case ESUnrealMcpTaskState::Pending:
        return TEXT("pending");
    case ESUnrealMcpTaskState::Running:
        return TEXT("running");
    case ESUnrealMcpTaskState::Succeeded:
        return TEXT("succeeded");
    case ESUnrealMcpTaskState::Failed:
        return TEXT("failed");
    case ESUnrealMcpTaskState::Cancelled:
        return TEXT("cancelled");
    default:
        return TEXT("unknown");
    }
}
