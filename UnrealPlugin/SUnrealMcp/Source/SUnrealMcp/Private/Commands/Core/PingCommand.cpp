#include "Mcp/ISUnrealMcpCommand.h"
#include "Mcp/SUnrealMcpCommandRegistry.h"
#include "Mcp/SUnrealMcpProtocol.h"

namespace
{
    class FPingCommand final : public ISUnrealMcpCommand
    {
    public:
        virtual FString GetCommandName() const override
        {
            return TEXT("ping");
        }

        virtual FSUnrealMcpResponse Execute(const FSUnrealMcpRequest& Request, const FSUnrealMcpExecutionContext& Context) override
        {
            static_cast<void>(Context);

            TSharedPtr<FJsonObject> Data = MakeShared<FJsonObject>();
            Data->SetStringField(TEXT("server"), TEXT("SUnrealMcp"));
            Data->SetStringField(TEXT("plugin"), TEXT("SUnrealMcp"));
            Data->SetStringField(TEXT("status"), TEXT("ready"));
            Data->SetNumberField(TEXT("protocolVersion"), FSUnrealMcpRequest::ProtocolVersion);
            return FSUnrealMcpResponse::MakeSuccess(Request.RequestId, Data);
        }
    };

    const FSUnrealMcpCommandAutoRegistrar PingCommandRegistrar([]()
    {
        return MakeShared<FPingCommand>();
    });
}
