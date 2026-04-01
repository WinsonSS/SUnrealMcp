#include "Commands/Shared/Node/NodeCommandUtils.h"
#include "Mcp/ISUnrealMcpCommand.h"
#include "Mcp/SUnrealMcpCommandRegistry.h"

namespace
{
    class FAddBlueprintFunctionNodeCommand final : public ISUnrealMcpCommand
    {
    public:
        virtual FString GetCommandName() const override
        {
            return TEXT("add_blueprint_function_node");
        }

        virtual FSUnrealMcpResponse Execute(const FSUnrealMcpRequest& Request, const FSUnrealMcpExecutionContext& Context) override
        {
            static_cast<void>(Context);

            if (!Request.Params.IsValid())
            {
                return FSUnrealMcpResponse::MakeError(Request.RequestId, TEXT("INVALID_PARAMS"), TEXT("add_blueprint_function_node requires a params object."));
            }

            FString BlueprintPath;
            FString Target;
            FString FunctionName;
            if (!Request.Params->TryGetStringField(TEXT("blueprint_path"), BlueprintPath) || BlueprintPath.IsEmpty())
            {
                return FSUnrealMcpResponse::MakeError(Request.RequestId, TEXT("INVALID_PARAMS"), TEXT("Missing blueprint_path."));
            }
            Request.Params->TryGetStringField(TEXT("target"), Target);
            if (!Request.Params->TryGetStringField(TEXT("function_name"), FunctionName) || FunctionName.IsEmpty())
            {
                return FSUnrealMcpResponse::MakeError(Request.RequestId, TEXT("INVALID_PARAMS"), TEXT("Missing function_name."));
            }

            UBlueprint* Blueprint = SUnrealMcpBlueprintCommandUtils::LoadBlueprintAsset(BlueprintPath);
            if (Blueprint == nullptr)
            {
                return FSUnrealMcpResponse::MakeError(Request.RequestId, TEXT("BLUEPRINT_NOT_FOUND"), FString::Printf(TEXT("Could not load blueprint '%s'."), *BlueprintPath));
            }

            UFunction* Function = SUnrealMcpNodeCommandUtils::ResolveFunction(Blueprint, Target, FunctionName);
            if (Function == nullptr)
            {
                return FSUnrealMcpResponse::MakeError(Request.RequestId, TEXT("FUNCTION_NOT_FOUND"), FString::Printf(TEXT("Could not resolve function '%s' for target '%s'."), *FunctionName, Target.IsEmpty() ? TEXT("self") : *Target));
            }

            UEdGraph* EventGraph = SUnrealMcpNodeCommandUtils::EnsureEventGraph(Blueprint);
            if (EventGraph == nullptr)
            {
                return FSUnrealMcpResponse::MakeError(Request.RequestId, TEXT("GRAPH_UNAVAILABLE"), TEXT("Could not access the blueprint event graph."));
            }

            FVector2D Position = FVector2D::ZeroVector;
            SUnrealMcpNodeCommandUtils::TryReadVector2(Request.Params, TEXT("node_position"), Position);

            UK2Node_CallFunction* FunctionNode = NewObject<UK2Node_CallFunction>(EventGraph);
            FunctionNode->SetFromFunction(Function);
            FunctionNode->NodePosX = FMath::RoundToInt(Position.X);
            FunctionNode->NodePosY = FMath::RoundToInt(Position.Y);
            EventGraph->AddNode(FunctionNode, true, false);
            FunctionNode->CreateNewGuid();
            FunctionNode->PostPlacedNewNode();
            FunctionNode->AllocateDefaultPins();

            const TSharedPtr<FJsonObject>* ParamObject = nullptr;
            if (Request.Params->TryGetObjectField(TEXT("params"), ParamObject) && ParamObject != nullptr)
            {
                for (const TPair<FString, TSharedPtr<FJsonValue>>& Pair : (*ParamObject)->Values)
                {
                    UEdGraphPin* Pin = SUnrealMcpNodeCommandUtils::FindPinByName(FunctionNode, Pair.Key, EGPD_Input);
                    if (Pin == nullptr)
                    {
                        return FSUnrealMcpResponse::MakeError(Request.RequestId, TEXT("PIN_NOT_FOUND"), FString::Printf(TEXT("Could not find function input pin '%s'."), *Pair.Key));
                    }

                    FString ApplyError;
                    if (!SUnrealMcpNodeCommandUtils::ApplyJsonValueToPinDefault(Pair.Value, Pin, ApplyError))
                    {
                        return FSUnrealMcpResponse::MakeError(Request.RequestId, TEXT("INVALID_PIN_VALUE"), FString::Printf(TEXT("Failed to apply value for pin '%s': %s"), *Pair.Key, *ApplyError));
                    }
                }
            }

            FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(Blueprint);

            TSharedPtr<FJsonObject> Data = MakeShared<FJsonObject>();
            Data->SetObjectField(TEXT("blueprint"), SUnrealMcpBlueprintCommandUtils::BuildBlueprintSummary(Blueprint));
            Data->SetObjectField(TEXT("node"), SUnrealMcpNodeCommandUtils::BuildNodeSummary(FunctionNode));
            Data->SetStringField(TEXT("target"), Target.IsEmpty() ? TEXT("self") : Target);
            Data->SetStringField(TEXT("resolvedFunctionOwner"), Function->GetOwnerClass() != nullptr ? Function->GetOwnerClass()->GetPathName() : TEXT(""));
            return FSUnrealMcpResponse::MakeSuccess(Request.RequestId, Data);
        }
    };

    const FSUnrealMcpCommandAutoRegistrar AddBlueprintFunctionNodeCommandRegistrar([]()
    {
        return MakeShared<FAddBlueprintFunctionNodeCommand>();
    });
}

