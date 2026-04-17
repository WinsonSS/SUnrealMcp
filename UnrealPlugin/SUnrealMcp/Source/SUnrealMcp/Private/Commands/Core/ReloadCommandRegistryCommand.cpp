#include "Mcp/ISUnrealMcpCommand.h"
#include "Mcp/SUnrealMcpCommandRegistry.h"
#include "Modules/ModuleManager.h"
#include "SUnrealMcpModule.h"

namespace
{
    class FReloadCommandRegistryCommand final : public ISUnrealMcpCommand
    {
    public:
        virtual FString GetCommandName() const override
        {
            return TEXT("reload_command_registry");
        }

        virtual FSUnrealMcpResponse Execute(const FSUnrealMcpRequest& Request, const FSUnrealMcpExecutionContext& Context) override
        {
            static_cast<void>(Context);

            FSUnrealMcpModule* Module = FModuleManager::GetModulePtr<FSUnrealMcpModule>(TEXT("SUnrealMcp"));
            if (Module == nullptr)
            {
                return FSUnrealMcpResponse::MakeError(
                    Request.RequestId,
                    TEXT("MODULE_NOT_LOADED"),
                    TEXT("SUnrealMcp module is not loaded."));
            }

            FString Error;
            if (!Module->RebuildCommandRegistry(&Error))
            {
                return FSUnrealMcpResponse::MakeError(
                    Request.RequestId,
                    TEXT("RELOAD_FAILED"),
                    Error.IsEmpty() ? TEXT("Failed to rebuild command registry.") : Error);
            }

            return FSUnrealMcpResponse::MakeSuccess(Request.RequestId, MakeShared<FJsonObject>());
        }
    };

    const FSUnrealMcpCommandAutoRegistrar ReloadCommandRegistryCommandRegistrar([]()
    {
        return MakeShared<FReloadCommandRegistryCommand>();
    });
}
