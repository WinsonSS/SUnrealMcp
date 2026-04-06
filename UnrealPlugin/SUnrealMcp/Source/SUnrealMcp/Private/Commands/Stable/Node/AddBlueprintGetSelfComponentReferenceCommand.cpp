#include "Commands/Shared/Node/NodeCommandUtils.h"
#include "Mcp/ISUnrealMcpCommand.h"
#include "Mcp/SUnrealMcpCommandRegistry.h"

namespace
{
    class FAddBlueprintGetSelfComponentReferenceCommand final : public ISUnrealMcpCommand
    {
    public:
        virtual FString GetCommandName() const override
        {
            return TEXT("add_blueprint_get_self_component_reference");
        }

        virtual FSUnrealMcpResponse Execute(const FSUnrealMcpRequest& Request, const FSUnrealMcpExecutionContext& Context) override
        {
            static_cast<void>(Context);

            if (!Request.Params.IsValid())
            {
                return FSUnrealMcpResponse::MakeError(Request.RequestId, TEXT("INVALID_PARAMS"), TEXT("add_blueprint_get_self_component_reference requires a params object."));
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

            if (SUnrealMcpBlueprintCommandUtils::FindBlueprintComponentNode(Blueprint, ComponentName) == nullptr)
            {
                return FSUnrealMcpResponse::MakeError(Request.RequestId, TEXT("COMPONENT_NOT_FOUND"), FString::Printf(TEXT("Could not find component '%s'."), *ComponentName));
            }

            UEdGraph* EventGraph = SUnrealMcpNodeCommandUtils::EnsureEventGraph(Blueprint);
            if (EventGraph == nullptr)
            {
                return FSUnrealMcpResponse::MakeError(Request.RequestId, TEXT("GRAPH_UNAVAILABLE"), TEXT("Could not access the blueprint event graph."));
            }

            FVector2D Position = FVector2D::ZeroVector;
            SUnrealMcpNodeCommandUtils::TryReadVector2(Request.Params, TEXT("node_position"), Position);

            UK2Node_VariableGet* GetNode = NewObject<UK2Node_VariableGet>(EventGraph);
            GetNode->VariableReference.SetSelfMember(FName(*ComponentName));
            GetNode->NodePosX = FMath::RoundToInt(Position.X);
            GetNode->NodePosY = FMath::RoundToInt(Position.Y);
            EventGraph->AddNode(GetNode, true, false);
            GetNode->CreateNewGuid();
            GetNode->PostPlacedNewNode();
            GetNode->AllocateDefaultPins();
            GetNode->ReconstructNode();

            FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(Blueprint);

            TSharedPtr<FJsonObject> Data = MakeShared<FJsonObject>();
            Data->SetObjectField(TEXT("blueprint"), SUnrealMcpBlueprintCommandUtils::BuildBlueprintSummary(Blueprint));
            Data->SetObjectField(TEXT("node"), SUnrealMcpNodeCommandUtils::BuildNodeSummary(GetNode));
            Data->SetStringField(TEXT("componentName"), ComponentName);
            return FSUnrealMcpResponse::MakeSuccess(Request.RequestId, Data);
        }
    };

    const FSUnrealMcpCommandAutoRegistrar AddBlueprintGetSelfComponentReferenceCommandRegistrar([]()
    {
        return MakeShared<FAddBlueprintGetSelfComponentReferenceCommand>();
    });
}

