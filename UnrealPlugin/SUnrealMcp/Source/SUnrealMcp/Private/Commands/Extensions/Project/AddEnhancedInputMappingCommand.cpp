#include "EnhancedActionKeyMapping.h"
#include "InputAction.h"
#include "InputCoreTypes.h"
#include "InputMappingContext.h"
#include "Mcp/ISUnrealMcpCommand.h"
#include "Mcp/SUnrealMcpCommandRegistry.h"

namespace
{
    class FAddEnhancedInputMappingCommand final : public ISUnrealMcpCommand
    {
    public:
        virtual FString GetCommandName() const override
        {
            return TEXT("add_enhanced_input_mapping");
        }

        virtual FSUnrealMcpResponse Execute(const FSUnrealMcpRequest& Request, const FSUnrealMcpExecutionContext& Context) override
        {
            static_cast<void>(Context);

            if (!Request.Params.IsValid())
            {
                return FSUnrealMcpResponse::MakeError(Request.RequestId, TEXT("INVALID_PARAMS"), TEXT("add_enhanced_input_mapping requires a params object."));
            }

            FString MappingContextPath;
            FString InputActionPath;
            FString KeyName;
            if (!Request.Params->TryGetStringField(TEXT("mapping_context_path"), MappingContextPath) || MappingContextPath.IsEmpty())
            {
                return FSUnrealMcpResponse::MakeError(Request.RequestId, TEXT("INVALID_PARAMS"), TEXT("Missing mapping_context_path."));
            }
            if (!Request.Params->TryGetStringField(TEXT("input_action_path"), InputActionPath) || InputActionPath.IsEmpty())
            {
                return FSUnrealMcpResponse::MakeError(Request.RequestId, TEXT("INVALID_PARAMS"), TEXT("Missing input_action_path."));
            }
            if (!Request.Params->TryGetStringField(TEXT("key"), KeyName) || KeyName.IsEmpty())
            {
                return FSUnrealMcpResponse::MakeError(Request.RequestId, TEXT("INVALID_PARAMS"), TEXT("Missing key."));
            }

            UInputMappingContext* MappingContext = LoadObject<UInputMappingContext>(nullptr, *MappingContextPath);
            if (MappingContext == nullptr)
            {
                return FSUnrealMcpResponse::MakeError(Request.RequestId, TEXT("MAPPING_CONTEXT_NOT_FOUND"), FString::Printf(TEXT("Could not load mapping context '%s'."), *MappingContextPath));
            }

            UInputAction* InputAction = LoadObject<UInputAction>(nullptr, *InputActionPath);
            if (InputAction == nullptr)
            {
                return FSUnrealMcpResponse::MakeError(Request.RequestId, TEXT("INPUT_ACTION_NOT_FOUND"), FString::Printf(TEXT("Could not load input action '%s'."), *InputActionPath));
            }

            const FKey Key(*KeyName);
            if (!Key.IsValid())
            {
                return FSUnrealMcpResponse::MakeError(Request.RequestId, TEXT("INVALID_KEY"), FString::Printf(TEXT("Key '%s' is not a valid Unreal input key."), *KeyName));
            }

            MappingContext->Modify();
            MappingContext->MapKey(InputAction, Key);
            MappingContext->PostEditChange();
            MappingContext->MarkPackageDirty();

            TSharedPtr<FJsonObject> Data = MakeShared<FJsonObject>();
            Data->SetStringField(TEXT("mappingContext"), MappingContext->GetPathName());
            Data->SetStringField(TEXT("inputAction"), InputAction->GetPathName());
            Data->SetStringField(TEXT("key"), Key.GetDisplayName().ToString());
            Data->SetStringField(TEXT("keyName"), Key.GetFName().ToString());
            Data->SetNumberField(TEXT("mappingCount"), MappingContext->GetMappings().Num());
            return FSUnrealMcpResponse::MakeSuccess(Request.RequestId, Data);
        }
    };

    const FSUnrealMcpCommandAutoRegistrar AddEnhancedInputMappingCommandRegistrar([]()
    {
        return MakeShared<FAddEnhancedInputMappingCommand>();
    });
}
