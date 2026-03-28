#include "K2Node_ComponentBoundEvent.h"
#include "Commands/Extensions/UMG/UMGCommandUtils.h"
#include "EdGraph/EdGraph.h"
#include "EdGraphSchema_K2.h"
#include "K2Node_CallFunction.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "Mcp/ISUnrealMcpCommand.h"
#include "Mcp/SUnrealMcpCommandRegistry.h"

namespace
{
    static UEdGraphPin* FindFirstExecPin(UEdGraphNode* Node, EEdGraphPinDirection Direction)
    {
        if (Node == nullptr)
        {
            return nullptr;
        }

        for (UEdGraphPin* Pin : Node->Pins)
        {
            if (Pin != nullptr
                && Pin->Direction == Direction
                && Pin->PinType.PinCategory == UEdGraphSchema_K2::PC_Exec)
            {
                return Pin;
            }
        }

        return nullptr;
    }

    class FBindWidgetEventCommand final : public ISUnrealMcpCommand
    {
    public:
        virtual FString GetCommandName() const override
        {
            return TEXT("bind_widget_event");
        }

        virtual FSUnrealMcpResponse Execute(const FSUnrealMcpRequest& Request, const FSUnrealMcpExecutionContext& Context) override
        {
            static_cast<void>(Context);

            if (!Request.Params.IsValid())
            {
                return FSUnrealMcpResponse::MakeError(Request.RequestId, TEXT("INVALID_PARAMS"), TEXT("bind_widget_event requires a params object."));
            }

            FString WidgetPath;
            FString WidgetComponentName;
            FString EventName;
            FString FunctionName;
            if (!Request.Params->TryGetStringField(TEXT("widget_path"), WidgetPath) || WidgetPath.IsEmpty())
            {
                return FSUnrealMcpResponse::MakeError(Request.RequestId, TEXT("INVALID_PARAMS"), TEXT("Missing widget_path."));
            }
            if (!Request.Params->TryGetStringField(TEXT("widget_component_name"), WidgetComponentName) || WidgetComponentName.IsEmpty())
            {
                return FSUnrealMcpResponse::MakeError(Request.RequestId, TEXT("INVALID_PARAMS"), TEXT("Missing widget_component_name."));
            }
            if (!Request.Params->TryGetStringField(TEXT("event_name"), EventName) || EventName.IsEmpty())
            {
                return FSUnrealMcpResponse::MakeError(Request.RequestId, TEXT("INVALID_PARAMS"), TEXT("Missing event_name."));
            }
            Request.Params->TryGetStringField(TEXT("function_name"), FunctionName);
            if (FunctionName.IsEmpty())
            {
                FunctionName = FString::Printf(TEXT("%s_%s"), *WidgetComponentName, *EventName);
            }

            UWidgetBlueprint* WidgetBlueprint = SUnrealMcpUMGCommandUtils::LoadWidgetBlueprintAsset(WidgetPath);
            if (WidgetBlueprint == nullptr)
            {
                return FSUnrealMcpResponse::MakeError(Request.RequestId, TEXT("WIDGET_NOT_FOUND"), FString::Printf(TEXT("Could not load widget blueprint '%s'."), *WidgetPath));
            }

            UWidget* Widget = WidgetBlueprint->WidgetTree != nullptr ? WidgetBlueprint->WidgetTree->FindWidget(*WidgetComponentName) : nullptr;
            if (Widget == nullptr)
            {
                return FSUnrealMcpResponse::MakeError(Request.RequestId, TEXT("WIDGET_COMPONENT_NOT_FOUND"), FString::Printf(TEXT("Could not find widget component '%s'."), *WidgetComponentName));
            }

            FObjectProperty* ComponentProperty = SUnrealMcpUMGCommandUtils::FindWidgetVariableProperty(WidgetBlueprint, WidgetComponentName);
            if (ComponentProperty == nullptr)
            {
                return FSUnrealMcpResponse::MakeError(Request.RequestId, TEXT("WIDGET_VARIABLE_NOT_FOUND"), FString::Printf(TEXT("Could not resolve widget variable '%s'. Ensure the widget is marked as variable."), *WidgetComponentName));
            }

            UK2Node_ComponentBoundEvent* ExistingNode = const_cast<UK2Node_ComponentBoundEvent*>(
                FKismetEditorUtilities::FindBoundEventForComponent(WidgetBlueprint, FName(*EventName), ComponentProperty->GetFName()));
            if (ExistingNode == nullptr)
            {
                FKismetEditorUtilities::CreateNewBoundEventForClass(Widget->GetClass(), FName(*EventName), WidgetBlueprint, ComponentProperty);
                ExistingNode = const_cast<UK2Node_ComponentBoundEvent*>(
                    FKismetEditorUtilities::FindBoundEventForComponent(WidgetBlueprint, FName(*EventName), ComponentProperty->GetFName()));
            }

            if (ExistingNode == nullptr)
            {
                return FSUnrealMcpResponse::MakeError(Request.RequestId, TEXT("EVENT_BIND_FAILED"), TEXT("Failed to create bound event node for widget event."));
            }

            UEdGraph* FunctionGraph = SUnrealMcpUMGCommandUtils::EnsureFunctionGraph(WidgetBlueprint, FunctionName);
            if (FunctionGraph == nullptr)
            {
                return FSUnrealMcpResponse::MakeError(Request.RequestId, TEXT("FUNCTION_GRAPH_CREATE_FAILED"), TEXT("Failed to create or resolve target function graph."));
            }

            FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(WidgetBlueprint);
            FKismetEditorUtilities::CompileBlueprint(WidgetBlueprint);

            UFunction* TargetFunction = nullptr;
            if (WidgetBlueprint->SkeletonGeneratedClass != nullptr)
            {
                TargetFunction = WidgetBlueprint->SkeletonGeneratedClass->FindFunctionByName(*FunctionName);
            }
            if (TargetFunction == nullptr && WidgetBlueprint->GeneratedClass != nullptr)
            {
                TargetFunction = WidgetBlueprint->GeneratedClass->FindFunctionByName(*FunctionName);
            }
            if (TargetFunction == nullptr)
            {
                return FSUnrealMcpResponse::MakeError(Request.RequestId, TEXT("FUNCTION_NOT_FOUND"), FString::Printf(TEXT("Could not resolve widget function '%s' after creating the function graph."), *FunctionName));
            }

            UEdGraph* EventGraph = ExistingNode->GetGraph();
            if (EventGraph == nullptr)
            {
                return FSUnrealMcpResponse::MakeError(Request.RequestId, TEXT("EVENT_GRAPH_UNAVAILABLE"), TEXT("Bound event node is not attached to a valid event graph."));
            }

            UK2Node_CallFunction* FunctionCallNode = nullptr;
            for (UEdGraphNode* Node : EventGraph->Nodes)
            {
                UK2Node_CallFunction* Candidate = Cast<UK2Node_CallFunction>(Node);
                if (Candidate != nullptr && Candidate->GetFunctionName() == TargetFunction->GetFName())
                {
                    FunctionCallNode = Candidate;
                    break;
                }
            }

            const bool bCreatedFunctionCallNode = FunctionCallNode == nullptr;
            if (FunctionCallNode == nullptr)
            {
                FunctionCallNode = NewObject<UK2Node_CallFunction>(EventGraph);
                FunctionCallNode->SetFromFunction(TargetFunction);
                FunctionCallNode->NodePosX = ExistingNode->NodePosX + 320;
                FunctionCallNode->NodePosY = ExistingNode->NodePosY;
                EventGraph->AddNode(FunctionCallNode, true, false);
                FunctionCallNode->CreateNewGuid();
                FunctionCallNode->PostPlacedNewNode();
                FunctionCallNode->AllocateDefaultPins();
            }

            UEdGraphPin* EventExecPin = FindFirstExecPin(ExistingNode, EGPD_Output);
            UEdGraphPin* FunctionExecPin = FindFirstExecPin(FunctionCallNode, EGPD_Input);
            if (EventExecPin == nullptr || FunctionExecPin == nullptr)
            {
                return FSUnrealMcpResponse::MakeError(Request.RequestId, TEXT("EXEC_PIN_NOT_FOUND"), TEXT("Failed to resolve exec pins needed to bind the widget event to the target function."));
            }

            if (!EventExecPin->LinkedTo.Contains(FunctionExecPin))
            {
                const UEdGraphSchema* Schema = EventGraph->GetSchema();
                const bool bConnected = Schema != nullptr ? Schema->TryCreateConnection(EventExecPin, FunctionExecPin) : false;
                if (!bConnected)
                {
                    return FSUnrealMcpResponse::MakeError(Request.RequestId, TEXT("EVENT_BIND_CONNECT_FAILED"), TEXT("Failed to connect the widget event node to the target function call."));
                }
            }

            FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(WidgetBlueprint);
            FKismetEditorUtilities::CompileBlueprint(WidgetBlueprint);

            TSharedPtr<FJsonObject> Data = MakeShared<FJsonObject>();
            Data->SetObjectField(TEXT("widgetBlueprint"), SUnrealMcpUMGCommandUtils::BuildWidgetBlueprintSummary(WidgetBlueprint));
            Data->SetStringField(TEXT("widgetComponentName"), WidgetComponentName);
            Data->SetStringField(TEXT("eventName"), EventName);
            Data->SetStringField(TEXT("functionName"), FunctionName);
            Data->SetBoolField(TEXT("createdFunctionCallNode"), bCreatedFunctionCallNode);
            Data->SetStringField(TEXT("eventNodeId"), ExistingNode->NodeGuid.ToString());
            Data->SetStringField(TEXT("functionCallNodeId"), FunctionCallNode->NodeGuid.ToString());
            return FSUnrealMcpResponse::MakeSuccess(Request.RequestId, Data);
        }
    };

    const FSUnrealMcpCommandAutoRegistrar BindWidgetEventCommandRegistrar([]()
    {
        return MakeShared<FBindWidgetEventCommand>();
    });
}
