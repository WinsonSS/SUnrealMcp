#include "Mcp/ISUnrealMcpCommand.h"
#include "Mcp/ISUnrealMcpTask.h"
#include "Mcp/SUnrealMcpCommandRegistry.h"

#include "Dom/JsonObject.h"
#include "Misc/Paths.h"
#include "Modules/ModuleManager.h"

#if WITH_EDITOR
#include "Editor.h"
#endif

#if PLATFORM_WINDOWS
#include "ILiveCodingModule.h"
#endif

namespace
{
    enum class ERefreshCppRuntimePhase : uint8
    {
        Preflight,
        RunningLiveCoding,
        ReloadingCommandRegistry,
        Succeeded,
        Failed
    };

    static FString LexToString(const ERefreshCppRuntimePhase Phase)
    {
        switch (Phase)
        {
        case ERefreshCppRuntimePhase::Preflight:
            return TEXT("preflight");
        case ERefreshCppRuntimePhase::RunningLiveCoding:
            return TEXT("running_live_coding");
        case ERefreshCppRuntimePhase::ReloadingCommandRegistry:
            return TEXT("reloading_command_registry");
        case ERefreshCppRuntimePhase::Succeeded:
            return TEXT("succeeded");
        case ERefreshCppRuntimePhase::Failed:
            return TEXT("failed");
        default:
            return TEXT("unknown");
        }
    }

    class FRefreshCppRuntimeTask final : public ISUnrealMcpTask
    {
    public:
        explicit FRefreshCppRuntimeTask(TFunction<bool(FString*)> InReloadCommandRegistry)
            : ReloadCommandRegistry(MoveTemp(InReloadCommandRegistry))
        {
        }

        virtual FString GetTaskName() const override
        {
            return TEXT("refresh_cpp_runtime");
        }

        virtual void Start() override
        {
            if (State == ESUnrealMcpTaskState::Pending)
            {
                State = ESUnrealMcpTaskState::Running;
                Phase = ERefreshCppRuntimePhase::Preflight;
            }
        }

        virtual void Tick(float DeltaTime) override
        {
            static_cast<void>(DeltaTime);

            if (State != ESUnrealMcpTaskState::Running || bWorkExecuted)
            {
                return;
            }

            bWorkExecuted = true;

#if WITH_EDITOR
            if (GEditor != nullptr && GEditor->IsPlaySessionInProgress())
            {
                TSharedRef<FJsonObject> Details = MakeResolutionDetails(
                    TEXT("needs_user_editor_action"),
                    TEXT("stop_pie_and_retry"));
                Details->SetStringField(TEXT("required_user_action"), TEXT("stop_pie"));
                Fail(TEXT("PIE_ACTIVE"), TEXT("Cannot refresh the C++ runtime while PIE is running."), Details);
                return;
            }
#endif

            const FString ProjectFilePath = FPaths::ConvertRelativePathToFull(FPaths::GetProjectFilePath());
            if (ProjectFilePath.IsEmpty() || !FPaths::FileExists(ProjectFilePath))
            {
                FailUnavailable(ProjectFilePath.IsEmpty()
                    ? TEXT("Live Coding requires a valid source project file for the current editor session.")
                    : FString::Printf(TEXT("Live Coding requires the current source project file to exist: %s"), *ProjectFilePath));
                return;
            }

#if PLATFORM_WINDOWS
            ILiveCodingModule* LiveCodingModule = FModuleManager::LoadModulePtr<ILiveCodingModule>(LIVE_CODING_MODULE_NAME);
            if (LiveCodingModule == nullptr)
            {
                FailUnavailable(TEXT("Live Coding module is not available in the current editor session."));
                return;
            }

            Phase = ERefreshCppRuntimePhase::RunningLiveCoding;

            ELiveCodingCompileResult CompileResult = ELiveCodingCompileResult::Failure;
            const bool bCompileCallSucceeded =
                LiveCodingModule->Compile(ELiveCodingCompileFlags::WaitForCompletion, &CompileResult);

            switch (CompileResult)
            {
            case ELiveCodingCompileResult::Success:
            case ELiveCodingCompileResult::NoChanges:
                break;

            case ELiveCodingCompileResult::CompileStillActive:
                {
                    TSharedRef<FJsonObject> Details = MakeResolutionDetails(
                        TEXT("needs_user_editor_action"),
                        TEXT("wait_for_current_compile_and_retry"));
                    Fail(TEXT("LIVE_CODING_BUSY"), TEXT("Another Live Coding compile is already in progress."), Details);
                    return;
                }

            case ELiveCodingCompileResult::NotStarted:
                {
                    const FString EnableError = LiveCodingModule->GetEnableErrorText().ToString();
                    FailUnavailable(EnableError.IsEmpty()
                        ? TEXT("Live Coding could not be started in the current environment.")
                        : EnableError);
                    return;
                }

            case ELiveCodingCompileResult::Cancelled:
                FailCompile(TEXT("Live Coding was cancelled before it could apply the current code changes."));
                return;

            case ELiveCodingCompileResult::Failure:
            default:
                FailCompile(bCompileCallSucceeded
                    ? TEXT("Live Coding failed. Please see the Live Coding console for more information.")
                    : TEXT("Live Coding compile did not succeed."));
                return;
            }

            Phase = ERefreshCppRuntimePhase::ReloadingCommandRegistry;

            FString ReloadError;
            if (!ReloadCommandRegistry || !ReloadCommandRegistry(&ReloadError))
            {
                TSharedRef<FJsonObject> Details = MakeResolutionDetails(
                    TEXT("agent_retry"),
                    TEXT("retry_refresh_cpp_runtime"));
                if (!ReloadError.IsEmpty())
                {
                    Details->SetStringField(TEXT("reload_error"), ReloadError);
                }

                Fail(
                    TEXT("REGISTRY_RELOAD_FAILED"),
                    ReloadError.IsEmpty()
                        ? TEXT("Live Coding succeeded, but reloading the command registry failed.")
                        : ReloadError,
                    Details);
                return;
            }

            bCommandRegistryReloaded = true;
            Phase = ERefreshCppRuntimePhase::Succeeded;
            State = ESUnrealMcpTaskState::Succeeded;
#else
            FailUnavailable(TEXT("Live Coding is only supported by this workflow on Windows editor builds."));
#endif
        }

