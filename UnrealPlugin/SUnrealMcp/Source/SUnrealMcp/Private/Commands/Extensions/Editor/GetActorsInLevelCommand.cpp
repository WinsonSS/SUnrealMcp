#include "Editor.h"
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

            if (GEditor == nullptr)
            {
                return FSUnrealMcpResponse::MakeError(
                    Request.RequestId,
                    TEXT("EDITOR_UNAVAILABLE"),
                    TEXT("GEditor is not available."));
            }

            UWorld* EditorWorld = GEditor->GetEditorWorldContext().World();
            if (EditorWorld == nullptr)
            {
                return FSUnrealMcpResponse::MakeError(
                    Request.RequestId,
                    TEXT("WORLD_UNAVAILABLE"),
                    TEXT("Editor world is not available."));
            }

            TArray<TSharedPtr<FJsonValue>> Actors;
            for (TActorIterator<AActor> It(EditorWorld); It; ++It)
            {
                AActor* Actor = *It;
                TSharedPtr<FJsonObject> ActorObject = MakeShared<FJsonObject>();
                ActorObject->SetStringField(TEXT("label"), Actor->GetActorLabel());
                ActorObject->SetStringField(TEXT("path"), Actor->GetPathName());
                ActorObject->SetStringField(TEXT("class"), Actor->GetClass()->GetPathName());
                ActorObject->SetStringField(TEXT("level"), Actor->GetLevel() ? Actor->GetLevel()->GetPathName() : TEXT(""));

                const FVector Location = Actor->GetActorLocation();
                TArray<TSharedPtr<FJsonValue>> LocationValues;
                LocationValues.Add(MakeShared<FJsonValueNumber>(Location.X));
                LocationValues.Add(MakeShared<FJsonValueNumber>(Location.Y));
                LocationValues.Add(MakeShared<FJsonValueNumber>(Location.Z));
                ActorObject->SetArrayField(TEXT("location"), LocationValues);

                Actors.Add(MakeShared<FJsonValueObject>(ActorObject));
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
