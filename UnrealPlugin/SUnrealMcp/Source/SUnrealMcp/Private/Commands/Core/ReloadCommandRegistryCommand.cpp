#include "Mcp/ISUnrealMcpCommand.h"
#include "Mcp/SUnrealMcpCommandRegistry.h"

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
            FString Error;
            if (!Context.ReloadCommandRegistry(&Error))
            {
                return FSUnrealMcpResponse::MakeError(
                    Request.RequestId,
                    Error == TEXT("SUnrealMcp module is not loaded.") ? TEXT("MODULE_NOT_LOADED") : TEXT("RELOAD_FAILED"),
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
