#include "Mcp/ISUnrealMcpCommand.h"
#include "Mcp/SUnrealMcpCommandRegistry.h"

#include "Dom/JsonValue.h"

namespace
{
    class FDiffCommandsCommand final : public ISUnrealMcpCommand
    {
    public:
        virtual FString GetCommandName() const override
        {
            return TEXT("diff_commands");
        }

        virtual FSUnrealMcpResponse Execute(const FSUnrealMcpRequest& Request, const FSUnrealMcpExecutionContext& Context) override
        {
            if (!Request.Params.IsValid())
            {
                return FSUnrealMcpResponse::MakeError(
                    Request.RequestId,
                    TEXT("INVALID_PARAMS"),
                    TEXT("diff_commands requires a params object with a \"cli_names\" array."));
            }

            const TArray<TSharedPtr<FJsonValue>>* JsonArray = nullptr;
            if (!Request.Params->TryGetArrayField(TEXT("cli_names"), JsonArray))
            {
                return FSUnrealMcpResponse::MakeError(
                    Request.RequestId,
                    TEXT("INVALID_PARAMS"),
                    TEXT("Missing or non-array \"cli_names\"."));
            }

            TSet<FString> CliNamesSet;
            CliNamesSet.Reserve(JsonArray->Num());
            for (const TSharedPtr<FJsonValue>& Entry : *JsonArray)
            {
                FString EntryString;
                if (Entry.IsValid() && Entry->TryGetString(EntryString))
                {
                    CliNamesSet.Add(EntryString);
                }
            }

            const TArray<FString> UnrealNames = Context.ListCommandNames();
            TSet<FString> UnrealNamesSet(UnrealNames);

            TArray<FString> OnlyInUnreal;
            for (const FString& Name : UnrealNames)
            {
                if (!CliNamesSet.Contains(Name))
                {
                    OnlyInUnreal.Add(Name);
                }
            }
            OnlyInUnreal.Sort();

            TArray<FString> OnlyInCli;
            for (const FString& Name : CliNamesSet)
            {
                if (!UnrealNamesSet.Contains(Name))
                {
                    OnlyInCli.Add(Name);
                }
            }
            OnlyInCli.Sort();

            auto ToJsonArray = [](const TArray<FString>& Strings)
            {
                TArray<TSharedPtr<FJsonValue>> Values;
                Values.Reserve(Strings.Num());
                for (const FString& Name : Strings)
                {
                    Values.Add(MakeShared<FJsonValueString>(Name));
                }
                return Values;
            };

            TSharedPtr<FJsonObject> Data = MakeShared<FJsonObject>();
            Data->SetArrayField(TEXT("only_in_unreal"), ToJsonArray(OnlyInUnreal));
            Data->SetArrayField(TEXT("only_in_cli"), ToJsonArray(OnlyInCli));
            return FSUnrealMcpResponse::MakeSuccess(Request.RequestId, Data);
        }
    };

    const FSUnrealMcpCommandAutoRegistrar DiffCommandsCommandRegistrar([]()
    {
        return MakeShared<FDiffCommandsCommand>();
    });
}
