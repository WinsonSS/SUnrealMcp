#include "SUnrealMcpModule.h"

#include "Mcp/SUnrealMcpCommandRegistry.h"
#include "Mcp/SUnrealMcpServer.h"
#include "Mcp/SUnrealMcpTaskRegistry.h"
#include "HAL/IConsoleManager.h"
#include "Misc/MessageDialog.h"
#include "SUnrealMcpSettings.h"

#include "Logging/LogMacros.h"
#include "Modules/ModuleManager.h"

IMPLEMENT_MODULE(FSUnrealMcpModule, SUnrealMcp)
DEFINE_LOG_CATEGORY(LogSUnrealMcp);

namespace
{
    static FAutoConsoleCommand GReloadSUnrealMcpCommandRegistry(
        TEXT("SUnrealMcp.ReloadCommandRegistry"),
        TEXT("Rebuilds the active SUnrealMcp command registry after Live Coding or other runtime code changes."),
        FConsoleCommandDelegate::CreateLambda([]()
        {
            FSUnrealMcpModule* Module = FModuleManager::GetModulePtr<FSUnrealMcpModule>(TEXT("SUnrealMcp"));
            if (Module == nullptr)
            {
                UE_LOG(LogSUnrealMcp, Warning, TEXT("SUnrealMcp module is not loaded; cannot rebuild command registry."));
                return;
            }

            FString Error;
            if (!Module->RebuildCommandRegistry(&Error))
            {
                UE_LOG(LogSUnrealMcp, Error, TEXT("Failed to rebuild SUnrealMcp command registry: %s"), *Error);
                return;
            }

            UE_LOG(LogSUnrealMcp, Log, TEXT("SUnrealMcp command registry rebuilt successfully."));
        }));
}

FSUnrealMcpModule::~FSUnrealMcpModule() = default;

void FSUnrealMcpModule::StartupModule()
{
    FString RegistrationError;
    if (!RebuildCommandRegistry(&RegistrationError))
    {
        UE_LOG(LogSUnrealMcp, Error, TEXT("Command registration failed; SUnrealMcp server will not start."));
        FMessageDialog::Open(
            EAppMsgType::Ok,
            FText::FromString(RegistrationError));
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
    ServerConfig.MaxPendingSendBytesPerConnection = Settings->MaxPendingSendBytesPerConnection;

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

bool FSUnrealMcpModule::RebuildCommandRegistry(FString* OutError)
{
    TSharedRef<FSUnrealMcpCommandRegistry> NewRegistry = MakeShared<FSUnrealMcpCommandRegistry>();
    if (!FSUnrealMcpCommandAutoRegistrar::RegisterAll(NewRegistry))
    {
        const FString Summary = NewRegistry->GetRegistrationErrorSummary();
        if (OutError != nullptr)
        {
            *OutError = Summary;
        }
        return false;
    }

    CommandRegistry = NewRegistry;
    if (Server.IsValid())
    {
        Server->SetCommandRegistry(NewRegistry);
    }

    if (OutError != nullptr)
    {
        OutError->Reset();
    }

    return true;
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
