#include "Commands/Shared/Node/NodeCommandUtils.h"
#include "Mcp/ISUnrealMcpCommand.h"
#include "Mcp/SUnrealMcpCommandRegistry.h"

namespace
{
    class FAddBlueprintEventNodeCommand final : public ISUnrealMcpCommand
    {
    public:
        virtual FString GetCommandName() const override
        {
            return TEXT("add_blueprint_event_node");
        }

        virtual FSUnrealMcpResponse Execute(const FSUnrealMcpRequest& Request, const FSUnrealMcpExecutionContext& Context) override
        {
            static_cast<void>(Context);

            if (!Request.Params.IsValid())
            {
                return FSUnrealMcpResponse::MakeError(Request.RequestId, TEXT("INVALID_PARAMS"), TEXT("add_blueprint_event_node requires a params object."));
            }

            FString BlueprintPath;
            FString EventName;
            if (!Request.Params->TryGetStringField(TEXT("blueprint_path"), BlueprintPath) || BlueprintPath.IsEmpty())
            {
                return FSUnrealMcpResponse::MakeError(Request.RequestId, TEXT("INVALID_PARAMS"), TEXT("Missing blueprint_path."));
            }
            if (!Request.Params->TryGetStringField(TEXT("event_name"), EventName) || EventName.IsEmpty())
            {
                return FSUnrealMcpResponse::MakeError(Request.RequestId, TEXT("INVALID_PARAMS"), TEXT("Missing event_name."));
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

            for (UEdGraphNode* ExistingNode : EventGraph->Nodes)
            {
                if (const UK2Node_Event* EventNode = Cast<UK2Node_Event>(ExistingNode))
                {
                    if (EventNode->EventReference.GetMemberName().ToString().Equals(EventName, ESearchCase::IgnoreCase))
                    {
                        TSharedPtr<FJsonObject> Data = MakeShared<FJsonObject>();
                        Data->SetObjectField(TEXT("blueprint"), SUnrealMcpBlueprintCommandUtils::BuildBlueprintSummary(Blueprint));
                        Data->SetObjectField(TEXT("node"), SUnrealMcpNodeCommandUtils::BuildNodeSummary(ExistingNode));
                        Data->SetBoolField(TEXT("alreadyExisted"), true);
                        return FSUnrealMcpResponse::MakeSuccess(Request.RequestId, Data);
                    }
                }
            }

            UFunction* EventFunction = SUnrealMcpNodeCommandUtils::ResolveFunction(Blueprint, TEXT("self"), EventName);
            if (EventFunction == nullptr)
            {
                return FSUnrealMcpResponse::MakeError(Request.RequestId, TEXT("EVENT_NOT_FOUND"), FString::Printf(TEXT("Could not resolve event '%s' on blueprint '%s'."), *EventName, *BlueprintPath));
            }

            FVector2D Position = FVector2D::ZeroVector;
            SUnrealMcpNodeCommandUtils::TryReadVector2(Request.Params, TEXT("node_position"), Position);

            UK2Node_Event* EventNode = NewObject<UK2Node_Event>(EventGraph);
            EventNode->EventReference.SetExternalMember(EventFunction->GetFName(), EventFunction->GetOwnerClass());
            EventNode->NodePosX = FMath::RoundToInt(Position.X);
            EventNode->NodePosY = FMath::RoundToInt(Position.Y);
            EventGraph->AddNode(EventNode, true, false);
            EventNode->CreateNewGuid();
            EventNode->PostPlacedNewNode();
            EventNode->AllocateDefaultPins();

            FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(Blueprint);

            TSharedPtr<FJsonObject> Data = MakeShared<FJsonObject>();
            Data->SetObjectField(TEXT("blueprint"), SUnrealMcpBlueprintCommandUtils::BuildBlueprintSummary(Blueprint));
            Data->SetObjectField(TEXT("node"), SUnrealMcpNodeCommandUtils::BuildNodeSummary(EventNode));
            Data->SetBoolField(TEXT("alreadyExisted"), false);
            return FSUnrealMcpResponse::MakeSuccess(Request.RequestId, Data);
        }
    };

    const FSUnrealMcpCommandAutoRegistrar AddBlueprintEventNodeCommandRegistrar([]()
    {
        return MakeShared<FAddBlueprintEventNodeCommand>();
    });
}

