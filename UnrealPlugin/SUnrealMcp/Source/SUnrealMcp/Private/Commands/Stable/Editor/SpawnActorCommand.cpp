#include "Commands/Shared/Editor/EditorCommandUtils.h"
#include "Mcp/ISUnrealMcpCommand.h"
#include "Mcp/SUnrealMcpCommandRegistry.h"

namespace
{
    class FSpawnActorCommand final : public ISUnrealMcpCommand
    {
    public:
        virtual FString GetCommandName() const override
        {
            return TEXT("spawn_actor");
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

            if (!Request.Params.IsValid())
            {
                return FSUnrealMcpResponse::MakeError(
                    Request.RequestId,
                    TEXT("INVALID_PARAMS"),
                    TEXT("spawn_actor requires a params object."));
            }

            FString ActorClassReference;
            if (!Request.Params->TryGetStringField(TEXT("actor_class"), ActorClassReference))
            {
                return FSUnrealMcpResponse::MakeError(
                    Request.RequestId,
                    TEXT("INVALID_PARAMS"),
                    TEXT("Missing actor_class."));
            }

            UClass* ActorClass = SUnrealMcpEditorCommandUtils::ResolveActorClassPath(ActorClassReference);
            if (ActorClass == nullptr)
            {
                return FSUnrealMcpResponse::MakeError(
                    Request.RequestId,
                    TEXT("INVALID_CLASS"),
                    FString::Printf(
                        TEXT("Could not resolve actor class '%s'. Provide a full Unreal class path such as /Script/Engine.StaticMeshActor or /Game/Blueprints/BP_MyActor.BP_MyActor_C."),
                        *ActorClassReference));
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
                    TEXT("Editor world failed to spawn actor."));
            }

            FString ActorLabel;
            if (Request.Params->TryGetStringField(TEXT("actor_label"), ActorLabel) && !ActorLabel.IsEmpty())
            {
                SpawnedActor->SetActorLabel(ActorLabel, true);
            }

            TSharedPtr<FJsonObject> Data = MakeShared<FJsonObject>();
            Data->SetObjectField(TEXT("actor"), SUnrealMcpEditorCommandUtils::BuildActorSummary(SpawnedActor));
            return FSUnrealMcpResponse::MakeSuccess(Request.RequestId, Data);
        }
    };

    const FSUnrealMcpCommandAutoRegistrar SpawnActorCommandRegistrar([]()
    {
        return MakeShared<FSpawnActorCommand>();
    });
}

