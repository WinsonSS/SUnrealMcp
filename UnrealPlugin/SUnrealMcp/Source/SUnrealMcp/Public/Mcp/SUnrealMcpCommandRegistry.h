#pragma once

#include "Containers/Map.h"
#include "Templates/SharedPointer.h"
#include "Templates/Function.h"

#include "Mcp/ISUnrealMcpCommand.h"

using FSUnrealMcpCommandFactory = TFunction<TSharedRef<ISUnrealMcpCommand>()>;

class SUNREALMCP_API FSUnrealMcpCommandRegistry
{
public:
    bool RegisterCommand(const TSharedRef<ISUnrealMcpCommand>& Command);
    FSUnrealMcpResponse Execute(const FSUnrealMcpRequest& Request, const FSUnrealMcpExecutionContext& Context) const;
    FString GetRegistrationErrorSummary() const;

private:
    TMap<FString, TSharedRef<ISUnrealMcpCommand>> Commands;
    TArray<FString> RegistrationErrors;
};

class SUNREALMCP_API FSUnrealMcpCommandAutoRegistrar
{
public:
    explicit FSUnrealMcpCommandAutoRegistrar(FSUnrealMcpCommandFactory InFactory);

    static bool RegisterAll(const TSharedRef<FSUnrealMcpCommandRegistry>& Registry);

private:
    static TArray<FSUnrealMcpCommandFactory>& GetFactories();
};
