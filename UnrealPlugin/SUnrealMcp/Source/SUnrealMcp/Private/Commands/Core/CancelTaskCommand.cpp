#include "Mcp/ISUnrealMcpCommand.h"
#include "Mcp/SUnrealMcpCommandRegistry.h"

namespace
{
    class FCancelTaskCommand final : public ISUnrealMcpCommand
    {
    public:
        virtual FString GetCommandName() const override
        {
            return TEXT("cancel_task");
        }

        virtual FSUnrealMcpResponse Execute(const FSUnrealMcpRequest& Request, const FSUnrealMcpExecutionContext& Context) override
        {
            if (!Request.Params.IsValid())
            {
                return FSUnrealMcpResponse::MakeError(
                    Request.RequestId,
                    TEXT("INVALID_PARAMS"),
                    TEXT("cancel_task requires a params object."));
            }

            FString TaskId;
            if (!Request.Params->TryGetStringField(TEXT("task_id"), TaskId) || TaskId.IsEmpty())
            {
                return FSUnrealMcpResponse::MakeError(
                    Request.RequestId,
                    TEXT("INVALID_PARAMS"),
                    TEXT("Missing task_id."));
            }

            return Context.CancelTask(Request.RequestId, TaskId);
        }
    };

    const FSUnrealMcpCommandAutoRegistrar CancelTaskCommandRegistrar([]()
    {
        return MakeShared<FCancelTaskCommand>();
    });
}