        virtual void Cancel() override
        {
            if (State == ESUnrealMcpTaskState::Pending || State == ESUnrealMcpTaskState::Running)
            {
                State = ESUnrealMcpTaskState::Cancelled;
            }
        }

        virtual ESUnrealMcpTaskState GetState() const override
        {
            return State;
        }

        virtual TSharedPtr<FJsonObject> BuildPayload() const override
        {
            const TSharedRef<FJsonObject> Payload = MakeShared<FJsonObject>();
            Payload->SetStringField(TEXT("phase"), LexToString(Phase));
            Payload->SetBoolField(TEXT("command_registry_reloaded"), bCommandRegistryReloaded);

            if (State == ESUnrealMcpTaskState::Succeeded)
            {
                Payload->SetStringField(TEXT("live_coding"), TEXT("succeeded"));
            }

            return Payload;
        }

        virtual TOptional<FSUnrealMcpError> GetError() const override
        {
            return Error;
        }

    private:
        void FailUnavailable(const FString& Message)
        {
            TSharedRef<FJsonObject> Details = MakeResolutionDetails(
                TEXT("needs_user_approval"),
                TEXT("close_editor_and_rebuild"));
            Fail(TEXT("LIVE_CODING_UNAVAILABLE"), Message, Details);
        }

        void FailCompile(const FString& Message)
        {
            TSharedRef<FJsonObject> Details = MakeResolutionDetails(
                TEXT("agent_retry"),
                TEXT("fix_code_and_retry"));
            Fail(TEXT("LIVE_CODING_COMPILE_FAILED"), Message, Details);
        }

        void Fail(const FString& Code, const FString& Message, const TSharedRef<FJsonObject>& Details)
        {
            Error = FSUnrealMcpError{Code, Message, Details};
            Phase = ERefreshCppRuntimePhase::Failed;
            State = ESUnrealMcpTaskState::Failed;
        }

        TSharedRef<FJsonObject> MakeResolutionDetails(const FString& Resolution, const FString& SuggestedAction) const
        {
            const TSharedRef<FJsonObject> Details = MakeShared<FJsonObject>();
            Details->SetStringField(TEXT("resolution"), Resolution);
            Details->SetStringField(TEXT("suggested_action"), SuggestedAction);
            return Details;
        }

    private:
        TFunction<bool(FString*)> ReloadCommandRegistry;
        TOptional<FSUnrealMcpError> Error;
        ESUnrealMcpTaskState State = ESUnrealMcpTaskState::Pending;
        ERefreshCppRuntimePhase Phase = ERefreshCppRuntimePhase::Preflight;
        bool bWorkExecuted = false;
        bool bCommandRegistryReloaded = false;
    };

    class FRefreshCppRuntimeCommand final : public ISUnrealMcpCommand
    {
    public:
        virtual FString GetCommandName() const override
        {
            return TEXT("refresh_cpp_runtime");
        }

        virtual FSUnrealMcpResponse Execute(const FSUnrealMcpRequest& Request, const FSUnrealMcpExecutionContext& Context) override
        {
            const FString TaskId = Context.EnqueueTask(MakeShared<FRefreshCppRuntimeTask>(
                [Context](FString* OutError)
                {
                    return Context.ReloadCommandRegistry(OutError);
                }));
            return Context.MakeAcceptedTaskResponse(Request.RequestId, TaskId);
        }
    };

    const FSUnrealMcpCommandAutoRegistrar RefreshCppRuntimeCommandRegistrar([]()
    {
        return MakeShared<FRefreshCppRuntimeCommand>();
    });
}
