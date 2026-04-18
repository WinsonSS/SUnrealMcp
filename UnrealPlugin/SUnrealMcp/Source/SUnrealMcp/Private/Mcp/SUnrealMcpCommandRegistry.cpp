#include "Mcp/SUnrealMcpCommandRegistry.h"

#include "SUnrealMcpModule.h"

bool FSUnrealMcpCommandRegistry::RegisterCommand(const TSharedRef<ISUnrealMcpCommand>& Command)
{
    const FString CommandName = Command->GetCommandName().TrimStartAndEnd();
    if (CommandName.IsEmpty())
    {
        UE_LOG(LogSUnrealMcp, Error, TEXT("Refusing to register command with an empty name."));
        RegistrationErrors.Add(TEXT("Command name must not be empty."));
        return false;
    }

    if (Commands.Contains(CommandName))
    {
        UE_LOG(LogSUnrealMcp, Error, TEXT("Duplicate command registration detected for '%s'."), *CommandName);
        RegistrationErrors.Add(FString::Printf(
            TEXT("Duplicate command name: '%s'"),
            *CommandName));
        return false;
    }

    Commands.Add(CommandName, Command);
    return true;
}

FSUnrealMcpResponse FSUnrealMcpCommandRegistry::Execute(const FSUnrealMcpRequest& Request, const FSUnrealMcpExecutionContext& Context) const
{
    const TSharedRef<ISUnrealMcpCommand>* FoundCommand = Commands.Find(Request.Command);
    if (!FoundCommand)
    {
        return FSUnrealMcpResponse::MakeError(
            Request.RequestId,
            TEXT("UNKNOWN_COMMAND"),
            FString::Printf(TEXT("No command handler is registered for '%s'."), *Request.Command));
    }

    return (*FoundCommand)->Execute(Request, Context);
}

TArray<FString> FSUnrealMcpCommandRegistry::ListCommandNames() const
{
    TArray<FString> Names;
    Names.Reserve(Commands.Num());
    for (const TPair<FString, TSharedRef<ISUnrealMcpCommand>>& Pair : Commands)
    {
        Names.Add(Pair.Key);
    }
    Names.Sort();
    return Names;
}

bool FSUnrealMcpCommandRegistry::HasCommand(const FString& Name) const
{
    return Commands.Contains(Name);
}

FString FSUnrealMcpCommandRegistry::GetRegistrationErrorSummary() const
{
    if (RegistrationErrors.IsEmpty())
    {
        return TEXT("Unknown command registration error.");
    }

    FString Summary = TEXT("SUnrealMcp command registration failed:\n\n");
    for (const FString& Error : RegistrationErrors)
    {
        Summary += FString::Printf(TEXT("- %s\n"), *Error);
    }

    return Summary.TrimEnd();
}

FSUnrealMcpCommandAutoRegistrar::FSUnrealMcpCommandAutoRegistrar(FSUnrealMcpCommandFactory InFactory)
{
    GetFactories().Add(MoveTemp(InFactory));
}

bool FSUnrealMcpCommandAutoRegistrar::RegisterAll(const TSharedRef<FSUnrealMcpCommandRegistry>& Registry)
{
    bool bRegisteredAllCommands = true;
    for (const FSUnrealMcpCommandFactory& Factory : GetFactories())
    {
        if (!Registry->RegisterCommand(Factory()))
        {
            bRegisteredAllCommands = false;
        }
    }

    return bRegisteredAllCommands;
}

TArray<FSUnrealMcpCommandFactory>& FSUnrealMcpCommandAutoRegistrar::GetFactories()
{
    static TArray<FSUnrealMcpCommandFactory> Factories;
    return Factories;
}
