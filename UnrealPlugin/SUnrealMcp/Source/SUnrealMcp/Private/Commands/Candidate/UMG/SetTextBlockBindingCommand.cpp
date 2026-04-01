#include "Commands/Shared/UMG/UMGCommandUtils.h"
#include "Mcp/ISUnrealMcpCommand.h"
#include "Mcp/SUnrealMcpCommandRegistry.h"

namespace
{
    class FSetTextBlockBindingCommand final : public ISUnrealMcpCommand
    {
    public:
        virtual FString GetCommandName() const override
        {
            return TEXT("set_text_block_binding");
        }

        virtual FSUnrealMcpResponse Execute(const FSUnrealMcpRequest& Request, const FSUnrealMcpExecutionContext& Context) override
        {
            static_cast<void>(Context);

            if (!Request.Params.IsValid())
            {
                return FSUnrealMcpResponse::MakeError(Request.RequestId, TEXT("INVALID_PARAMS"), TEXT("set_text_block_binding requires a params object."));
            }

            FString WidgetPath;
            FString TextBlockName;
            FString BindingProperty;
            FString BindingType;
            if (!Request.Params->TryGetStringField(TEXT("widget_path"), WidgetPath) || WidgetPath.IsEmpty())
            {
                return FSUnrealMcpResponse::MakeError(Request.RequestId, TEXT("INVALID_PARAMS"), TEXT("Missing widget_path."));
            }
            if (!Request.Params->TryGetStringField(TEXT("text_block_name"), TextBlockName) || TextBlockName.IsEmpty())
            {
                return FSUnrealMcpResponse::MakeError(Request.RequestId, TEXT("INVALID_PARAMS"), TEXT("Missing text_block_name."));
            }
            if (!Request.Params->TryGetStringField(TEXT("binding_property"), BindingProperty) || BindingProperty.IsEmpty())
            {
                return FSUnrealMcpResponse::MakeError(Request.RequestId, TEXT("INVALID_PARAMS"), TEXT("Missing binding_property."));
            }
            Request.Params->TryGetStringField(TEXT("binding_type"), BindingType);
            if (BindingType.IsEmpty())
            {
                BindingType = TEXT("Text");
            }

            UWidgetBlueprint* WidgetBlueprint = SUnrealMcpUMGCommandUtils::LoadWidgetBlueprintAsset(WidgetPath);
            if (WidgetBlueprint == nullptr)
            {
                return FSUnrealMcpResponse::MakeError(Request.RequestId, TEXT("WIDGET_NOT_FOUND"), FString::Printf(TEXT("Could not load widget blueprint '%s'."), *WidgetPath));
            }

            UTextBlock* TextBlock = WidgetBlueprint->WidgetTree != nullptr ? Cast<UTextBlock>(WidgetBlueprint->WidgetTree->FindWidget(*TextBlockName)) : nullptr;
            if (TextBlock == nullptr)
            {
                return FSUnrealMcpResponse::MakeError(Request.RequestId, TEXT("TEXT_BLOCK_NOT_FOUND"), FString::Printf(TEXT("Could not find TextBlock '%s'."), *TextBlockName));
            }

            if (WidgetBlueprint->SkeletonGeneratedClass == nullptr || FindFProperty<FProperty>(WidgetBlueprint->SkeletonGeneratedClass, *BindingProperty) == nullptr)
            {
                return FSUnrealMcpResponse::MakeError(Request.RequestId, TEXT("BINDING_SOURCE_NOT_FOUND"), FString::Printf(TEXT("Could not find binding source property or function '%s' on widget blueprint."), *BindingProperty));
            }

            FDelegateEditorBinding Binding;
            Binding.ObjectName = TextBlock->GetName();
            Binding.PropertyName = SUnrealMcpUMGCommandUtils::GetBindingTargetPropertyName(BindingType);
            Binding.SourceProperty = FName(*BindingProperty);
            Binding.Kind = EBindingKind::Property;
            Binding.SourcePath = FEditorPropertyPath(TArray<FFieldVariant>{ FindFProperty<FProperty>(WidgetBlueprint->SkeletonGeneratedClass, *BindingProperty) });

            WidgetBlueprint->Modify();
            WidgetBlueprint->Bindings.Remove(Binding);
            WidgetBlueprint->Bindings.AddUnique(Binding);
            FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(WidgetBlueprint);
            FKismetEditorUtilities::CompileBlueprint(WidgetBlueprint);

            TSharedPtr<FJsonObject> Data = MakeShared<FJsonObject>();
            Data->SetObjectField(TEXT("widgetBlueprint"), SUnrealMcpUMGCommandUtils::BuildWidgetBlueprintSummary(WidgetBlueprint));
            Data->SetStringField(TEXT("textBlockName"), TextBlockName);
            Data->SetStringField(TEXT("bindingProperty"), BindingProperty);
            Data->SetStringField(TEXT("bindingType"), BindingType);
            Data->SetStringField(TEXT("targetProperty"), Binding.PropertyName.ToString());
            return FSUnrealMcpResponse::MakeSuccess(Request.RequestId, Data);
        }
    };

    const FSUnrealMcpCommandAutoRegistrar SetTextBlockBindingCommandRegistrar([]()
    {
        return MakeShared<FSetTextBlockBindingCommand>();
    });
}

