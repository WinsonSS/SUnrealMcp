#include "Commands/Shared/Node/NodeCommandUtils.h"
#include "Mcp/ISUnrealMcpCommand.h"
#include "Mcp/SUnrealMcpCommandRegistry.h"

namespace
{
    static FString GetGraphDisplayName(UEdGraph* Graph)
    {
        if (Graph == nullptr)
        {
            return TEXT("");
        }

        FString DisplayName = Graph->GetName();
        if (Graph->GetSchema() != nullptr)
        {
            FGraphDisplayInfo DisplayInfo;
            Graph->GetSchema()->GetGraphDisplayInformation(*Graph, DisplayInfo);
            DisplayName = DisplayInfo.PlainName.ToString();
        }

        return DisplayName;
    }

    static TSharedPtr<FJsonObject> BuildGraphListing(UEdGraph* Graph, const FString& GraphType)
    {
        TSharedPtr<FJsonObject> GraphObject = MakeShared<FJsonObject>();
        if (Graph == nullptr)
        {
            return GraphObject;
        }

        GraphObject->SetStringField(TEXT("name"), Graph->GetName());
        GraphObject->SetStringField(TEXT("type"), GraphType);
        GraphObject->SetStringField(TEXT("class"), Graph->GetClass()->GetPathName());
        GraphObject->SetStringField(TEXT("display_name"), GetGraphDisplayName(Graph));
        GraphObject->SetNumberField(TEXT("node_count"), Graph->Nodes.Num());
        return GraphObject;
    }

    static void AddTopLevelGraphs(
        const TArray<UEdGraph*>& SourceGraphs,
        const FString& GraphType,
        TArray<TSharedPtr<FJsonValue>>& OutGraphs)
    {
        for (int32 GraphIndex = 0; GraphIndex < SourceGraphs.Num(); ++GraphIndex)
        {
            UEdGraph* Graph = SourceGraphs[GraphIndex];
            if (Graph == nullptr)
            {
                continue;
            }

            TSharedPtr<FJsonObject> GraphObject = BuildGraphListing(Graph, GraphType);
            if (GraphType.Equals(TEXT("Event"), ESearchCase::CaseSensitive))
            {
                GraphObject->SetNumberField(TEXT("index"), GraphIndex);
            }
            OutGraphs.Add(MakeShared<FJsonValueObject>(GraphObject));
        }
    }

    static void AddCollapsedGraphsFromGraph(
        UEdGraph* SourceGraph,
        const FString& SourceGraphType,
        TArray<TSharedPtr<FJsonValue>>& OutGraphs,
        TSet<const UEdGraph*>& VisitedCollapsedGraphs)
    {
        if (SourceGraph == nullptr)
        {
            return;
        }

        for (UEdGraphNode* Node : SourceGraph->Nodes)
        {
            const UK2Node_Composite* CompositeNode = Cast<UK2Node_Composite>(Node);
            if (CompositeNode == nullptr || CompositeNode->BoundGraph == nullptr)
            {
                continue;
            }

            UEdGraph* CollapsedGraph = CompositeNode->BoundGraph;
            if (VisitedCollapsedGraphs.Contains(CollapsedGraph))
            {
                continue;
            }
            VisitedCollapsedGraphs.Add(CollapsedGraph);

            TSharedPtr<FJsonObject> GraphObject = BuildGraphListing(CollapsedGraph, TEXT("Collapsed"));
            GraphObject->SetStringField(TEXT("source_graph_name"), SourceGraph->GetName());
            GraphObject->SetStringField(TEXT("source_graph_type"), SourceGraphType);
            GraphObject->SetStringField(TEXT("source_node_id"), CompositeNode->NodeGuid.ToString());
            GraphObject->SetStringField(TEXT("source_node_title"), CompositeNode->GetNodeTitle(ENodeTitleType::ListView).ToString());
            OutGraphs.Add(MakeShared<FJsonValueObject>(GraphObject));

            AddCollapsedGraphsFromGraph(CollapsedGraph, TEXT("Collapsed"), OutGraphs, VisitedCollapsedGraphs);
        }
    }

