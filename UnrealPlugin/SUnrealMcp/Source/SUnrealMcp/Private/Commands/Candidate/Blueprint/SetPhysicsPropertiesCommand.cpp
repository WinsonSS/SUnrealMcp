#include "Commands/Shared/Blueprint/BlueprintCommandUtils.h"
#include "Components/PrimitiveComponent.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "Mcp/ISUnrealMcpCommand.h"
#include "Mcp/SUnrealMcpCommandRegistry.h"

namespace
{
    class FSetPhysicsPropertiesCommand final : public ISUnrealMcpCommand
    {
    public:
        virtual FString GetCommandName() const override
        {
            return TEXT("set_physics_properties");
        }

        virtual FSUnrealMcpResponse Execute(const FSUnrealMcpRequest& Request, const FSUnrealMcpExecutionContext& Context) override
        {
            static_cast<void>(Context);

            if (!Request.Params.IsValid())
            {
                return FSUnrealMcpResponse::MakeError(Request.RequestId, TEXT("INVALID_PARAMS"), TEXT("set_physics_properties requires a params object."));
            }

            FString BlueprintPath;
            FString ComponentName;
            if (!Request.Params->TryGetStringField(TEXT("blueprint_path"), BlueprintPath) || BlueprintPath.IsEmpty())
            {
                return FSUnrealMcpResponse::MakeError(Request.RequestId, TEXT("INVALID_PARAMS"), TEXT("Missing blueprint_path."));
            }
            if (!Request.Params->TryGetStringField(TEXT("component_name"), ComponentName) || ComponentName.IsEmpty())
            {
                return FSUnrealMcpResponse::MakeError(Request.RequestId, TEXT("INVALID_PARAMS"), TEXT("Missing component_name."));
            }

            UBlueprint* Blueprint = SUnrealMcpBlueprintCommandUtils::LoadBlueprintAsset(BlueprintPath);
            if (Blueprint == nullptr)
            {
                return FSUnrealMcpResponse::MakeError(Request.RequestId, TEXT("BLUEPRINT_NOT_FOUND"), FString::Printf(TEXT("Could not load blueprint '%s'."), *BlueprintPath));
            }

            USCS_Node* ComponentNode = SUnrealMcpBlueprintCommandUtils::FindBlueprintComponentNode(Blueprint, ComponentName);
            if (ComponentNode == nullptr)
            {
                return FSUnrealMcpResponse::MakeError(Request.RequestId, TEXT("COMPONENT_NOT_FOUND"), FString::Printf(TEXT("Could not find component '%s'."), *ComponentName));
            }

            UPrimitiveComponent* PrimitiveComponent = Cast<UPrimitiveComponent>(ComponentNode->ComponentTemplate);
            if (PrimitiveComponent == nullptr)
            {
                return FSUnrealMcpResponse::MakeError(Request.RequestId, TEXT("INVALID_COMPONENT_TYPE"), TEXT("Component is not a PrimitiveComponent."));
            }

            bool bSimulatePhysics = true;
            bool bGravityEnabled = true;
            double Mass = 1.0;
            double LinearDamping = 0.01;
            double AngularDamping = 0.0;

            Request.Params->TryGetBoolField(TEXT("simulate_physics"), bSimulatePhysics);
            Request.Params->TryGetBoolField(TEXT("gravity_enabled"), bGravityEnabled);
            Request.Params->TryGetNumberField(TEXT("mass"), Mass);
            Request.Params->TryGetNumberField(TEXT("linear_damping"), LinearDamping);
            Request.Params->TryGetNumberField(TEXT("angular_damping"), AngularDamping);

            PrimitiveComponent->Modify();
            PrimitiveComponent->SetSimulatePhysics(bSimulatePhysics);
            PrimitiveComponent->SetEnableGravity(bGravityEnabled);
            PrimitiveComponent->SetMassOverrideInKg(NAME_None, static_cast<float>(Mass));
            PrimitiveComponent->SetLinearDamping(static_cast<float>(LinearDamping));
            PrimitiveComponent->SetAngularDamping(static_cast<float>(AngularDamping));
            PrimitiveComponent->PostEditChange();

            FBlueprintEditorUtils::MarkBlueprintAsModified(Blueprint);
            FKismetEditorUtilities::CompileBlueprint(Blueprint);

            TSharedPtr<FJsonObject> Data = MakeShared<FJsonObject>();
            Data->SetObjectField(TEXT("blueprint"), SUnrealMcpBlueprintCommandUtils::BuildBlueprintSummary(Blueprint));
            Data->SetStringField(TEXT("componentName"), ComponentNode->GetVariableName().ToString());
            Data->SetBoolField(TEXT("simulatePhysics"), bSimulatePhysics);
            Data->SetBoolField(TEXT("gravityEnabled"), bGravityEnabled);
            Data->SetNumberField(TEXT("mass"), Mass);
            Data->SetNumberField(TEXT("linearDamping"), LinearDamping);
            Data->SetNumberField(TEXT("angularDamping"), AngularDamping);
            return FSUnrealMcpResponse::MakeSuccess(Request.RequestId, Data);
        }
    };

    const FSUnrealMcpCommandAutoRegistrar SetPhysicsPropertiesCommandRegistrar([]()
    {
        return MakeShared<FSetPhysicsPropertiesCommand>();
    });
}

