#pragma once

#include "Mcp/SUnrealMcpProtocol.h"

class FSUnrealMcpTaskRegistry;
class FSUnrealMcpCommandRegistry;

struct FSUnrealMcpExecutionContext
{
    TSharedRef<FSUnrealMcpTaskRegistry> TaskRegistry;
    TSharedRef<FSUnrealMcpCommandRegistry> CommandRegistry;
};

class SUNREALMCP_API ISUnrealMcpCommand
{
public:
    virtual ~ISUnrealMcpCommand() = default;

    virtual FString GetCommandName() const = 0;
    virtual FSUnrealMcpResponse Execute(const FSUnrealMcpRequest& Request, const FSUnrealMcpExecutionContext& Context) = 0;
};
