#include "Commands/Extensions/Editor/EditorCommandUtils.h"
#include "Mcp/ISUnrealMcpCommand.h"
#include "Mcp/SUnrealMcpCommandRegistry.h"

namespace
{
    class FGetActorPropertiesCommand final : public ISUnrealMcpCommand
    {
    public:
        virtual FString GetCommandName() const override
        {
            return TEXT("get_actor_properties");
        }

        virtual FSUnrealMcpResponse Execute(const FSUnrealMcpRequest& Request, const FSUnrealMcpExecutionContext& Context) override
        {
            static_cast<void>(Context);

            if (!Request.Params.IsValid())
            {
                return FSUnrealMcpResponse::MakeError(
                    Request.RequestId,
                    TEXT("INVALID_PARAMS"),
                    TEXT("get_actor_properties requires a params object."));
            }

            FString ActorPath;
            if (!Request.Params->TryGetStringField(TEXT("actor_path"), ActorPath) || ActorPath.IsEmpty())
            {
                return FSUnrealMcpResponse::MakeError(
                    Request.RequestId,
                    TEXT("INVALID_PARAMS"),
                    TEXT("Missing actor_path."));
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

            TSharedPtr<FJsonObject> Properties = MakeShared<FJsonObject>();
            int32 ExportedPropertyCount = 0;
            for (TFieldIterator<FProperty> It(Actor->GetClass(), EFieldIteratorFlags::IncludeSuper); It; ++It)
            {
                FProperty* Property = *It;
                if (!SUnrealMcpEditorCommandUtils::IsExposedActorProperty(Property))
                {
                    continue;
                }

                Properties->SetField(Property->GetName(), SUnrealMcpEditorCommandUtils::ExportPropertyValueToJson(Property, Actor));
                ++ExportedPropertyCount;
            }

            TSharedPtr<FJsonObject> Data = MakeShared<FJsonObject>();
            Data->SetObjectField(TEXT("actor"), SUnrealMcpEditorCommandUtils::BuildActorSummary(Actor));
            Data->SetObjectField(TEXT("properties"), Properties);
            Data->SetNumberField(TEXT("count"), ExportedPropertyCount);
            return FSUnrealMcpResponse::MakeSuccess(Request.RequestId, Data);
        }
    };

    const FSUnrealMcpCommandAutoRegistrar GetActorPropertiesCommandRegistrar([]()
    {
        return MakeShared<FGetActorPropertiesCommand>();
    });
}
