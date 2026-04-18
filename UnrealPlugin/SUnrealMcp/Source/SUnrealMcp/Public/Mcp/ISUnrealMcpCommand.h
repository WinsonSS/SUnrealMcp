#pragma once

#include "Templates/Function.h"
#include "Templates/SharedPointer.h"

#include "Mcp/SUnrealMcpProtocol.h"

class ISUnrealMcpTask;
class FSUnrealMcpTaskRegistry;
class FSUnrealMcpCommandRegistry;

class FSUnrealMcpExecutionContext
{
public:
    FSUnrealMcpExecutionContext(
        const TSharedRef<FSUnrealMcpTaskRegistry>& InTaskRegistry,
        const TSharedRef<FSUnrealMcpCommandRegistry>& InCommandRegistry,
        TFunction<bool(FString*)> InReloadCommandRegistry);

    bool HasCommand(const FString& Name) const;
    TArray<FString> ListCommandNames() const;
    FSUnrealMcpResponse GetTaskStatusResponse(const FString& RequestId, const FString& TaskId) const;
    FSUnrealMcpResponse CancelTask(const FString& RequestId, const FString& TaskId) const;
    FString EnqueueTask(const TSharedRef<ISUnrealMcpTask>& Task) const;
    FSUnrealMcpResponse MakeAcceptedTaskResponse(const FString& RequestId, const FString& TaskId) const;
    bool ReloadCommandRegistry(FString* OutError = nullptr) const;

private:
    TSharedRef<FSUnrealMcpTaskRegistry> TaskRegistry;
    TSharedRef<FSUnrealMcpCommandRegistry> CommandRegistry;
    TFunction<bool(FString*)> ReloadCommandRegistryCallback;
};

class SUNREALMCP_API ISUnrealMcpCommand
{
public:
    virtual ~ISUnrealMcpCommand() = default;

    virtual FString GetCommandName() const = 0;
    virtual FSUnrealMcpResponse Execute(const FSUnrealMcpRequest& Request, const FSUnrealMcpExecutionContext& Context) = 0;
};
