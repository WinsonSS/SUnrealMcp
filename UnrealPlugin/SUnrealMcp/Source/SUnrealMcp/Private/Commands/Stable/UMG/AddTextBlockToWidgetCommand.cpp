#include "Commands/Shared/UMG/UMGCommandUtils.h"
#include "Fonts/SlateFontInfo.h"
#include "Mcp/ISUnrealMcpCommand.h"
#include "Mcp/SUnrealMcpCommandRegistry.h"

namespace
{
    class FAddTextBlockToWidgetCommand final : public ISUnrealMcpCommand
    {
    public:
        virtual FString GetCommandName() const override
        {
            return TEXT("add_text_block_to_widget");
        }

        virtual FSUnrealMcpResponse Execute(const FSUnrealMcpRequest& Request, const FSUnrealMcpExecutionContext& Context) override
        {
            static_cast<void>(Context);

            if (!Request.Params.IsValid())
            {
                return FSUnrealMcpResponse::MakeError(Request.RequestId, TEXT("INVALID_PARAMS"), TEXT("add_text_block_to_widget requires a params object."));
            }

            FString WidgetPath;
            FString TextBlockName;
            FString TextValue;
            if (!Request.Params->TryGetStringField(TEXT("widget_path"), WidgetPath) || WidgetPath.IsEmpty())
            {
                return FSUnrealMcpResponse::MakeError(Request.RequestId, TEXT("INVALID_PARAMS"), TEXT("Missing widget_path."));
            }
            if (!Request.Params->TryGetStringField(TEXT("text_block_name"), TextBlockName) || TextBlockName.IsEmpty())
            {
                return FSUnrealMcpResponse::MakeError(Request.RequestId, TEXT("INVALID_PARAMS"), TEXT("Missing text_block_name."));
            }
            Request.Params->TryGetStringField(TEXT("text"), TextValue);

            UWidgetBlueprint* WidgetBlueprint = SUnrealMcpUMGCommandUtils::LoadWidgetBlueprintAsset(WidgetPath);
            if (WidgetBlueprint == nullptr)
            {
                return FSUnrealMcpResponse::MakeError(Request.RequestId, TEXT("WIDGET_NOT_FOUND"), FString::Printf(TEXT("Could not load widget blueprint '%s'."), *WidgetPath));
            }

            if (WidgetBlueprint->WidgetTree->FindWidget(*TextBlockName) != nullptr)
            {
                return FSUnrealMcpResponse::MakeError(Request.RequestId, TEXT("WIDGET_ALREADY_EXISTS"), FString::Printf(TEXT("Widget '%s' already exists."), *TextBlockName));
            }

            UCanvasPanel* RootCanvas = SUnrealMcpUMGCommandUtils::EnsureRootCanvasPanel(WidgetBlueprint);
            if (RootCanvas == nullptr)
            {
                return FSUnrealMcpResponse::MakeError(Request.RequestId, TEXT("ROOT_WIDGET_UNAVAILABLE"), TEXT("Widget blueprint does not have a valid root canvas."));
            }

            UTextBlock* TextBlock = WidgetBlueprint->WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), *TextBlockName);
            if (TextBlock == nullptr)
            {
                return FSUnrealMcpResponse::MakeError(Request.RequestId, TEXT("WIDGET_CREATE_FAILED"), TEXT("Failed to create TextBlock widget."));
            }

            TextBlock->bIsVariable = true;
            TextBlock->SetText(FText::FromString(TextValue));
            TextBlock->SetColorAndOpacity(FSlateColor(ReadColorOrDefault(Request.Params, TEXT("color"), FLinearColor::White)));

            int32 FontSize = 12;
            Request.Params->TryGetNumberField(TEXT("font_size"), FontSize);
            FSlateFontInfo FontInfo = TextBlock->GetFont();
            FontInfo.Size = FontSize;
            TextBlock->SetFont(FontInfo);

            UCanvasPanelSlot* CanvasSlot = RootCanvas->AddChildToCanvas(TextBlock);
            SUnrealMcpUMGCommandUtils::ApplyCanvasSlotLayout(
                CanvasSlot,
                ReadVector2OrDefault(Request.Params, TEXT("position"), FVector2D::ZeroVector),
                ReadVector2OrDefault(Request.Params, TEXT("size"), FVector2D(200.0f, 50.0f)));

            FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(WidgetBlueprint);
            FKismetEditorUtilities::CompileBlueprint(WidgetBlueprint);

            TSharedPtr<FJsonObject> WidgetData = MakeShared<FJsonObject>();
            WidgetData->SetStringField(TEXT("name"), TextBlockName);
            WidgetData->SetStringField(TEXT("type"), TextBlock->GetClass()->GetPathName());

            TSharedPtr<FJsonObject> Data = MakeShared<FJsonObject>();
            Data->SetObjectField(TEXT("widgetBlueprint"), SUnrealMcpUMGCommandUtils::BuildWidgetBlueprintSummary(WidgetBlueprint));
            Data->SetObjectField(TEXT("widget"), WidgetData);
            return FSUnrealMcpResponse::MakeSuccess(Request.RequestId, Data);
        }
    private:
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

    };

    const FSUnrealMcpCommandAutoRegistrar AddTextBlockToWidgetCommandRegistrar([]()
    {
        return MakeShared<FAddTextBlockToWidgetCommand>();
    });
}

