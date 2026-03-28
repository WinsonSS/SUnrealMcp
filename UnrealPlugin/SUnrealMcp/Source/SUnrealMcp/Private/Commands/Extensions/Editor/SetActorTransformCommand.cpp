#include "Commands/Extensions/Editor/EditorCommandUtils.h"
#include "Mcp/ISUnrealMcpCommand.h"
#include "Mcp/SUnrealMcpCommandRegistry.h"

namespace
{
    class FSetActorTransformCommand final : public ISUnrealMcpCommand
    {
    public:
        virtual FString GetCommandName() const override
        {
            return TEXT("set_actor_transform");
        }

        virtual FSUnrealMcpResponse Execute(const FSUnrealMcpRequest& Request, const FSUnrealMcpExecutionContext& Context) override
        {
            static_cast<void>(Context);

            if (!Request.Params.IsValid())
            {
                return FSUnrealMcpResponse::MakeError(
                    Request.RequestId,
                    TEXT("INVALID_PARAMS"),
                    TEXT("set_actor_transform requires a params object."));
            }

            FString ActorPath;
            if (!Request.Params->TryGetStringField(TEXT("actor_path"), ActorPath) || ActorPath.IsEmpty())
            {
                return FSUnrealMcpResponse::MakeError(
                    Request.RequestId,
                    TEXT("INVALID_PARAMS"),
                    TEXT("Missing actor_path."));
            }

            FSUnrealMcpResponse ErrorResponse;
            UWorld* EditorWorld = SUnrealMcpEditorCommandUtils::GetEditorWorld(Request.RequestId, ErrorResponse);
            if (EditorWorld == nullptr)
            {
                return ErrorResponse;
            }

            AActor* Actor = SUnrealMcpEditorCommandUtils::FindActorByPath(EditorWorld, ActorPath);
            if (Actor == nullptr)
            {
                return FSUnrealMcpResponse::MakeError(
                    Request.RequestId,
                    TEXT("ACTOR_NOT_FOUND"),
                    FString::Printf(TEXT("Could not find actor '%s'."), *ActorPath));
            }

            FVector Location = FVector::ZeroVector;
            FVector RotationValues = FVector::ZeroVector;
            FVector Scale = FVector::OneVector;
            const bool bHasLocation = SUnrealMcpEditorCommandUtils::TryReadVector3(Request.Params, TEXT("location"), Location);
            const bool bHasRotation = SUnrealMcpEditorCommandUtils::TryReadVector3(Request.Params, TEXT("rotation"), RotationValues);
            const bool bHasScale = SUnrealMcpEditorCommandUtils::TryReadVector3(Request.Params, TEXT("scale"), Scale);

            if (!bHasLocation && !bHasRotation && !bHasScale)
            {
                return FSUnrealMcpResponse::MakeError(
                    Request.RequestId,
                    TEXT("INVALID_PARAMS"),
                    TEXT("At least one of location, rotation, or scale must be provided."));
            }

            Actor->Modify();

            if (bHasLocation)
            {
                Actor->SetActorLocation(Location, false, nullptr, ETeleportType::TeleportPhysics);
            }

            if (bHasRotation)
            {
                Actor->SetActorRotation(FRotator(RotationValues.X, RotationValues.Y, RotationValues.Z), ETeleportType::TeleportPhysics);
            }

            if (bHasScale)
            {
                Actor->SetActorScale3D(Scale);
            }

            Actor->PostEditMove(true);
            Actor->MarkPackageDirty();

            TSharedPtr<FJsonObject> Data = MakeShared<FJsonObject>();
            Data->SetObjectField(TEXT("actor"), SUnrealMcpEditorCommandUtils::BuildActorSummary(Actor));
            return FSUnrealMcpResponse::MakeSuccess(Request.RequestId, Data);
        }
    };

    const FSUnrealMcpCommandAutoRegistrar SetActorTransformCommandRegistrar([]()
    {
        return MakeShared<FSetActorTransformCommand>();
    });
}
