#include "Editor.h"
#include "Engine/World.h"
#include "Mcp/ISUnrealMcpCommand.h"
#include "Mcp/SUnrealMcpCommandRegistry.h"

namespace
{
    class FSpawnActorCommand final : public ISUnrealMcpCommand
    {
    public:
        static bool LooksLikeClassPath(const FString& ActorClassReference)
        {
            return ActorClassReference.Contains(TEXT("/")) && ActorClassReference.Contains(TEXT("."));
        }

        static bool TryReadVector3(const TSharedPtr<FJsonObject>& Object, const FString& FieldName, FVector& OutVector)
        {
            const TArray<TSharedPtr<FJsonValue>>* JsonArray = nullptr;
            if (!Object.IsValid() || !Object->TryGetArrayField(FieldName, JsonArray) || JsonArray->Num() != 3)
            {
                return false;
            }

            double X = 0.0;
            double Y = 0.0;
            double Z = 0.0;
            if (!(*JsonArray)[0]->TryGetNumber(X) || !(*JsonArray)[1]->TryGetNumber(Y) || !(*JsonArray)[2]->TryGetNumber(Z))
            {
                return false;
            }

            OutVector = FVector(X, Y, Z);
            return true;
        }

        static UClass* ResolveActorClass(const FString& ActorClassReference)
        {
            if (ActorClassReference.IsEmpty() || !LooksLikeClassPath(ActorClassReference))
            {
                return nullptr;
            }

            return LoadObject<UClass>(nullptr, *ActorClassReference);
        }

        virtual FString GetCommandName() const override
        {
            return TEXT("spawn_actor");
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

            UClass* ActorClass = ResolveActorClass(ActorClassReference);
            if (ActorClass == nullptr || !ActorClass->IsChildOf(AActor::StaticClass()))
            {
                return FSUnrealMcpResponse::MakeError(
                    Request.RequestId,
                    TEXT("INVALID_CLASS"),
                    FString::Printf(
                        TEXT("Could not resolve actor class '%s'. Provide a full Unreal class path such as /Script/Engine.StaticMeshActor or /Game/Blueprints/BP_MyActor.BP_MyActor_C."),
                        *ActorClassReference));
            }

            FVector Location = FVector::ZeroVector;
            FVector RotationEuler = FVector::ZeroVector;
            TryReadVector3(Request.Params, TEXT("location"), Location);
            TryReadVector3(Request.Params, TEXT("rotation"), RotationEuler);

            FActorSpawnParameters SpawnParameters;
            SpawnParameters.Name = NAME_None;

            AActor* SpawnedActor = EditorWorld->SpawnActor<AActor>(
                ActorClass,
                Location,
                FRotator::MakeFromEuler(RotationEuler),
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
            Data->SetStringField(TEXT("label"), SpawnedActor->GetActorLabel());
            Data->SetStringField(TEXT("path"), SpawnedActor->GetPathName());
            Data->SetStringField(TEXT("class"), SpawnedActor->GetClass()->GetPathName());
            return FSUnrealMcpResponse::MakeSuccess(Request.RequestId, Data);
        }
    };

    const FSUnrealMcpCommandAutoRegistrar SpawnActorCommandRegistrar([]()
    {
        return MakeShared<FSpawnActorCommand>();
    });
}
