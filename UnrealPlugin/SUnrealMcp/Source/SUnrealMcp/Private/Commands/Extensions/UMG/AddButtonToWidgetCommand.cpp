#include "Commands/Extensions/UMG/UMGCommandUtils.h"
#include "Fonts/SlateFontInfo.h"
#include "Mcp/ISUnrealMcpCommand.h"
#include "Mcp/SUnrealMcpCommandRegistry.h"

namespace
{
    static FVector2D ReadVector2OrDefault(const TSharedPtr<FJsonObject>& Object, const FString& FieldName, const FVector2D& DefaultValue)
    {
        const TArray<TSharedPtr<FJsonValue>>* JsonArray = nullptr;
        if (!Object.IsValid() || !Object->TryGetArrayField(FieldName, JsonArray) || JsonArray->Num() != 2)
        {
            return DefaultValue;
        }

        return FVector2D((*JsonArray)[0]->AsNumber(), (*JsonArray)[1]->AsNumber());
    }

    static FLinearColor ReadColorOrDefault(const TSharedPtr<FJsonObject>& Object, const FString& FieldName, const FLinearColor& DefaultValue)
    {
        const TArray<TSharedPtr<FJsonValue>>* JsonArray = nullptr;
        if (!Object.IsValid() || !Object->TryGetArrayField(FieldName, JsonArray) || JsonArray->Num() != 4)
        {
            return DefaultValue;
        }

        return FLinearColor(
            (*JsonArray)[0]->AsNumber(),
            (*JsonArray)[1]->AsNumber(),
            (*JsonArray)[2]->AsNumber(),
            (*JsonArray)[3]->AsNumber());
    }

    class FAddButtonToWidgetCommand final : public ISUnrealMcpCommand
    {
    public:
        virtual FString GetCommandName() const override
        {
            return TEXT("add_button_to_widget");
        }

        virtual FSUnrealMcpResponse Execute(const FSUnrealMcpRequest& Request, const FSUnrealMcpExecutionContext& Context) override
        {
            static_cast<void>(Context);

            if (!Request.Params.IsValid())
            {
                return FSUnrealMcpResponse::MakeError(Request.RequestId, TEXT("INVALID_PARAMS"), TEXT("add_button_to_widget requires a params object."));
            }

            FString WidgetPath;
            FString ButtonName;
            FString TextValue;
            if (!Request.Params->TryGetStringField(TEXT("widget_path"), WidgetPath) || WidgetPath.IsEmpty())
            {
                return FSUnrealMcpResponse::MakeError(Request.RequestId, TEXT("INVALID_PARAMS"), TEXT("Missing widget_path."));
            }
            if (!Request.Params->TryGetStringField(TEXT("button_name"), ButtonName) || ButtonName.IsEmpty())
            {
                return FSUnrealMcpResponse::MakeError(Request.RequestId, TEXT("INVALID_PARAMS"), TEXT("Missing button_name."));
            }
            Request.Params->TryGetStringField(TEXT("text"), TextValue);

            UWidgetBlueprint* WidgetBlueprint = SUnrealMcpUMGCommandUtils::LoadWidgetBlueprintAsset(WidgetPath);
            if (WidgetBlueprint == nullptr)
            {
                return FSUnrealMcpResponse::MakeError(Request.RequestId, TEXT("WIDGET_NOT_FOUND"), FString::Printf(TEXT("Could not load widget blueprint '%s'."), *WidgetPath));
            }

            if (WidgetBlueprint->WidgetTree->FindWidget(*ButtonName) != nullptr)
            {
                return FSUnrealMcpResponse::MakeError(Request.RequestId, TEXT("WIDGET_ALREADY_EXISTS"), FString::Printf(TEXT("Widget '%s' already exists."), *ButtonName));
            }

            UCanvasPanel* RootCanvas = SUnrealMcpUMGCommandUtils::EnsureRootCanvasPanel(WidgetBlueprint);
            if (RootCanvas == nullptr)
            {
                return FSUnrealMcpResponse::MakeError(Request.RequestId, TEXT("ROOT_WIDGET_UNAVAILABLE"), TEXT("Widget blueprint does not have a valid root canvas."));
            }

            UButton* Button = WidgetBlueprint->WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), *ButtonName);
            if (Button == nullptr)
            {
                return FSUnrealMcpResponse::MakeError(Request.RequestId, TEXT("WIDGET_CREATE_FAILED"), TEXT("Failed to create Button widget."));
            }

            Button->bIsVariable = true;
            Button->SetBackgroundColor(ReadColorOrDefault(Request.Params, TEXT("background_color"), FLinearColor(0.1f, 0.1f, 0.1f, 1.0f)));

            UTextBlock* TextBlock = WidgetBlueprint->WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), *FString::Printf(TEXT("%s_Text"), *ButtonName));
            if (TextBlock != nullptr)
            {
                TextBlock->SetText(FText::FromString(TextValue));
                TextBlock->SetColorAndOpacity(FSlateColor(ReadColorOrDefault(Request.Params, TEXT("color"), FLinearColor::White)));

                int32 FontSize = 12;
                Request.Params->TryGetNumberField(TEXT("font_size"), FontSize);
                FSlateFontInfo FontInfo = TextBlock->GetFont();
                FontInfo.Size = FontSize;
                TextBlock->SetFont(FontInfo);
                Button->AddChild(TextBlock);
            }

            UCanvasPanelSlot* CanvasSlot = RootCanvas->AddChildToCanvas(Button);
            SUnrealMcpUMGCommandUtils::ApplyCanvasSlotLayout(
                CanvasSlot,
                ReadVector2OrDefault(Request.Params, TEXT("position"), FVector2D::ZeroVector),
                ReadVector2OrDefault(Request.Params, TEXT("size"), FVector2D(200.0f, 50.0f)));

            FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(WidgetBlueprint);
            FKismetEditorUtilities::CompileBlueprint(WidgetBlueprint);

            TSharedPtr<FJsonObject> WidgetData = MakeShared<FJsonObject>();
            WidgetData->SetStringField(TEXT("name"), ButtonName);
            WidgetData->SetStringField(TEXT("type"), Button->GetClass()->GetPathName());

            TSharedPtr<FJsonObject> Data = MakeShared<FJsonObject>();
            Data->SetObjectField(TEXT("widgetBlueprint"), SUnrealMcpUMGCommandUtils::BuildWidgetBlueprintSummary(WidgetBlueprint));
            Data->SetObjectField(TEXT("widget"), WidgetData);
            return FSUnrealMcpResponse::MakeSuccess(Request.RequestId, Data);
        }
    };

    const FSUnrealMcpCommandAutoRegistrar AddButtonToWidgetCommandRegistrar([]()
    {
        return MakeShared<FAddButtonToWidgetCommand>();
    });
}
