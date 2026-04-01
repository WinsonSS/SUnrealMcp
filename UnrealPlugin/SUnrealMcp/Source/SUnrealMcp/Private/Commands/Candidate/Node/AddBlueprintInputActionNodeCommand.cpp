#include "Commands/Shared/Node/NodeCommandUtils.h"
#include "Mcp/ISUnrealMcpCommand.h"
#include "Mcp/SUnrealMcpCommandRegistry.h"

namespace
{
    class FAddBlueprintInputActionNodeCommand final : public ISUnrealMcpCommand
    {
    public:
        virtual FString GetCommandName() const override
        {
            return TEXT("add_blueprint_input_action_node");
        }

        virtual FSUnrealMcpResponse Execute(const FSUnrealMcpRequest& Request, const FSUnrealMcpExecutionContext& Context) override
        {
            static_cast<void>(Context);

            if (!Request.Params.IsValid())
            {
                return FSUnrealMcpResponse::MakeError(Request.RequestId, TEXT("INVALID_PARAMS"), TEXT("add_blueprint_input_action_node requires a params object."));
            }

            FString BlueprintPath;
            FString InputActionPath;
            if (!Request.Params->TryGetStringField(TEXT("blueprint_path"), BlueprintPath) || BlueprintPath.IsEmpty())
            {
                return FSUnrealMcpResponse::MakeError(Request.RequestId, TEXT("INVALID_PARAMS"), TEXT("Missing blueprint_path."));
            }
            if (!Request.Params->TryGetStringField(TEXT("input_action_path"), InputActionPath) || InputActionPath.IsEmpty())
            {
                return FSUnrealMcpResponse::MakeError(Request.RequestId, TEXT("INVALID_PARAMS"), TEXT("Missing input_action_path."));
            }

            UBlueprint* Blueprint = SUnrealMcpBlueprintCommandUtils::LoadBlueprintAsset(BlueprintPath);
            if (Blueprint == nullptr)
            {
                return FSUnrealMcpResponse::MakeError(Request.RequestId, TEXT("BLUEPRINT_NOT_FOUND"), FString::Printf(TEXT("Could not load blueprint '%s'."), *BlueprintPath));
            }

            UInputAction* InputAction = LoadObject<UInputAction>(nullptr, *InputActionPath);
            if (InputAction == nullptr)
            {
                return FSUnrealMcpResponse::MakeError(Request.RequestId, TEXT("INPUT_ACTION_NOT_FOUND"), FString::Printf(TEXT("Could not load input action '%s'."), *InputActionPath));
            }

            UEdGraph* EventGraph = SUnrealMcpNodeCommandUtils::EnsureEventGraph(Blueprint);
            if (EventGraph == nullptr)
            {
                return FSUnrealMcpResponse::MakeError(Request.RequestId, TEXT("GRAPH_UNAVAILABLE"), TEXT("Could not access the blueprint event graph."));
            }

            TArray<UK2Node_EnhancedInputAction*> ExistingNodes;
            FBlueprintEditorUtils::GetAllNodesOfClass(Blueprint, ExistingNodes);
            for (UK2Node_EnhancedInputAction* ExistingNode : ExistingNodes)
            {
                if (ExistingNode != nullptr && ExistingNode->InputAction == InputAction)
                {
                    TSharedPtr<FJsonObject> Data = MakeShared<FJsonObject>();
                    Data->SetObjectField(TEXT("blueprint"), SUnrealMcpBlueprintCommandUtils::BuildBlueprintSummary(Blueprint));
                    Data->SetObjectField(TEXT("node"), SUnrealMcpNodeCommandUtils::BuildNodeSummary(ExistingNode));
                    Data->SetBoolField(TEXT("alreadyExisted"), true);
                    return FSUnrealMcpResponse::MakeSuccess(Request.RequestId, Data);
                }
            }

            FVector2D Position = FVector2D::ZeroVector;
            SUnrealMcpNodeCommandUtils::TryReadVector2(Request.Params, TEXT("node_position"), Position);

            UK2Node_EnhancedInputAction* InputNode = NewObject<UK2Node_EnhancedInputAction>(EventGraph);
            InputNode->InputAction = InputAction;
            InputNode->NodePosX = FMath::RoundToInt(Position.X);
            InputNode->NodePosY = FMath::RoundToInt(Position.Y);
            EventGraph->AddNode(InputNode, true, false);
            InputNode->CreateNewGuid();
            InputNode->PostPlacedNewNode();
            InputNode->AllocateDefaultPins();
            InputNode->ReconstructNode();

            FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(Blueprint);

            TSharedPtr<FJsonObject> Data = MakeShared<FJsonObject>();
            Data->SetObjectField(TEXT("blueprint"), SUnrealMcpBlueprintCommandUtils::BuildBlueprintSummary(Blueprint));
            Data->SetObjectField(TEXT("node"), SUnrealMcpNodeCommandUtils::BuildNodeSummary(InputNode));
            Data->SetBoolField(TEXT("alreadyExisted"), false);
            return FSUnrealMcpResponse::MakeSuccess(Request.RequestId, Data);
        }
    };

    const FSUnrealMcpCommandAutoRegistrar AddBlueprintInputActionNodeCommandRegistrar([]()
    {
        return MakeShared<FAddBlueprintInputActionNodeCommand>();
    });
}

