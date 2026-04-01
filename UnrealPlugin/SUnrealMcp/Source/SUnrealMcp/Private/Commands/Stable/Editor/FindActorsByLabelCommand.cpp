#include "Commands/Shared/Editor/EditorCommandUtils.h"
#include "EngineUtils.h"
#include "Mcp/ISUnrealMcpCommand.h"
#include "Mcp/SUnrealMcpCommandRegistry.h"

namespace
{
    class FFindActorsByLabelCommand final : public ISUnrealMcpCommand
    {
    public:
        virtual FString GetCommandName() const override
        {
            return TEXT("find_actors_by_label");
        }

        virtual FSUnrealMcpResponse Execute(const FSUnrealMcpRequest& Request, const FSUnrealMcpExecutionContext& Context) override
        {
            static_cast<void>(Context);

            if (!Request.Params.IsValid())
            {
                return FSUnrealMcpResponse::MakeError(
                    Request.RequestId,
                    TEXT("INVALID_PARAMS"),
                    TEXT("find_actors_by_label requires a params object."));
            }

            FString Pattern;
            if (!Request.Params->TryGetStringField(TEXT("pattern"), Pattern) || Pattern.IsEmpty())
            {
                return FSUnrealMcpResponse::MakeError(
                    Request.RequestId,
                    TEXT("INVALID_PARAMS"),
                    TEXT("Missing pattern."));
            }

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
                if (Actor != nullptr && SUnrealMcpEditorCommandUtils::MatchesActorLabel(Actor->GetActorLabel(), Pattern))
                {
                    Actors.Add(MakeShared<FJsonValueObject>(SUnrealMcpEditorCommandUtils::BuildActorSummary(Actor)));
                }
            }

            TSharedPtr<FJsonObject> Data = MakeShared<FJsonObject>();
            Data->SetStringField(TEXT("pattern"), Pattern);
            Data->SetArrayField(TEXT("actors"), Actors);
            Data->SetNumberField(TEXT("count"), Actors.Num());
            return FSUnrealMcpResponse::MakeSuccess(Request.RequestId, Data);
        }
    };

    const FSUnrealMcpCommandAutoRegistrar FindActorsByLabelCommandRegistrar([]()
    {
        return MakeShared<FFindActorsByLabelCommand>();
    });
}

