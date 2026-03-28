#include "Commands/Extensions/UMG/UMGCommandUtils.h"
#include "Blueprint/UserWidget.h"
#include "Editor.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Mcp/ISUnrealMcpCommand.h"
#include "Mcp/SUnrealMcpCommandRegistry.h"

namespace
{
    class FAddWidgetToViewportCommand final : public ISUnrealMcpCommand
    {
    public:
        virtual FString GetCommandName() const override
        {
            return TEXT("add_widget_to_viewport");
        }

        virtual FSUnrealMcpResponse Execute(const FSUnrealMcpRequest& Request, const FSUnrealMcpExecutionContext& Context) override
        {
            static_cast<void>(Context);

            if (!Request.Params.IsValid())
            {
                return FSUnrealMcpResponse::MakeError(Request.RequestId, TEXT("INVALID_PARAMS"), TEXT("add_widget_to_viewport requires a params object."));
            }

            FString WidgetPath;
            if (!Request.Params->TryGetStringField(TEXT("widget_path"), WidgetPath) || WidgetPath.IsEmpty())
            {
                return FSUnrealMcpResponse::MakeError(Request.RequestId, TEXT("INVALID_PARAMS"), TEXT("Missing widget_path."));
            }

            int32 ZOrder = 0;
            Request.Params->TryGetNumberField(TEXT("z_order"), ZOrder);

            UWidgetBlueprint* WidgetBlueprint = SUnrealMcpUMGCommandUtils::LoadWidgetBlueprintAsset(WidgetPath);
            if (WidgetBlueprint == nullptr)
            {
                return FSUnrealMcpResponse::MakeError(Request.RequestId, TEXT("WIDGET_NOT_FOUND"), FString::Printf(TEXT("Could not load widget blueprint '%s'."), *WidgetPath));
            }

            UClass* WidgetClass = WidgetBlueprint->GeneratedClass;
            if (WidgetClass == nullptr || !WidgetClass->IsChildOf(UUserWidget::StaticClass()))
            {
                return FSUnrealMcpResponse::MakeError(Request.RequestId, TEXT("INVALID_WIDGET_CLASS"), TEXT("Widget blueprint does not have a valid generated widget class."));
            }

            UWorld* RuntimeWorld = nullptr;
            if (GEditor != nullptr && GEditor->PlayWorld != nullptr)
            {
                RuntimeWorld = GEditor->PlayWorld;
            }

            if (RuntimeWorld == nullptr && GEngine != nullptr)
            {
                for (const FWorldContext& WorldContext : GEngine->GetWorldContexts())
                {
                    if ((WorldContext.WorldType == EWorldType::PIE || WorldContext.WorldType == EWorldType::Game)
                        && WorldContext.World() != nullptr)
                    {
                        RuntimeWorld = WorldContext.World();
                        break;
                    }
                }
            }

            if (RuntimeWorld == nullptr)
            {
                return FSUnrealMcpResponse::MakeError(
                    Request.RequestId,
                    TEXT("RUNTIME_WORLD_UNAVAILABLE"),
                    TEXT("No runtime game world is active. Start PIE or run the game before calling add_widget_to_viewport."));
            }

            UUserWidget* WidgetInstance = CreateWidget<UUserWidget>(RuntimeWorld, WidgetClass);
            if (WidgetInstance == nullptr)
            {
                return FSUnrealMcpResponse::MakeError(Request.RequestId, TEXT("WIDGET_INSTANCE_CREATE_FAILED"), TEXT("Failed to create widget instance for the active runtime world."));
            }

            WidgetInstance->AddToViewport(ZOrder);

            TSharedPtr<FJsonObject> Data = MakeShared<FJsonObject>();
            Data->SetObjectField(TEXT("widgetBlueprint"), SUnrealMcpUMGCommandUtils::BuildWidgetBlueprintSummary(WidgetBlueprint));
            Data->SetNumberField(TEXT("zOrder"), ZOrder);
            Data->SetStringField(TEXT("runtimeWorld"), RuntimeWorld->GetPathName());
            Data->SetStringField(TEXT("widgetInstanceClass"), WidgetClass->GetPathName());
            Data->SetStringField(TEXT("widgetInstanceName"), WidgetInstance->GetName());
            return FSUnrealMcpResponse::MakeSuccess(Request.RequestId, Data);
        }
    };

    const FSUnrealMcpCommandAutoRegistrar AddWidgetToViewportCommandRegistrar([]()
    {
        return MakeShared<FAddWidgetToViewportCommand>();
    });
}
