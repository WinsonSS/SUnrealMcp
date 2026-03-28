#include "Commands/Extensions/Node/NodeCommandUtils.h"
#include "Mcp/ISUnrealMcpCommand.h"
#include "Mcp/SUnrealMcpCommandRegistry.h"

namespace
{
    class FConnectBlueprintNodesCommand final : public ISUnrealMcpCommand
    {
    public:
        virtual FString GetCommandName() const override
        {
            return TEXT("connect_blueprint_nodes");
        }

        virtual FSUnrealMcpResponse Execute(const FSUnrealMcpRequest& Request, const FSUnrealMcpExecutionContext& Context) override
        {
            static_cast<void>(Context);

            if (!Request.Params.IsValid())
            {
                return FSUnrealMcpResponse::MakeError(Request.RequestId, TEXT("INVALID_PARAMS"), TEXT("connect_blueprint_nodes requires a params object."));
            }

            FString BlueprintPath;
            FString SourceNodeId;
            FString SourcePinName;
            FString TargetNodeId;
            FString TargetPinName;
            if (!Request.Params->TryGetStringField(TEXT("blueprint_path"), BlueprintPath) || BlueprintPath.IsEmpty())
            {
                return FSUnrealMcpResponse::MakeError(Request.RequestId, TEXT("INVALID_PARAMS"), TEXT("Missing blueprint_path."));
            }
            if (!Request.Params->TryGetStringField(TEXT("source_node_id"), SourceNodeId) || SourceNodeId.IsEmpty())
            {
                return FSUnrealMcpResponse::MakeError(Request.RequestId, TEXT("INVALID_PARAMS"), TEXT("Missing source_node_id."));
            }
            if (!Request.Params->TryGetStringField(TEXT("source_pin"), SourcePinName) || SourcePinName.IsEmpty())
            {
                return FSUnrealMcpResponse::MakeError(Request.RequestId, TEXT("INVALID_PARAMS"), TEXT("Missing source_pin."));
            }
            if (!Request.Params->TryGetStringField(TEXT("target_node_id"), TargetNodeId) || TargetNodeId.IsEmpty())
            {
                return FSUnrealMcpResponse::MakeError(Request.RequestId, TEXT("INVALID_PARAMS"), TEXT("Missing target_node_id."));
            }
            if (!Request.Params->TryGetStringField(TEXT("target_pin"), TargetPinName) || TargetPinName.IsEmpty())
            {
                return FSUnrealMcpResponse::MakeError(Request.RequestId, TEXT("INVALID_PARAMS"), TEXT("Missing target_pin."));
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

            UEdGraphNode* SourceNode = SUnrealMcpNodeCommandUtils::FindNodeByGuid(EventGraph, SourceNodeId);
            UEdGraphNode* TargetNode = SUnrealMcpNodeCommandUtils::FindNodeByGuid(EventGraph, TargetNodeId);
            if (SourceNode == nullptr || TargetNode == nullptr)
            {
                return FSUnrealMcpResponse::MakeError(Request.RequestId, TEXT("NODE_NOT_FOUND"), TEXT("Could not resolve the source or target node."));
            }

            UEdGraphPin* SourcePin = SUnrealMcpNodeCommandUtils::FindPinByName(SourceNode, SourcePinName, EGPD_Output);
            UEdGraphPin* TargetPin = SUnrealMcpNodeCommandUtils::FindPinByName(TargetNode, TargetPinName, EGPD_Input);
            if (SourcePin == nullptr || TargetPin == nullptr)
            {
                return FSUnrealMcpResponse::MakeError(Request.RequestId, TEXT("PIN_NOT_FOUND"), TEXT("Could not resolve the source or target pin."));
            }

            const UEdGraphSchema* Schema = EventGraph->GetSchema();
            const bool bConnected = Schema != nullptr ? Schema->TryCreateConnection(SourcePin, TargetPin) : false;
            if (!bConnected)
            {
                return FSUnrealMcpResponse::MakeError(Request.RequestId, TEXT("CONNECT_FAILED"), TEXT("The requested pins could not be connected."));
            }

            FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(Blueprint);

            TSharedPtr<FJsonObject> Data = MakeShared<FJsonObject>();
            Data->SetObjectField(TEXT("blueprint"), SUnrealMcpBlueprintCommandUtils::BuildBlueprintSummary(Blueprint));
            Data->SetObjectField(TEXT("sourceNode"), SUnrealMcpNodeCommandUtils::BuildNodeSummary(SourceNode));
            Data->SetObjectField(TEXT("targetNode"), SUnrealMcpNodeCommandUtils::BuildNodeSummary(TargetNode));
            Data->SetStringField(TEXT("sourcePin"), SourcePin->PinName.ToString());
            Data->SetStringField(TEXT("targetPin"), TargetPin->PinName.ToString());
            return FSUnrealMcpResponse::MakeSuccess(Request.RequestId, Data);
        }
    };

    const FSUnrealMcpCommandAutoRegistrar ConnectBlueprintNodesCommandRegistrar([]()
    {
        return MakeShared<FConnectBlueprintNodesCommand>();
    });
}
