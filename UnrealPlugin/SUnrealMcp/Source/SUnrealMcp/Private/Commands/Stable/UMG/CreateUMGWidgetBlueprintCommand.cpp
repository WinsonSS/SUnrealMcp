#include "AssetRegistry/AssetRegistryModule.h"
#include "Commands/Shared/UMG/UMGCommandUtils.h"
#include "Mcp/ISUnrealMcpCommand.h"
#include "Mcp/SUnrealMcpCommandRegistry.h"
#include "UObject/Package.h"

namespace
{
    class FCreateUMGWidgetBlueprintCommand final : public ISUnrealMcpCommand
    {
    public:
        virtual FString GetCommandName() const override
        {
            return TEXT("create_umg_widget_blueprint");
        }

        virtual FSUnrealMcpResponse Execute(const FSUnrealMcpRequest& Request, const FSUnrealMcpExecutionContext& Context) override
        {
            static_cast<void>(Context);

            if (!Request.Params.IsValid())
            {
                return FSUnrealMcpResponse::MakeError(Request.RequestId, TEXT("INVALID_PARAMS"), TEXT("create_umg_widget_blueprint requires a params object."));
            }

            FString WidgetPath;
            FString ParentClassReference;
            if (!Request.Params->TryGetStringField(TEXT("widget_path"), WidgetPath) || WidgetPath.IsEmpty())
            {
                return FSUnrealMcpResponse::MakeError(Request.RequestId, TEXT("INVALID_PARAMS"), TEXT("Missing widget_path."));
            }

            Request.Params->TryGetStringField(TEXT("parent_class"), ParentClassReference);
            if (ParentClassReference.IsEmpty())
            {
                ParentClassReference = TEXT("UserWidget");
            }

            FString PackagePath;
            FString AssetName;
            FString PackageName;
            FString ObjectPath;
            if (!SUnrealMcpBlueprintCommandUtils::ResolveAssetObjectPath(WidgetPath, PackagePath, AssetName, PackageName, ObjectPath))
            {
                return FSUnrealMcpResponse::MakeError(Request.RequestId, TEXT("INVALID_WIDGET_PATH"), FString::Printf(TEXT("Widget path '%s' is invalid."), *WidgetPath));
            }

            if (SUnrealMcpBlueprintCommandUtils::DoesAssetTargetExist(PackageName, ObjectPath))
            {
                return FSUnrealMcpResponse::MakeError(Request.RequestId, TEXT("WIDGET_ALREADY_EXISTS"), FString::Printf(TEXT("Widget Blueprint '%s' already exists."), *WidgetPath));
            }

            UClass* ParentClass = SUnrealMcpUMGCommandUtils::ResolveWidgetParentClassReference(ParentClassReference);
            if (ParentClass == nullptr)
            {
                return FSUnrealMcpResponse::MakeError(Request.RequestId, TEXT("INVALID_PARENT_CLASS"), FString::Printf(TEXT("Could not resolve widget parent class '%s'."), *ParentClassReference));
            }

            UPackage* Package = CreatePackage(*PackageName);
            if (Package == nullptr)
            {
                return FSUnrealMcpResponse::MakeError(Request.RequestId, TEXT("PACKAGE_CREATE_FAILED"), FString::Printf(TEXT("Could not create package for '%s'."), *WidgetPath));
            }

            UWidgetBlueprint* WidgetBlueprint = Cast<UWidgetBlueprint>(FKismetEditorUtilities::CreateBlueprint(
                ParentClass,
                Package,
                *AssetName,
                BPTYPE_Normal,
                UWidgetBlueprint::StaticClass(),
                UWidgetBlueprintGeneratedClass::StaticClass(),
                NAME_None));

            if (WidgetBlueprint == nullptr)
            {
                return FSUnrealMcpResponse::MakeError(Request.RequestId, TEXT("WIDGET_CREATE_FAILED"), FString::Printf(TEXT("Failed to create widget blueprint '%s'."), *WidgetPath));
            }

            SUnrealMcpUMGCommandUtils::EnsureRootCanvasPanel(WidgetBlueprint);
            FAssetRegistryModule::AssetCreated(WidgetBlueprint);
            Package->MarkPackageDirty();
            FString CompileError;
            if (!SUnrealMcpBlueprintCommandUtils::CompileBlueprintAndGetError(WidgetBlueprint, CompileError))
            {
                return FSUnrealMcpResponse::MakeError(
                    Request.RequestId,
                    TEXT("WIDGET_COMPILE_FAILED"),
                    CompileError);
            }

            TSharedPtr<FJsonObject> Data = MakeShared<FJsonObject>();
            Data->SetObjectField(TEXT("widget"), SUnrealMcpUMGCommandUtils::BuildWidgetBlueprintSummary(WidgetBlueprint));
            Data->SetStringField(TEXT("parentClass"), ParentClass->GetPathName());
            return FSUnrealMcpResponse::MakeSuccess(Request.RequestId, Data);
        }
    };

    const FSUnrealMcpCommandAutoRegistrar CreateUMGWidgetBlueprintCommandRegistrar([]()
    {
        return MakeShared<FCreateUMGWidgetBlueprintCommand>();
    });
}

