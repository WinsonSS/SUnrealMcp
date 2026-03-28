#include "Commands/Extensions/Editor/EditorCommandUtils.h"
#include "Mcp/ISUnrealMcpCommand.h"
#include "Mcp/SUnrealMcpCommandRegistry.h"

namespace
{
    class FSpawnBlueprintActorCommand final : public ISUnrealMcpCommand
    {
    public:
        virtual FString GetCommandName() const override
        {
            return TEXT("spawn_blueprint_actor");
        }

        virtual FSUnrealMcpResponse Execute(const FSUnrealMcpRequest& Request, const FSUnrealMcpExecutionContext& Context) override
        {
            static_cast<void>(Context);

            if (!Request.Params.IsValid())
            {
                return FSUnrealMcpResponse::MakeError(
                    Request.RequestId,
                    TEXT("INVALID_PARAMS"),
                    TEXT("spawn_blueprint_actor requires a params object."));
            }

            FString BlueprintPath;
            if (!Request.Params->TryGetStringField(TEXT("blueprint_path"), BlueprintPath) || BlueprintPath.IsEmpty())
            {
                return FSUnrealMcpResponse::MakeError(
                    Request.RequestId,
                    TEXT("INVALID_PARAMS"),
                    TEXT("Missing blueprint_path."));
            }

            FSUnrealMcpResponse ErrorResponse;
            UWorld* EditorWorld = SUnrealMcpEditorCommandUtils::GetEditorWorld(Request.RequestId, ErrorResponse);
            if (EditorWorld == nullptr)
            {
                return ErrorResponse;
            }

            FString ClassLoadError;
            UClass* ActorClass = SUnrealMcpEditorCommandUtils::LoadBlueprintActorClass(BlueprintPath, ClassLoadError);
            if (ActorClass == nullptr)
            {
                return FSUnrealMcpResponse::MakeError(
                    Request.RequestId,
                    TEXT("INVALID_BLUEPRINT"),
                    ClassLoadError);
            }

            FVector Location = FVector::ZeroVector;
            FVector RotationValues = FVector::ZeroVector;
            SUnrealMcpEditorCommandUtils::TryReadVector3(Request.Params, TEXT("location"), Location);
            SUnrealMcpEditorCommandUtils::TryReadVector3(Request.Params, TEXT("rotation"), RotationValues);

            FActorSpawnParameters SpawnParameters;
            SpawnParameters.Name = NAME_None;

            AActor* SpawnedActor = EditorWorld->SpawnActor<AActor>(
                ActorClass,
                Location,
                FRotator(RotationValues.X, RotationValues.Y, RotationValues.Z),
                SpawnParameters);

            if (SpawnedActor == nullptr)
            {
                return FSUnrealMcpResponse::MakeError(
                    Request.RequestId,
                    TEXT("SPAWN_FAILED"),
                    TEXT("Editor world failed to spawn blueprint actor."));
            }

            FString ActorLabel;
            if (Request.Params->TryGetStringField(TEXT("actor_label"), ActorLabel) && !ActorLabel.IsEmpty())
            {
                SpawnedActor->SetActorLabel(ActorLabel, true);
            }

            TSharedPtr<FJsonObject> Data = MakeShared<FJsonObject>();
            Data->SetStringField(TEXT("blueprintPath"), BlueprintPath);
            Data->SetObjectField(TEXT("actor"), SUnrealMcpEditorCommandUtils::BuildActorSummary(SpawnedActor));
            return FSUnrealMcpResponse::MakeSuccess(Request.RequestId, Data);
        }
    };

    const FSUnrealMcpCommandAutoRegistrar SpawnBlueprintActorCommandRegistrar([]()
    {
        return MakeShared<FSpawnBlueprintActorCommand>();
    });
}
