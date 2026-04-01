#include "Commands/Shared/Editor/EditorCommandUtils.h"
#include "EngineUtils.h"
#include "Mcp/ISUnrealMcpCommand.h"
#include "Mcp/SUnrealMcpCommandRegistry.h"

namespace
{
    class FGetActorsInLevelCommand final : public ISUnrealMcpCommand
    {
    public:
        virtual FString GetCommandName() const override
        {
            return TEXT("get_actors_in_level");
        }

        virtual FSUnrealMcpResponse Execute(const FSUnrealMcpRequest& Request, const FSUnrealMcpExecutionContext& Context) override
        {
            static_cast<void>(Context);

            FSUnrealMcpResponse ErrorResponse;
            UWorld* EditorWorld = SUnrealMcpEditorCommandUtils::GetEditorWorld(Request.RequestId, ErrorResponse);
            if (EditorWorld == nullptr)
            {
                return ErrorResponse;
            }

            TArray<TSharedPtr<FJsonValue>> Actors;
            for (TActorIterator<AActor> It(EditorWorld); It; ++It)
            {
                AActor* Actor = *It;
                Actors.Add(MakeShared<FJsonValueObject>(SUnrealMcpEditorCommandUtils::BuildActorSummary(Actor)));
            }

            TSharedPtr<FJsonObject> Data = MakeShared<FJsonObject>();
            Data->SetArrayField(TEXT("actors"), Actors);
            Data->SetStringField(TEXT("world"), EditorWorld->GetPathName());
            return FSUnrealMcpResponse::MakeSuccess(Request.RequestId, Data);
        }
    };

    const FSUnrealMcpCommandAutoRegistrar GetActorsInLevelCommandRegistrar([]()
    {
        return MakeShared<FGetActorsInLevelCommand>();
    });
}

