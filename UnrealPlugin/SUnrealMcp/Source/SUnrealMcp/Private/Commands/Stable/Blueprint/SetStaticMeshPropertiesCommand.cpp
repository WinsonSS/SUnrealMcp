#include "Commands/Shared/Blueprint/BlueprintCommandUtils.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "Mcp/ISUnrealMcpCommand.h"
#include "Mcp/SUnrealMcpCommandRegistry.h"

namespace
{
    class FSetStaticMeshPropertiesCommand final : public ISUnrealMcpCommand
    {
    public:
        virtual FString GetCommandName() const override
        {
            return TEXT("set_static_mesh_properties");
        }

        virtual FSUnrealMcpResponse Execute(const FSUnrealMcpRequest& Request, const FSUnrealMcpExecutionContext& Context) override
        {
            static_cast<void>(Context);

            if (!Request.Params.IsValid())
            {
                return FSUnrealMcpResponse::MakeError(Request.RequestId, TEXT("INVALID_PARAMS"), TEXT("set_static_mesh_properties requires a params object."));
            }

            FString BlueprintPath;
            FString ComponentName;
            FString StaticMeshPath;
            if (!Request.Params->TryGetStringField(TEXT("blueprint_path"), BlueprintPath) || BlueprintPath.IsEmpty())
            {
                return FSUnrealMcpResponse::MakeError(Request.RequestId, TEXT("INVALID_PARAMS"), TEXT("Missing blueprint_path."));
            }
            if (!Request.Params->TryGetStringField(TEXT("component_name"), ComponentName) || ComponentName.IsEmpty())
            {
                return FSUnrealMcpResponse::MakeError(Request.RequestId, TEXT("INVALID_PARAMS"), TEXT("Missing component_name."));
            }
            if (!Request.Params->TryGetStringField(TEXT("static_mesh"), StaticMeshPath) || StaticMeshPath.IsEmpty())
            {
                return FSUnrealMcpResponse::MakeError(Request.RequestId, TEXT("INVALID_PARAMS"), TEXT("Missing static_mesh."));
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

            UStaticMeshComponent* MeshComponent = Cast<UStaticMeshComponent>(ComponentNode->ComponentTemplate);
            if (MeshComponent == nullptr)
            {
                return FSUnrealMcpResponse::MakeError(Request.RequestId, TEXT("INVALID_COMPONENT_TYPE"), TEXT("Component is not a StaticMeshComponent."));
            }

            UStaticMesh* StaticMesh = LoadObject<UStaticMesh>(nullptr, *StaticMeshPath);
            if (StaticMesh == nullptr)
            {
                return FSUnrealMcpResponse::MakeError(Request.RequestId, TEXT("STATIC_MESH_NOT_FOUND"), FString::Printf(TEXT("Could not load static mesh '%s'."), *StaticMeshPath));
            }

            MeshComponent->Modify();
            MeshComponent->SetStaticMesh(StaticMesh);
            FBlueprintEditorUtils::MarkBlueprintAsModified(Blueprint);
            FKismetEditorUtilities::CompileBlueprint(Blueprint);

            TSharedPtr<FJsonObject> Data = MakeShared<FJsonObject>();
            Data->SetObjectField(TEXT("blueprint"), SUnrealMcpBlueprintCommandUtils::BuildBlueprintSummary(Blueprint));
            Data->SetStringField(TEXT("componentName"), ComponentNode->GetVariableName().ToString());
            Data->SetStringField(TEXT("staticMesh"), StaticMesh->GetPathName());
            return FSUnrealMcpResponse::MakeSuccess(Request.RequestId, Data);
        }
    };

    const FSUnrealMcpCommandAutoRegistrar SetStaticMeshPropertiesCommandRegistrar([]()
    {
        return MakeShared<FSetStaticMeshPropertiesCommand>();
    });
}

