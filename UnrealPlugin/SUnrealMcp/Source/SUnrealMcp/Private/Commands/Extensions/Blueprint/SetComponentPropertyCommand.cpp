#include "Commands/Extensions/Blueprint/BlueprintCommandUtils.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "Mcp/ISUnrealMcpCommand.h"
#include "Mcp/SUnrealMcpCommandRegistry.h"

namespace
{
    class FSetComponentPropertyCommand final : public ISUnrealMcpCommand
    {
    public:
        virtual FString GetCommandName() const override
        {
            return TEXT("set_component_property");
        }

        virtual FSUnrealMcpResponse Execute(const FSUnrealMcpRequest& Request, const FSUnrealMcpExecutionContext& Context) override
        {
            static_cast<void>(Context);

            if (!Request.Params.IsValid())
            {
                return FSUnrealMcpResponse::MakeError(Request.RequestId, TEXT("INVALID_PARAMS"), TEXT("set_component_property requires a params object."));
            }

            FString BlueprintPath;
            FString ComponentName;
            FString PropertyName;
            if (!Request.Params->TryGetStringField(TEXT("blueprint_path"), BlueprintPath) || BlueprintPath.IsEmpty())
            {
                return FSUnrealMcpResponse::MakeError(Request.RequestId, TEXT("INVALID_PARAMS"), TEXT("Missing blueprint_path."));
            }
            if (!Request.Params->TryGetStringField(TEXT("component_name"), ComponentName) || ComponentName.IsEmpty())
            {
                return FSUnrealMcpResponse::MakeError(Request.RequestId, TEXT("INVALID_PARAMS"), TEXT("Missing component_name."));
            }
            if (!Request.Params->TryGetStringField(TEXT("property_name"), PropertyName) || PropertyName.IsEmpty())
            {
                return FSUnrealMcpResponse::MakeError(Request.RequestId, TEXT("INVALID_PARAMS"), TEXT("Missing property_name."));
            }

            TSharedPtr<FJsonValue> PropertyValue = Request.Params->TryGetField(TEXT("property_value"));
            if (!PropertyValue.IsValid())
            {
                return FSUnrealMcpResponse::MakeError(Request.RequestId, TEXT("INVALID_PARAMS"), TEXT("Missing property_value."));
            }

            UBlueprint* Blueprint = SUnrealMcpBlueprintCommandUtils::LoadBlueprintAsset(BlueprintPath);
            if (Blueprint == nullptr)
            {
                return FSUnrealMcpResponse::MakeError(Request.RequestId, TEXT("BLUEPRINT_NOT_FOUND"), FString::Printf(TEXT("Could not load blueprint '%s'."), *BlueprintPath));
            }

            USCS_Node* ComponentNode = SUnrealMcpBlueprintCommandUtils::FindBlueprintComponentNode(Blueprint, ComponentName);
            if (ComponentNode == nullptr || ComponentNode->ComponentTemplate == nullptr)
            {
                return FSUnrealMcpResponse::MakeError(Request.RequestId, TEXT("COMPONENT_NOT_FOUND"), FString::Printf(TEXT("Could not find component '%s'."), *ComponentName));
            }

            FProperty* Property = SUnrealMcpEditorCommandUtils::FindObjectProperty(ComponentNode->ComponentTemplate, PropertyName);
            if (!SUnrealMcpEditorCommandUtils::IsEditableObjectProperty(Property))
            {
                return FSUnrealMcpResponse::MakeError(Request.RequestId, TEXT("PROPERTY_NOT_FOUND"), FString::Printf(TEXT("Could not find supported property '%s'."), *PropertyName));
            }

            FString ApplyError;
            ComponentNode->ComponentTemplate->Modify();
            if (!SUnrealMcpEditorCommandUtils::ApplyJsonValueToProperty(PropertyValue, Property, ComponentNode->ComponentTemplate, ApplyError))
            {
                return FSUnrealMcpResponse::MakeError(Request.RequestId, TEXT("INVALID_PROPERTY_VALUE"), ApplyError);
            }

            ComponentNode->ComponentTemplate->PostEditChange();
            FBlueprintEditorUtils::MarkBlueprintAsModified(Blueprint);
            FKismetEditorUtilities::CompileBlueprint(Blueprint);

            TSharedPtr<FJsonObject> Data = MakeShared<FJsonObject>();
            Data->SetObjectField(TEXT("blueprint"), SUnrealMcpBlueprintCommandUtils::BuildBlueprintSummary(Blueprint));
            Data->SetStringField(TEXT("componentName"), ComponentNode->GetVariableName().ToString());
            Data->SetStringField(TEXT("propertyName"), Property->GetName());
            Data->SetField(TEXT("propertyValue"), SUnrealMcpEditorCommandUtils::ExportPropertyValueToJson(Property, ComponentNode->ComponentTemplate));
            return FSUnrealMcpResponse::MakeSuccess(Request.RequestId, Data);
        }
    };

    const FSUnrealMcpCommandAutoRegistrar SetComponentPropertyCommandRegistrar([]()
    {
        return MakeShared<FSetComponentPropertyCommand>();
    });
}
