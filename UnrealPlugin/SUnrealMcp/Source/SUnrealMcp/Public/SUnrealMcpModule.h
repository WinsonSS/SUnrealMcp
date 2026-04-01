#pragma once

#include "Modules/ModuleInterface.h"
#include "Mcp/SUnrealMcpServer.h"

class FSUnrealMcpCommandRegistry;
class FSUnrealMcpTaskRegistry;

DECLARE_LOG_CATEGORY_EXTERN(LogSUnrealMcp, Log, All);

class SUNREALMCP_API FSUnrealMcpModule final : public IModuleInterface
{
public:
    virtual ~FSUnrealMcpModule() override;
    virtual void StartupModule() override;
    virtual void ShutdownModule() override;

    bool RebuildCommandRegistry(FString* OutError = nullptr);

private:
    TSharedPtr<FSUnrealMcpCommandRegistry> CommandRegistry;
    TSharedPtr<FSUnrealMcpTaskRegistry> TaskRegistry;
    TUniquePtr<FSUnrealMcpServer> Server;
};
