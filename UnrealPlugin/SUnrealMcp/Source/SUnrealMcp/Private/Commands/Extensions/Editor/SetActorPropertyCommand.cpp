#include "Commands/Extensions/Editor/EditorCommandUtils.h"
#include "Mcp/ISUnrealMcpCommand.h"
#include "Mcp/SUnrealMcpCommandRegistry.h"

namespace
{
    class FSetActorPropertyCommand final : public ISUnrealMcpCommand
    {
    public:
        virtual FString GetCommandName() const override
        {
            return TEXT("set_actor_property");
        }

        virtual FSUnrealMcpResponse Execute(const FSUnrealMcpRequest& Request, const FSUnrealMcpExecutionContext& Context) override
        {
            static_cast<void>(Context);

            if (!Request.Params.IsValid())
            {
                return FSUnrealMcpResponse::MakeError(
                    Request.RequestId,
                    TEXT("INVALID_PARAMS"),
                    TEXT("set_actor_property requires a params object."));
            }

            FString ActorPath;
            FString PropertyName;
            if (!Request.Params->TryGetStringField(TEXT("actor_path"), ActorPath) || ActorPath.IsEmpty())
            {
                return FSUnrealMcpResponse::MakeError(
                    Request.RequestId,
                    TEXT("INVALID_PARAMS"),
                    TEXT("Missing actor_path."));
            }

            if (!Request.Params->TryGetStringField(TEXT("property_name"), PropertyName) || PropertyName.IsEmpty())
            {
                return FSUnrealMcpResponse::MakeError(
                    Request.RequestId,
                    TEXT("INVALID_PARAMS"),
                    TEXT("Missing property_name."));
            }

            TSharedPtr<FJsonValue> PropertyValue = Request.Params->TryGetField(TEXT("property_value"));
            if (!PropertyValue.IsValid())
            {
                return FSUnrealMcpResponse::MakeError(
                    Request.RequestId,
                    TEXT("INVALID_PARAMS"),
                    TEXT("Missing property_value."));
            }

            FSUnrealMcpResponse ErrorResponse;
            UWorld* EditorWorld = SUnrealMcpEditorCommandUtils::GetEditorWorld(Request.RequestId, ErrorResponse);
            if (EditorWorld == nullptr)
            {
                return ErrorResponse;
            }

            AActor* Actor = SUnrealMcpEditorCommandUtils::FindActorByPath(EditorWorld, ActorPath);
            if (Actor == nullptr)
            {
                return FSUnrealMcpResponse::MakeError(
                    Request.RequestId,
                    TEXT("ACTOR_NOT_FOUND"),
                    FString::Printf(TEXT("Could not find actor '%s'."), *ActorPath));
            }

            FProperty* Property = SUnrealMcpEditorCommandUtils::FindActorProperty(Actor, PropertyName);
            if (!SUnrealMcpEditorCommandUtils::IsExposedActorProperty(Property))
            {
                return FSUnrealMcpResponse::MakeError(
                    Request.RequestId,
                    TEXT("PROPERTY_NOT_FOUND"),
                    FString::Printf(TEXT("Could not find supported property '%s'."), *PropertyName));
            }

            FString ApplyError;
            Actor->Modify();
            if (!SUnrealMcpEditorCommandUtils::ApplyJsonValueToProperty(PropertyValue, Property, Actor, ApplyError))
            {
                return FSUnrealMcpResponse::MakeError(
                    Request.RequestId,
                    TEXT("INVALID_PROPERTY_VALUE"),
                    ApplyError);
            }

            Actor->PostEditChange();
            Actor->MarkPackageDirty();

            TSharedPtr<FJsonObject> Data = MakeShared<FJsonObject>();
            Data->SetObjectField(TEXT("actor"), SUnrealMcpEditorCommandUtils::BuildActorSummary(Actor));
            Data->SetStringField(TEXT("propertyName"), Property->GetName());
            Data->SetField(TEXT("propertyValue"), SUnrealMcpEditorCommandUtils::ExportPropertyValueToJson(Property, Actor));
            return FSUnrealMcpResponse::MakeSuccess(Request.RequestId, Data);
        }
    };

    const FSUnrealMcpCommandAutoRegistrar SetActorPropertyCommandRegistrar([]()
    {
        return MakeShared<FSetActorPropertyCommand>();
    });
}