    static void AddCollapsedGraphsFromGraphs(
        const TArray<UEdGraph*>& SourceGraphs,
        const FString& SourceGraphType,
        TArray<TSharedPtr<FJsonValue>>& OutGraphs,
        TSet<const UEdGraph*>& VisitedCollapsedGraphs)
    {
        for (UEdGraph* SourceGraph : SourceGraphs)
        {
            AddCollapsedGraphsFromGraph(SourceGraph, SourceGraphType, OutGraphs, VisitedCollapsedGraphs);
        }
    }

    class FInspectBlueprintGraphCommand final : public ISUnrealMcpCommand
    {
    public:
        virtual FString GetCommandName() const override
        {
            return TEXT("inspect_blueprint_graph");
        }

        virtual FSUnrealMcpResponse Execute(const FSUnrealMcpRequest& Request, const FSUnrealMcpExecutionContext& Context) override
        {
            static_cast<void>(Context);

            if (!Request.Params.IsValid())
            {
                return FSUnrealMcpResponse::MakeError(Request.RequestId, TEXT("INVALID_PARAMS"), TEXT("inspect_blueprint_graph requires a params object."));
            }

            FString BlueprintPath;
            FString GraphType;
            FString GraphName;
            bool bIncludePins = true;
            if (!Request.Params->TryGetStringField(TEXT("blueprint_path"), BlueprintPath) || BlueprintPath.IsEmpty())
            {
                return FSUnrealMcpResponse::MakeError(Request.RequestId, TEXT("INVALID_PARAMS"), TEXT("Missing blueprint_path."));
            }

            Request.Params->TryGetStringField(TEXT("graph_type"), GraphType);
            Request.Params->TryGetStringField(TEXT("graph_name"), GraphName);
            Request.Params->TryGetBoolField(TEXT("include_pins"), bIncludePins);

            UBlueprint* Blueprint = SUnrealMcpBlueprintCommandUtils::LoadBlueprintAsset(BlueprintPath);
            if (Blueprint == nullptr)
            {
                return FSUnrealMcpResponse::MakeError(Request.RequestId, TEXT("BLUEPRINT_NOT_FOUND"), FString::Printf(TEXT("Could not load blueprint '%s'."), *BlueprintPath));
            }

            UEdGraph* Graph = SUnrealMcpNodeCommandUtils::ResolveGraph(Blueprint, GraphType, GraphName);
            if (Graph == nullptr)
            {
                return FSUnrealMcpResponse::MakeError(
                    Request.RequestId,
                    TEXT("GRAPH_NOT_FOUND"),
                    FString::Printf(TEXT("Could not resolve graph '%s' of type '%s'."), *GraphName, GraphType.IsEmpty() ? TEXT("Event") : *GraphType));
            }

            TArray<TSharedPtr<FJsonValue>> Nodes;
            for (UEdGraphNode* Node : Graph->Nodes)
            {
                if (Node == nullptr)
                {
                    continue;
                }

                Nodes.Add(MakeShared<FJsonValueObject>(
                    bIncludePins
                        ? SUnrealMcpNodeCommandUtils::BuildNodeDetail(Node)
                        : SUnrealMcpNodeCommandUtils::BuildNodeSummary(Node)));
            }

            TSharedPtr<FJsonObject> Data = MakeShared<FJsonObject>();
            Data->SetObjectField(TEXT("blueprint"), SUnrealMcpBlueprintCommandUtils::BuildBlueprintSummary(Blueprint));
            Data->SetObjectField(TEXT("graph"), SUnrealMcpNodeCommandUtils::BuildGraphSummary(Graph, GraphType));
            Data->SetArrayField(TEXT("nodes"), Nodes);
            Data->SetNumberField(TEXT("count"), Nodes.Num());
            return FSUnrealMcpResponse::MakeSuccess(Request.RequestId, Data);
        }
    };

