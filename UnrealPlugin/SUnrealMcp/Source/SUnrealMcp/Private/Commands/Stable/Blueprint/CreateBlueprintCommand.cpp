#include "AssetRegistry/AssetRegistryModule.h"
#include "Commands/Shared/Blueprint/BlueprintCommandUtils.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "Mcp/ISUnrealMcpCommand.h"
#include "Mcp/SUnrealMcpCommandRegistry.h"
#include "UObject/Package.h"

namespace
{
    class FCreateBlueprintCommand final : public ISUnrealMcpCommand
    {
    public:
        virtual FString GetCommandName() const override
        {
            return TEXT("create_blueprint");
        }

        virtual FSUnrealMcpResponse Execute(const FSUnrealMcpRequest& Request, const FSUnrealMcpExecutionContext& Context) override
        {
            static_cast<void>(Context);

            if (!Request.Params.IsValid())
            {
                return FSUnrealMcpResponse::MakeError(
                    Request.RequestId,
                    TEXT("INVALID_PARAMS"),
                    TEXT("create_blueprint requires a params object."));
            }

            FString BlueprintPath;
            FString ParentClassReference;
            if (!Request.Params->TryGetStringField(TEXT("blueprint_path"), BlueprintPath) || BlueprintPath.IsEmpty())
            {
                return FSUnrealMcpResponse::MakeError(Request.RequestId, TEXT("INVALID_PARAMS"), TEXT("Missing blueprint_path."));
            }

            if (!Request.Params->TryGetStringField(TEXT("parent_class"), ParentClassReference) || ParentClassReference.IsEmpty())
            {
                return FSUnrealMcpResponse::MakeError(Request.RequestId, TEXT("INVALID_PARAMS"), TEXT("Missing parent_class."));
            }

            FString PackagePath;
            FString AssetName;
            if (!SUnrealMcpBlueprintCommandUtils::SplitAssetPath(BlueprintPath, PackagePath, AssetName))
            {
                return FSUnrealMcpResponse::MakeError(
                    Request.RequestId,
                    TEXT("INVALID_BLUEPRINT_PATH"),
                    FString::Printf(TEXT("Blueprint path '%s' is invalid."), *BlueprintPath));
            }

            if (FindObject<UBlueprint>(nullptr, *BlueprintPath) != nullptr)
            {
                return FSUnrealMcpResponse::MakeError(
                    Request.RequestId,
                    TEXT("BLUEPRINT_ALREADY_EXISTS"),
                    FString::Printf(TEXT("Blueprint '%s' already exists."), *BlueprintPath));
            }

            UClass* ParentClass = SUnrealMcpBlueprintCommandUtils::ResolveParentClassReference(ParentClassReference);
            if (ParentClass == nullptr)
            {
                return FSUnrealMcpResponse::MakeError(
                    Request.RequestId,
                    TEXT("INVALID_PARENT_CLASS"),
                    FString::Printf(TEXT("Could not resolve parent class '%s'."), *ParentClassReference));
            }

            UPackage* Package = CreatePackage(*FString::Printf(TEXT("%s/%s"), *PackagePath, *AssetName));
            if (Package == nullptr)
            {
                return FSUnrealMcpResponse::MakeError(
                    Request.RequestId,
                    TEXT("PACKAGE_CREATE_FAILED"),
                    FString::Printf(TEXT("Could not create package for '%s'."), *BlueprintPath));
            }

            UBlueprint* NewBlueprint = FKismetEditorUtilities::CreateBlueprint(
                ParentClass,
                Package,
                *AssetName,
                BPTYPE_Normal,
                UBlueprint::StaticClass(),
                UBlueprintGeneratedClass::StaticClass(),
                NAME_None);

            if (NewBlueprint == nullptr)
            {
                return FSUnrealMcpResponse::MakeError(
                    Request.RequestId,
                    TEXT("BLUEPRINT_CREATE_FAILED"),
                    FString::Printf(TEXT("Failed to create blueprint '%s'."), *BlueprintPath));
            }

            FAssetRegistryModule::AssetCreated(NewBlueprint);
            Package->MarkPackageDirty();
            FKismetEditorUtilities::CompileBlueprint(NewBlueprint);

            TSharedPtr<FJsonObject> Data = MakeShared<FJsonObject>();
            Data->SetObjectField(TEXT("blueprint"), SUnrealMcpBlueprintCommandUtils::BuildBlueprintSummary(NewBlueprint));
            Data->SetStringField(TEXT("parentClass"), ParentClass->GetPathName());
            return FSUnrealMcpResponse::MakeSuccess(Request.RequestId, Data);
        }
    };

    const FSUnrealMcpCommandAutoRegistrar CreateBlueprintCommandRegistrar([]()
    {
        return MakeShared<FCreateBlueprintCommand>();
    });
}

