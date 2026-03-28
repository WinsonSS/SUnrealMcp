#include "Commands/Extensions/Node/NodeCommandUtils.h"
#include "EdGraphSchema_K2.h"
#include "Mcp/ISUnrealMcpCommand.h"
#include "Mcp/SUnrealMcpCommandRegistry.h"

namespace
{
    class FAddBlueprintVariableCommand final : public ISUnrealMcpCommand
    {
    public:
        virtual FString GetCommandName() const override
        {
            return TEXT("add_blueprint_variable");
        }

        virtual FSUnrealMcpResponse Execute(const FSUnrealMcpRequest& Request, const FSUnrealMcpExecutionContext& Context) override
        {
            static_cast<void>(Context);

            if (!Request.Params.IsValid())
            {
                return FSUnrealMcpResponse::MakeError(Request.RequestId, TEXT("INVALID_PARAMS"), TEXT("add_blueprint_variable requires a params object."));
            }

            FString BlueprintPath;
            FString VariableName;
            FString VariableType;
            bool bIsExposed = false;
            if (!Request.Params->TryGetStringField(TEXT("blueprint_path"), BlueprintPath) || BlueprintPath.IsEmpty())
            {
                return FSUnrealMcpResponse::MakeError(Request.RequestId, TEXT("INVALID_PARAMS"), TEXT("Missing blueprint_path."));
            }
            if (!Request.Params->TryGetStringField(TEXT("variable_name"), VariableName) || VariableName.IsEmpty())
            {
                return FSUnrealMcpResponse::MakeError(Request.RequestId, TEXT("INVALID_PARAMS"), TEXT("Missing variable_name."));
            }
            if (!Request.Params->TryGetStringField(TEXT("variable_type"), VariableType) || VariableType.IsEmpty())
            {
                return FSUnrealMcpResponse::MakeError(Request.RequestId, TEXT("INVALID_PARAMS"), TEXT("Missing variable_type."));
            }
            Request.Params->TryGetBoolField(TEXT("is_exposed"), bIsExposed);

            UBlueprint* Blueprint = SUnrealMcpBlueprintCommandUtils::LoadBlueprintAsset(BlueprintPath);
            if (Blueprint == nullptr)
            {
                return FSUnrealMcpResponse::MakeError(Request.RequestId, TEXT("BLUEPRINT_NOT_FOUND"), FString::Printf(TEXT("Could not load blueprint '%s'."), *BlueprintPath));
            }

            for (const FBPVariableDescription& Variable : Blueprint->NewVariables)
            {
                if (Variable.VarName.ToString().Equals(VariableName, ESearchCase::IgnoreCase))
                {
                    return FSUnrealMcpResponse::MakeError(Request.RequestId, TEXT("VARIABLE_ALREADY_EXISTS"), FString::Printf(TEXT("Blueprint variable '%s' already exists."), *VariableName));
                }
            }

            FEdGraphPinType PinType;
            if (VariableType.Equals(TEXT("Boolean"), ESearchCase::IgnoreCase))
            {
                PinType.PinCategory = UEdGraphSchema_K2::PC_Boolean;
            }
            else if (VariableType.Equals(TEXT("Integer"), ESearchCase::IgnoreCase))
            {
                PinType.PinCategory = UEdGraphSchema_K2::PC_Int;
            }
            else if (VariableType.Equals(TEXT("Float"), ESearchCase::IgnoreCase))
            {
                PinType.PinCategory = UEdGraphSchema_K2::PC_Real;
                PinType.PinSubCategory = UEdGraphSchema_K2::PC_Float;
            }
            else if (VariableType.Equals(TEXT("Vector"), ESearchCase::IgnoreCase))
            {
                PinType.PinCategory = UEdGraphSchema_K2::PC_Struct;
                PinType.PinSubCategoryObject = TBaseStructure<FVector>::Get();
            }
            else if (VariableType.Equals(TEXT("Rotator"), ESearchCase::IgnoreCase))
            {
                PinType.PinCategory = UEdGraphSchema_K2::PC_Struct;
                PinType.PinSubCategoryObject = TBaseStructure<FRotator>::Get();
            }
            else if (VariableType.Equals(TEXT("String"), ESearchCase::IgnoreCase))
            {
                PinType.PinCategory = UEdGraphSchema_K2::PC_String;
            }
            else if (VariableType.Equals(TEXT("Name"), ESearchCase::IgnoreCase))
            {
                PinType.PinCategory = UEdGraphSchema_K2::PC_Name;
            }
            else if (VariableType.Equals(TEXT("Text"), ESearchCase::IgnoreCase))
            {
                PinType.PinCategory = UEdGraphSchema_K2::PC_Text;
            }
            else
            {
                return FSUnrealMcpResponse::MakeError(Request.RequestId, TEXT("UNSUPPORTED_VARIABLE_TYPE"), FString::Printf(TEXT("Unsupported variable type '%s'."), *VariableType));
            }

            FBlueprintEditorUtils::AddMemberVariable(Blueprint, FName(*VariableName), PinType);

            for (FBPVariableDescription& Variable : Blueprint->NewVariables)
            {
                if (Variable.VarName.ToString().Equals(VariableName, ESearchCase::IgnoreCase))
                {
                    if (bIsExposed)
                    {
                        Variable.PropertyFlags |= CPF_Edit | CPF_BlueprintVisible;
                    }
                    break;
                }
            }

            FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(Blueprint);

            TSharedPtr<FJsonObject> VariableData = MakeShared<FJsonObject>();
            VariableData->SetStringField(TEXT("name"), VariableName);
            VariableData->SetStringField(TEXT("type"), VariableType);
            VariableData->SetBoolField(TEXT("isExposed"), bIsExposed);

            TSharedPtr<FJsonObject> Data = MakeShared<FJsonObject>();
            Data->SetObjectField(TEXT("blueprint"), SUnrealMcpBlueprintCommandUtils::BuildBlueprintSummary(Blueprint));
            Data->SetObjectField(TEXT("variable"), VariableData);
            return FSUnrealMcpResponse::MakeSuccess(Request.RequestId, Data);
        }
    };

    const FSUnrealMcpCommandAutoRegistrar AddBlueprintVariableCommandRegistrar([]()
    {
        return MakeShared<FAddBlueprintVariableCommand>();
    });
}
