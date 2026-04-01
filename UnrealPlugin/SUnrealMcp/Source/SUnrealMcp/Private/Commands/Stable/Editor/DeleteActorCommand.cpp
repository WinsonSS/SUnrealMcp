#include "Commands/Shared/Editor/EditorCommandUtils.h"
#include "Mcp/ISUnrealMcpCommand.h"
#include "Mcp/SUnrealMcpCommandRegistry.h"

namespace
{
    class FDeleteActorCommand final : public ISUnrealMcpCommand
    {
    public:
        virtual FString GetCommandName() const override
        {
            return TEXT("delete_actor");
        }

        virtual FSUnrealMcpResponse Execute(const FSUnrealMcpRequest& Request, const FSUnrealMcpExecutionContext& Context) override
        {
            static_cast<void>(Context);

            if (!Request.Params.IsValid())
            {
                return FSUnrealMcpResponse::MakeError(
                    Request.RequestId,
                    TEXT("INVALID_PARAMS"),
                    TEXT("delete_actor requires a params object."));
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

            const TSharedPtr<FJsonObject> ActorSummary = SUnrealMcpEditorCommandUtils::BuildActorSummary(Actor);
            const bool bDestroyed = EditorWorld->EditorDestroyActor(Actor, true);
            if (!bDestroyed)
            {
                return FSUnrealMcpResponse::MakeError(
                    Request.RequestId,
                    TEXT("DELETE_FAILED"),
                    FString::Printf(TEXT("Failed to delete actor '%s'."), *ActorPath));
            }

            TSharedPtr<FJsonObject> Data = MakeShared<FJsonObject>();
            Data->SetObjectField(TEXT("deletedActor"), ActorSummary);
            return FSUnrealMcpResponse::MakeSuccess(Request.RequestId, Data);
        }
    };

    const FSUnrealMcpCommandAutoRegistrar DeleteActorCommandRegistrar([]()
    {
        return MakeShared<FDeleteActorCommand>();
    });
}

