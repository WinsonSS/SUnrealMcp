#include "Commands/Shared/Node/NodeCommandUtils.h"
#include "Mcp/ISUnrealMcpCommand.h"
#include "Mcp/SUnrealMcpCommandRegistry.h"

namespace
{
    class FAddBlueprintSelfReferenceCommand final : public ISUnrealMcpCommand
    {
    public:
        virtual FString GetCommandName() const override
        {
            return TEXT("add_blueprint_self_reference");
        }

        virtual FSUnrealMcpResponse Execute(const FSUnrealMcpRequest& Request, const FSUnrealMcpExecutionContext& Context) override
        {
            static_cast<void>(Context);

            if (!Request.Params.IsValid())
            {
                return FSUnrealMcpResponse::MakeError(Request.RequestId, TEXT("INVALID_PARAMS"), TEXT("add_blueprint_self_reference requires a params object."));
            }

            FString BlueprintPath;
            if (!Request.Params->TryGetStringField(TEXT("blueprint_path"), BlueprintPath) || BlueprintPath.IsEmpty())
            {
                return FSUnrealMcpResponse::MakeError(Request.RequestId, TEXT("INVALID_PARAMS"), TEXT("Missing blueprint_path."));
            }

            UBlueprint* Blueprint = SUnrealMcpBlueprintCommandUtils::LoadBlueprintAsset(BlueprintPath);
            if (Blueprint == nullptr)
            {
                return FSUnrealMcpResponse::MakeError(Request.RequestId, TEXT("BLUEPRINT_NOT_FOUND"), FString::Printf(TEXT("Could not load blueprint '%s'."), *BlueprintPath));
            }

            UEdGraph* EventGraph = SUnrealMcpNodeCommandUtils::EnsureEventGraph(Blueprint);
            if (EventGraph == nullptr)
            {
                return FSUnrealMcpResponse::MakeError(Request.RequestId, TEXT("GRAPH_UNAVAILABLE"), TEXT("Could not access the blueprint event graph."));
            }

            FVector2D Position = FVector2D::ZeroVector;
            SUnrealMcpNodeCommandUtils::TryReadVector2(Request.Params, TEXT("node_position"), Position);

            UK2Node_Self* SelfNode = NewObject<UK2Node_Self>(EventGraph);
            SelfNode->NodePosX = FMath::RoundToInt(Position.X);
            SelfNode->NodePosY = FMath::RoundToInt(Position.Y);
            EventGraph->AddNode(SelfNode, true, false);
            SelfNode->CreateNewGuid();
            SelfNode->PostPlacedNewNode();
            SelfNode->AllocateDefaultPins();

            FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(Blueprint);

            TSharedPtr<FJsonObject> Data = MakeShared<FJsonObject>();
            Data->SetObjectField(TEXT("blueprint"), SUnrealMcpBlueprintCommandUtils::BuildBlueprintSummary(Blueprint));
            Data->SetObjectField(TEXT("node"), SUnrealMcpNodeCommandUtils::BuildNodeSummary(SelfNode));
            return FSUnrealMcpResponse::MakeSuccess(Request.RequestId, Data);
        }
    };

    const FSUnrealMcpCommandAutoRegistrar AddBlueprintSelfReferenceCommandRegistrar([]()
    {
        return MakeShared<FAddBlueprintSelfReferenceCommand>();
    });
}

