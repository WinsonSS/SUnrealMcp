#include "Commands/Extensions/Node/NodeCommandUtils.h"
#include "Mcp/ISUnrealMcpCommand.h"
#include "Mcp/SUnrealMcpCommandRegistry.h"

namespace
{
    static bool MatchesEventTypeFilter(const FString& EventMemberName, const FString& EventTypeFilter)
    {
        if (EventTypeFilter.IsEmpty())
        {
            return true;
        }

        if (EventMemberName.Equals(EventTypeFilter, ESearchCase::IgnoreCase))
        {
            return true;
        }

        if (EventMemberName.Equals(FString::Printf(TEXT("Receive%s"), *EventTypeFilter), ESearchCase::IgnoreCase))
        {
            return true;
        }

        if (EventMemberName.StartsWith(TEXT("Receive"), ESearchCase::IgnoreCase)
            && EventMemberName.RightChop(7).Equals(EventTypeFilter, ESearchCase::IgnoreCase))
        {
            return true;
        }

        return false;
    }

    class FFindBlueprintNodesCommand final : public ISUnrealMcpCommand
    {
    public:
        virtual FString GetCommandName() const override
        {
            return TEXT("find_blueprint_nodes");
        }

        virtual FSUnrealMcpResponse Execute(const FSUnrealMcpRequest& Request, const FSUnrealMcpExecutionContext& Context) override
        {
            static_cast<void>(Context);

            if (!Request.Params.IsValid())
            {
                return FSUnrealMcpResponse::MakeError(Request.RequestId, TEXT("INVALID_PARAMS"), TEXT("find_blueprint_nodes requires a params object."));
            }

            FString BlueprintPath;
            FString NodeType;
            FString EventType;
            if (!Request.Params->TryGetStringField(TEXT("blueprint_path"), BlueprintPath) || BlueprintPath.IsEmpty())
            {
                return FSUnrealMcpResponse::MakeError(Request.RequestId, TEXT("INVALID_PARAMS"), TEXT("Missing blueprint_path."));
            }
            Request.Params->TryGetStringField(TEXT("node_type"), NodeType);
            Request.Params->TryGetStringField(TEXT("event_type"), EventType);

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

            TArray<TSharedPtr<FJsonValue>> MatchingNodes;
            for (UEdGraphNode* Node : EventGraph->Nodes)
            {
                if (Node == nullptr)
                {
                    continue;
                }

                bool bMatchesType = NodeType.IsEmpty();
                if (!bMatchesType)
                {
                    if (NodeType.Equals(TEXT("Event"), ESearchCase::IgnoreCase))
                    {
                        bMatchesType = Node->IsA<UK2Node_Event>();
                    }
                    else if (NodeType.Equals(TEXT("Function"), ESearchCase::IgnoreCase))
                    {
                        bMatchesType = Node->IsA<UK2Node_CallFunction>();
                    }
                    else if (NodeType.Equals(TEXT("Variable"), ESearchCase::IgnoreCase))
                    {
                        bMatchesType = Node->IsA<UK2Node_VariableGet>();
                    }
                    else if (NodeType.Equals(TEXT("Self"), ESearchCase::IgnoreCase))
                    {
                        bMatchesType = Node->IsA<UK2Node_Self>();
                    }
                    else if (NodeType.Equals(TEXT("InputAction"), ESearchCase::IgnoreCase))
                    {
                        bMatchesType = Node->IsA<UK2Node_EnhancedInputAction>();
                    }
                }

                if (!bMatchesType)
                {
                    continue;
                }

                if (!EventType.IsEmpty())
                {
                    const UK2Node_Event* EventNode = Cast<UK2Node_Event>(Node);
                    if (EventNode == nullptr || !MatchesEventTypeFilter(EventNode->EventReference.GetMemberName().ToString(), EventType))
                    {
                        continue;
                    }
                }

                MatchingNodes.Add(MakeShared<FJsonValueObject>(SUnrealMcpNodeCommandUtils::BuildNodeSummary(Node)));
            }

            TSharedPtr<FJsonObject> Data = MakeShared<FJsonObject>();
            Data->SetObjectField(TEXT("blueprint"), SUnrealMcpBlueprintCommandUtils::BuildBlueprintSummary(Blueprint));
            Data->SetStringField(TEXT("nodeType"), NodeType);
            Data->SetStringField(TEXT("eventType"), EventType);
            Data->SetArrayField(TEXT("nodes"), MatchingNodes);
            Data->SetNumberField(TEXT("count"), MatchingNodes.Num());
            return FSUnrealMcpResponse::MakeSuccess(Request.RequestId, Data);
        }
    };

    const FSUnrealMcpCommandAutoRegistrar FindBlueprintNodesCommandRegistrar([]()
    {
        return MakeShared<FFindBlueprintNodesCommand>();
    });
}
