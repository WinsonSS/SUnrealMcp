#pragma once

#include "Mcp/SUnrealMcpProtocol.h"

class FSUnrealMcpTaskRegistry;

struct FSUnrealMcpExecutionContext
{
    TSharedRef<FSUnrealMcpTaskRegistry> TaskRegistry;
};

class SUNREALMCP_API ISUnrealMcpCommand
{
public:
    virtual ~ISUnrealMcpCommand() = default;

    virtual FString GetCommandName() const = 0;
    virtual FSUnrealMcpResponse Execute(const FSUnrealMcpRequest& Request, const FSUnrealMcpExecutionContext& Context) = 0;
};
