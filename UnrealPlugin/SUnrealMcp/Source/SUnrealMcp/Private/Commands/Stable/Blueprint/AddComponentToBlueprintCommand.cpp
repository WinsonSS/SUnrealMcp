#include "Commands/Shared/Blueprint/BlueprintCommandUtils.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "Mcp/ISUnrealMcpCommand.h"
#include "Mcp/SUnrealMcpCommandRegistry.h"

namespace
{
    class FAddComponentToBlueprintCommand final : public ISUnrealMcpCommand
    {
    public:
        virtual FString GetCommandName() const override
        {
            return TEXT("add_component_to_blueprint");
        }

        virtual FSUnrealMcpResponse Execute(const FSUnrealMcpRequest& Request, const FSUnrealMcpExecutionContext& Context) override
        {
            static_cast<void>(Context);

            if (!Request.Params.IsValid())
            {
                return FSUnrealMcpResponse::MakeError(
                    Request.RequestId,
                    TEXT("INVALID_PARAMS"),
                    TEXT("add_component_to_blueprint requires a params object."));
            }

            FString BlueprintPath;
            FString ComponentType;
            FString ComponentName;
            if (!Request.Params->TryGetStringField(TEXT("blueprint_path"), BlueprintPath) || BlueprintPath.IsEmpty())
            {
                return FSUnrealMcpResponse::MakeError(Request.RequestId, TEXT("INVALID_PARAMS"), TEXT("Missing blueprint_path."));
            }

            if (!Request.Params->TryGetStringField(TEXT("component_type"), ComponentType) || ComponentType.IsEmpty())
            {
                return FSUnrealMcpResponse::MakeError(Request.RequestId, TEXT("INVALID_PARAMS"), TEXT("Missing component_type."));
            }

            if (!Request.Params->TryGetStringField(TEXT("component_name"), ComponentName) || ComponentName.IsEmpty())
            {
                return FSUnrealMcpResponse::MakeError(Request.RequestId, TEXT("INVALID_PARAMS"), TEXT("Missing component_name."));
            }

            UBlueprint* Blueprint = SUnrealMcpBlueprintCommandUtils::LoadBlueprintAsset(BlueprintPath);
            if (Blueprint == nullptr)
            {
                return FSUnrealMcpResponse::MakeError(
                    Request.RequestId,
                    TEXT("BLUEPRINT_NOT_FOUND"),
                    FString::Printf(TEXT("Could not load blueprint '%s'."), *BlueprintPath));
            }

            if (Blueprint->SimpleConstructionScript == nullptr)
            {
                return FSUnrealMcpResponse::MakeError(
                    Request.RequestId,
                    TEXT("UNSUPPORTED_BLUEPRINT"),
                    TEXT("Blueprint does not support SimpleConstructionScript components."));
            }

            if (SUnrealMcpBlueprintCommandUtils::FindBlueprintComponentNode(Blueprint, ComponentName) != nullptr)
            {
                return FSUnrealMcpResponse::MakeError(
                    Request.RequestId,
                    TEXT("COMPONENT_ALREADY_EXISTS"),
                    FString::Printf(TEXT("Component '%s' already exists."), *ComponentName));
            }

            UClass* ComponentClass = SUnrealMcpBlueprintCommandUtils::ResolveComponentClassReference(ComponentType);
            if (ComponentClass == nullptr)
            {
                return FSUnrealMcpResponse::MakeError(
                    Request.RequestId,
                    TEXT("INVALID_COMPONENT_TYPE"),
                    FString::Printf(TEXT("Could not resolve component type '%s'."), *ComponentType));
            }

            USCS_Node* NewNode = Blueprint->SimpleConstructionScript->CreateNode(ComponentClass, *ComponentName);
            if (NewNode == nullptr)
            {
                return FSUnrealMcpResponse::MakeError(
                    Request.RequestId,
                    TEXT("COMPONENT_CREATE_FAILED"),
                    TEXT("Failed to create blueprint component node."));
            }

            if (USceneComponent* SceneComponent = Cast<USceneComponent>(NewNode->ComponentTemplate))
            {
                FVector Location = FVector::ZeroVector;
                FVector RotationValues = FVector::ZeroVector;
                FVector Scale = FVector::OneVector;

                if (SUnrealMcpEditorCommandUtils::TryReadVector3(Request.Params, TEXT("location"), Location))
                {
                    SceneComponent->SetRelativeLocation(Location);
                }

                if (SUnrealMcpEditorCommandUtils::TryReadVector3(Request.Params, TEXT("rotation"), RotationValues))
                {
                    SceneComponent->SetRelativeRotation(FRotator(RotationValues.X, RotationValues.Y, RotationValues.Z));
                }

                if (SUnrealMcpEditorCommandUtils::TryReadVector3(Request.Params, TEXT("scale"), Scale))
                {
                    SceneComponent->SetRelativeScale3D(Scale);
                }
            }

            if (const TSharedPtr<FJsonObject>* ComponentProperties = nullptr;
                Request.Params->TryGetObjectField(TEXT("component_properties"), ComponentProperties) && ComponentProperties != nullptr)
            {
                for (const TPair<FString, TSharedPtr<FJsonValue>>& Pair : (*ComponentProperties)->Values)
                {
                    FProperty* Property = SUnrealMcpEditorCommandUtils::FindObjectProperty(NewNode->ComponentTemplate, Pair.Key);
                    if (!SUnrealMcpEditorCommandUtils::IsEditableObjectProperty(Property))
                    {
                        return FSUnrealMcpResponse::MakeError(
                            Request.RequestId,
                            TEXT("PROPERTY_NOT_FOUND"),
                            FString::Printf(TEXT("Could not find supported component property '%s'."), *Pair.Key));
                    }

                    FString ApplyError;
                    if (!SUnrealMcpEditorCommandUtils::ApplyJsonValueToProperty(Pair.Value, Property, NewNode->ComponentTemplate, ApplyError))
                    {
                        return FSUnrealMcpResponse::MakeError(
                            Request.RequestId,
                            TEXT("INVALID_PROPERTY_VALUE"),
                            FString::Printf(TEXT("Failed to apply component property '%s': %s"), *Pair.Key, *ApplyError));
                    }
                }
            }

            Blueprint->SimpleConstructionScript->AddNode(NewNode);
            FBlueprintEditorUtils::MarkBlueprintAsModified(Blueprint);
            FKismetEditorUtilities::CompileBlueprint(Blueprint);

            TSharedPtr<FJsonObject> ComponentData = MakeShared<FJsonObject>();
            ComponentData->SetStringField(TEXT("name"), NewNode->GetVariableName().ToString());
            ComponentData->SetStringField(TEXT("class"), ComponentClass->GetPathName());

            TSharedPtr<FJsonObject> Data = MakeShared<FJsonObject>();
            Data->SetObjectField(TEXT("blueprint"), SUnrealMcpBlueprintCommandUtils::BuildBlueprintSummary(Blueprint));
            Data->SetObjectField(TEXT("component"), ComponentData);
            return FSUnrealMcpResponse::MakeSuccess(Request.RequestId, Data);
        }
    };

    const FSUnrealMcpCommandAutoRegistrar AddComponentToBlueprintCommandRegistrar([]()
    {
        return MakeShared<FAddComponentToBlueprintCommand>();
    });
}

