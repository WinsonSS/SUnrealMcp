#include "Mcp/ISUnrealMcpCommand.h"
#include "Mcp/SUnrealMcpCommandRegistry.h"

namespace
{
    class FGetTaskStatusCommand final : public ISUnrealMcpCommand
    {
    public:
        virtual FString GetCommandName() const override
        {
            return TEXT("get_task_status");
        }

        virtual FSUnrealMcpResponse Execute(const FSUnrealMcpRequest& Request, const FSUnrealMcpExecutionContext& Context) override
        {
            if (!Request.Params.IsValid())
            {
                return FSUnrealMcpResponse::MakeError(
                    Request.RequestId,
                    TEXT("INVALID_PARAMS"),
                    TEXT("get_task_status requires a params object."));
            }

            FString TaskId;
            if (!Request.Params->TryGetStringField(TEXT("task_id"), TaskId) || TaskId.IsEmpty())
            {
                return FSUnrealMcpResponse::MakeError(
                    Request.RequestId,
                    TEXT("INVALID_PARAMS"),
                    TEXT("Missing task_id."));
            }

            return Context.GetTaskStatusResponse(Request.RequestId, TaskId);
        }
    };

    const FSUnrealMcpCommandAutoRegistrar GetTaskStatusCommandRegistrar([]()
    {
        return MakeShared<FGetTaskStatusCommand>();
    });
}
