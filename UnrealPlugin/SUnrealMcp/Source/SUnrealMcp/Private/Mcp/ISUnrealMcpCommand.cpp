#include "Mcp/ISUnrealMcpCommand.h"

#include "Mcp/SUnrealMcpCommandRegistry.h"
#include "Mcp/SUnrealMcpTaskRegistry.h"

FSUnrealMcpExecutionContext::FSUnrealMcpExecutionContext(
    const TSharedRef<FSUnrealMcpTaskRegistry>& InTaskRegistry,
    const TSharedRef<FSUnrealMcpCommandRegistry>& InCommandRegistry,
    TFunction<bool(FString*)> InReloadCommandRegistry)
    : TaskRegistry(InTaskRegistry)
    , CommandRegistry(InCommandRegistry)
    , ReloadCommandRegistryCallback(MoveTemp(InReloadCommandRegistry))
{
}

bool FSUnrealMcpExecutionContext::HasCommand(const FString& Name) const
{
    return CommandRegistry->HasCommand(Name);
}

TArray<FString> FSUnrealMcpExecutionContext::ListCommandNames() const
{
    return CommandRegistry->ListCommandNames();
}

FSUnrealMcpResponse FSUnrealMcpExecutionContext::GetTaskStatusResponse(const FString& RequestId, const FString& TaskId) const
{
    return TaskRegistry->MakeStatusResponse(RequestId, TaskId);
}

FSUnrealMcpResponse FSUnrealMcpExecutionContext::CancelTask(const FString& RequestId, const FString& TaskId) const
{
    return TaskRegistry->CancelTask(RequestId, TaskId);
}

FString FSUnrealMcpExecutionContext::EnqueueTask(const TSharedRef<ISUnrealMcpTask>& Task) const
{
    return TaskRegistry->EnqueueTask(Task);
}

FSUnrealMcpResponse FSUnrealMcpExecutionContext::MakeAcceptedTaskResponse(const FString& RequestId, const FString& TaskId) const
{
    return TaskRegistry->MakeAcceptedResponse(RequestId, TaskId);
}

bool FSUnrealMcpExecutionContext::ReloadCommandRegistry(FString* OutError) const
{
    if (!ReloadCommandRegistryCallback)
    {
        if (OutError != nullptr)
        {
            *OutError = TEXT("Reload command registry callback is not available.");
        }
        return false;
    }

    return ReloadCommandRegistryCallback(OutError);
}