    const FSUnrealMcpCommandAutoRegistrar InspectBlueprintGraphCommandRegistrar([]()
    {
        return MakeShared<FInspectBlueprintGraphCommand>();
    });

    class FListBlueprintGraphsCommand final : public ISUnrealMcpCommand
    {
    public:
        virtual FString GetCommandName() const override
        {
            return TEXT("list_blueprint_graphs");
        }

        virtual FSUnrealMcpResponse Execute(const FSUnrealMcpRequest& Request, const FSUnrealMcpExecutionContext& Context) override
        {
            static_cast<void>(Context);

            if (!Request.Params.IsValid())
            {
                return FSUnrealMcpResponse::MakeError(Request.RequestId, TEXT("INVALID_PARAMS"), TEXT("list_blueprint_graphs requires a params object."));
            }

            FString BlueprintPath;
            bool bIncludeCollapsed = true;
            if (!Request.Params->TryGetStringField(TEXT("blueprint_path"), BlueprintPath) || BlueprintPath.IsEmpty())
            {
                return FSUnrealMcpResponse::MakeError(Request.RequestId, TEXT("INVALID_PARAMS"), TEXT("Missing blueprint_path."));
            }
            Request.Params->TryGetBoolField(TEXT("include_collapsed"), bIncludeCollapsed);

            UBlueprint* Blueprint = SUnrealMcpBlueprintCommandUtils::LoadBlueprintAsset(BlueprintPath);
            if (Blueprint == nullptr)
            {
                return FSUnrealMcpResponse::MakeError(Request.RequestId, TEXT("BLUEPRINT_NOT_FOUND"), FString::Printf(TEXT("Could not load blueprint '%s'."), *BlueprintPath));
            }

            TArray<TSharedPtr<FJsonValue>> Graphs;
            AddTopLevelGraphs(Blueprint->UbergraphPages, TEXT("Event"), Graphs);
            AddTopLevelGraphs(Blueprint->FunctionGraphs, TEXT("Function"), Graphs);
            AddTopLevelGraphs(Blueprint->MacroGraphs, TEXT("Macro"), Graphs);
            AddTopLevelGraphs(Blueprint->DelegateSignatureGraphs, TEXT("Delegate"), Graphs);

            if (bIncludeCollapsed)
            {
                TSet<const UEdGraph*> VisitedCollapsedGraphs;
                AddCollapsedGraphsFromGraphs(Blueprint->UbergraphPages, TEXT("Event"), Graphs, VisitedCollapsedGraphs);
                AddCollapsedGraphsFromGraphs(Blueprint->FunctionGraphs, TEXT("Function"), Graphs, VisitedCollapsedGraphs);
                AddCollapsedGraphsFromGraphs(Blueprint->MacroGraphs, TEXT("Macro"), Graphs, VisitedCollapsedGraphs);
                AddCollapsedGraphsFromGraphs(Blueprint->DelegateSignatureGraphs, TEXT("Delegate"), Graphs, VisitedCollapsedGraphs);
            }

            TSharedPtr<FJsonObject> Data = MakeShared<FJsonObject>();
            Data->SetObjectField(TEXT("blueprint"), SUnrealMcpBlueprintCommandUtils::BuildBlueprintSummary(Blueprint));
            Data->SetArrayField(TEXT("graphs"), Graphs);
            Data->SetNumberField(TEXT("count"), Graphs.Num());
            return FSUnrealMcpResponse::MakeSuccess(Request.RequestId, Data);
        }
    };

    const FSUnrealMcpCommandAutoRegistrar ListBlueprintGraphsCommandRegistrar([]()
    {
        return MakeShared<FListBlueprintGraphsCommand>();
    });
}

