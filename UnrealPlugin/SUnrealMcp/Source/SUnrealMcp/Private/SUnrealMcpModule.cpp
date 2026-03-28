#include "SUnrealMcpModule.h"

#include "Mcp/SUnrealMcpCommandRegistry.h"
#include "Mcp/SUnrealMcpServer.h"
#include "Mcp/SUnrealMcpTaskRegistry.h"
#include "Misc/MessageDialog.h"
#include "SUnrealMcpSettings.h"

#include "Logging/LogMacros.h"
#include "Modules/ModuleManager.h"

IMPLEMENT_MODULE(FSUnrealMcpModule, SUnrealMcp)
DEFINE_LOG_CATEGORY(LogSUnrealMcp);

FSUnrealMcpModule::~FSUnrealMcpModule() = default;

void FSUnrealMcpModule::StartupModule()
{
    CommandRegistry = MakeShared<FSUnrealMcpCommandRegistry>();
    if (!FSUnrealMcpCommandAutoRegistrar::RegisterAll(CommandRegistry.ToSharedRef()))
    {
        UE_LOG(LogSUnrealMcp, Error, TEXT("Command registration failed; SUnrealMcp server will not start."));
        FMessageDialog::Open(
            EAppMsgType::Ok,
            FText::FromString(CommandRegistry->GetRegistrationErrorSummary()));
        CommandRegistry.Reset();
        return;
    }

    const USUnrealMcpSettings* Settings = GetDefault<USUnrealMcpSettings>();
    TaskRegistry = MakeShared<FSUnrealMcpTaskRegistry>();
    TaskRegistry->SetCompletedTaskRetentionSeconds(Settings->CompletedTaskRetentionSeconds);
    Server = MakeUnique<FSUnrealMcpServer>();

    FSUnrealMcpServerConfig ServerConfig;
    ServerConfig.BindAddress = Settings->BindAddress;
    ServerConfig.Port = Settings->Port;
    ServerConfig.ListenBacklog = Settings->ListenBacklog;
    ServerConfig.MaxPendingSendBytesPerConnection = 256 * 1024;

    if (!Settings->bAutoStartServer)
    {
        UE_LOG(LogSUnrealMcp, Log, TEXT("Auto-start disabled in settings; server will not start."));
        return;
    }

    if (!Server->Start(ServerConfig, CommandRegistry.ToSharedRef(), TaskRegistry.ToSharedRef()))
    {
        UE_LOG(LogSUnrealMcp, Error, TEXT("Failed to start TCP server on %s:%d"), *ServerConfig.BindAddress, ServerConfig.Port);
        FMessageDialog::Open(
            EAppMsgType::Ok,
            FText::FromString(FString::Printf(
                TEXT("SUnrealMcp failed to start TCP server on %s:%d. Check whether the bind address is valid and the port is already in use."),
                *ServerConfig.BindAddress,
                ServerConfig.Port)));
        return;
    }

    UE_LOG(LogSUnrealMcp, Log, TEXT("Listening on %s:%d"), *ServerConfig.BindAddress, ServerConfig.Port);
}

void FSUnrealMcpModule::ShutdownModule()
{
    if (Server.IsValid())
    {
        Server->Stop();
        Server.Reset();
    }

    CommandRegistry.Reset();
    TaskRegistry.Reset();
}
