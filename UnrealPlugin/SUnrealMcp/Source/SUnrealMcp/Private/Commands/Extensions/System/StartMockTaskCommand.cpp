#include "Mcp/ISUnrealMcpCommand.h"
#include "Mcp/ISUnrealMcpTask.h"
#include "Mcp/SUnrealMcpCommandRegistry.h"
#include "Mcp/SUnrealMcpTaskRegistry.h"

namespace
{
    class FMockDelayTask final : public ISUnrealMcpTask
    {
    public:
        explicit FMockDelayTask(int32 InRemainingTicks)
            : RemainingTicks(FMath::Max(1, InRemainingTicks))
        {
        }

        virtual FString GetTaskName() const override
        {
            return TEXT("mock_delay_task");
        }

        virtual void Start() override
        {
            State = ESUnrealMcpTaskState::Running;
        }

        virtual void Tick(float DeltaTime) override
        {
            static_cast<void>(DeltaTime);

            if (State != ESUnrealMcpTaskState::Running)
            {
                return;
            }

            --RemainingTicks;
            if (RemainingTicks <= 0)
            {
                State = ESUnrealMcpTaskState::Succeeded;
            }
        }

        virtual void Cancel() override
        {
            if (State == ESUnrealMcpTaskState::Pending || State == ESUnrealMcpTaskState::Running)
            {
                State = ESUnrealMcpTaskState::Cancelled;
            }
        }

        virtual ESUnrealMcpTaskState GetState() const override
        {
            return State;
        }

        virtual TSharedPtr<FJsonObject> BuildPayload() const override
        {
            const TSharedRef<FJsonObject> Payload = MakeShared<FJsonObject>();
            Payload->SetNumberField(TEXT("remainingTicks"), RemainingTicks);
            return Payload;
        }

        virtual TOptional<FSUnrealMcpError> GetError() const override
        {
            return {};
        }

    private:
        int32 RemainingTicks = 0;
        ESUnrealMcpTaskState State = ESUnrealMcpTaskState::Pending;
    };

    class FStartMockTaskCommand final : public ISUnrealMcpCommand
    {
    public:
        virtual FString GetCommandName() const override
        {
            return TEXT("start_mock_task");
        }

        virtual FSUnrealMcpResponse Execute(const FSUnrealMcpRequest& Request, const FSUnrealMcpExecutionContext& Context) override
        {
            int32 TickCount = 3;
            if (Request.Params.IsValid())
            {
                Request.Params->TryGetNumberField(TEXT("tickCount"), TickCount);
            }

            const FString TaskId = Context.TaskRegistry->EnqueueTask(MakeShared<FMockDelayTask>(TickCount));
            return Context.TaskRegistry->MakeAcceptedResponse(Request.RequestId, TaskId);
        }
    };

    const FSUnrealMcpCommandAutoRegistrar StartMockTaskCommandRegistrar([]()
    {
        return MakeShared<FStartMockTaskCommand>();
    });
}
