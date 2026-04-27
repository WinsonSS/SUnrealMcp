#include "Commands/Shared/Blueprint/BlueprintCommandUtils.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "Mcp/ISUnrealMcpCommand.h"
#include "Mcp/SUnrealMcpCommandRegistry.h"

namespace
{
    class FSetBlueprintPropertyCommand final : public ISUnrealMcpCommand
    {
    public:
        virtual FString GetCommandName() const override
        {
            return TEXT("set_blueprint_property");
        }

        virtual FSUnrealMcpResponse Execute(const FSUnrealMcpRequest& Request, const FSUnrealMcpExecutionContext& Context) override
        {
            static_cast<void>(Context);

            if (!Request.Params.IsValid())
            {
                return FSUnrealMcpResponse::MakeError(Request.RequestId, TEXT("INVALID_PARAMS"), TEXT("set_blueprint_property requires a params object."));
            }

            FString BlueprintPath;
            FString PropertyName;
            if (!Request.Params->TryGetStringField(TEXT("blueprint_path"), BlueprintPath) || BlueprintPath.IsEmpty())
            {
                return FSUnrealMcpResponse::MakeError(Request.RequestId, TEXT("INVALID_PARAMS"), TEXT("Missing blueprint_path."));
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

            UObject* DefaultObject = Blueprint->GeneratedClass != nullptr ? Blueprint->GeneratedClass->GetDefaultObject() : nullptr;
            if (DefaultObject == nullptr)
            {
                return FSUnrealMcpResponse::MakeError(Request.RequestId, TEXT("DEFAULT_OBJECT_UNAVAILABLE"), TEXT("Blueprint does not have a generated default object."));
            }

            FProperty* Property = SUnrealMcpEditorCommandUtils::FindObjectProperty(DefaultObject, PropertyName);
            if (!SUnrealMcpEditorCommandUtils::IsEditableObjectProperty(Property))
            {
                return FSUnrealMcpResponse::MakeError(Request.RequestId, TEXT("PROPERTY_NOT_FOUND"), FString::Printf(TEXT("Could not find supported property '%s'."), *PropertyName));
            }

            FString ApplyError;
            DefaultObject->Modify();
            if (!SUnrealMcpEditorCommandUtils::ApplyJsonValueToProperty(PropertyValue, Property, DefaultObject, ApplyError))
            {
                return FSUnrealMcpResponse::MakeError(Request.RequestId, TEXT("INVALID_PROPERTY_VALUE"), ApplyError);
            }

            DefaultObject->PostEditChange();
            FBlueprintEditorUtils::MarkBlueprintAsModified(Blueprint);
            FString CompileError;
            if (!SUnrealMcpBlueprintCommandUtils::CompileBlueprintAndGetError(Blueprint, CompileError))
            {
                return FSUnrealMcpResponse::MakeError(
                    Request.RequestId,
                    TEXT("BLUEPRINT_COMPILE_FAILED"),
                    CompileError);
            }

            TSharedPtr<FJsonObject> Data = MakeShared<FJsonObject>();
            Data->SetObjectField(TEXT("blueprint"), SUnrealMcpBlueprintCommandUtils::BuildBlueprintSummary(Blueprint));
            Data->SetStringField(TEXT("propertyName"), Property->GetName());
            Data->SetField(TEXT("propertyValue"), SUnrealMcpEditorCommandUtils::ExportPropertyValueToJson(Property, DefaultObject));
            return FSUnrealMcpResponse::MakeSuccess(Request.RequestId, Data);
        }
    };

    const FSUnrealMcpCommandAutoRegistrar SetBlueprintPropertyCommandRegistrar([]()
    {
        return MakeShared<FSetBlueprintPropertyCommand>();
    });
}

