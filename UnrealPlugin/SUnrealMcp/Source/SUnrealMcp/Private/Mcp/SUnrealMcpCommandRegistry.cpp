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
    for (const FSUnrealMcpCommandFactory& Factory : ConsumeFactories())
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

TArray<FSUnrealMcpCommandFactory> FSUnrealMcpCommandAutoRegistrar::ConsumeFactories()
{
    TArray<FSUnrealMcpCommandFactory> Factories = MoveTemp(GetFactories());
    GetFactories().Reset();
    return Factories;
}
