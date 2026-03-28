#pragma once

#include "Containers/Map.h"
#include "Templates/SharedPointer.h"

#include "Mcp/ISUnrealMcpTask.h"

class SUNREALMCP_API FSUnrealMcpTaskRegistry
{
public:
    FString EnqueueTask(const TSharedRef<ISUnrealMcpTask>& Task);
    void Tick(float DeltaTime);
    void SetCompletedTaskRetentionSeconds(double InCompletedTaskRetentionSeconds);

    FSUnrealMcpResponse MakeAcceptedResponse(const FString& RequestId, const FString& TaskId) const;
    FSUnrealMcpResponse MakeStatusResponse(const FString& RequestId, const FString& TaskId) const;
    FSUnrealMcpResponse CancelTask(const FString& RequestId, const FString& TaskId);

private:
    struct FRegisteredTask
    {
        FString TaskId;
        TSharedRef<ISUnrealMcpTask> Task;
        double CompletedAtSeconds = 0.0;
        bool bCompletionRecorded = false;
    };

    bool IsExpiredTask(const FString& TaskId) const;
    FSUnrealMcpResponse MakeMissingTaskResponse(const FString& RequestId, const FString& TaskId) const;
    const FRegisteredTask* FindTask(const FString& TaskId) const;
    FRegisteredTask* FindTask(const FString& TaskId);
    TSharedPtr<FJsonObject> BuildTaskData(const FRegisteredTask& RegisteredTask) const;
    void PruneCompletedTasks(double NowSeconds);
    static FString LexToString(ESUnrealMcpTaskState TaskState);

    TMap<FString, FRegisteredTask> Tasks;
    TMap<FString, double> ExpiredTaskIds;
    uint64 NextTaskId = 1;
    double CompletedTaskRetentionSeconds = 300.0;
};
