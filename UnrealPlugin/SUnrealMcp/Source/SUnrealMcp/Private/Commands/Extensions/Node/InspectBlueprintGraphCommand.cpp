#include "Commands/Extensions/Node/NodeCommandUtils.h"
#include "Mcp/ISUnrealMcpCommand.h"
#include "Mcp/SUnrealMcpCommandRegistry.h"

namespace
{
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
}
