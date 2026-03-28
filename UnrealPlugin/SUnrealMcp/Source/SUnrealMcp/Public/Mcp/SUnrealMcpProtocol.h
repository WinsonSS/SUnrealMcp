#pragma once

#include "Containers/Array.h"
#include "Containers/UnrealString.h"
#include "Dom/JsonObject.h"

struct FSUnrealMcpRequest
{
    static constexpr int32 ProtocolVersion = 1;

    FString RequestId;
    FString Command;
    TSharedPtr<FJsonObject> Params;

    static bool FromJson(const FString& JsonText, FSUnrealMcpRequest& OutRequest, FString& OutErrorMessage);
};

struct FSUnrealMcpError
{
    FString Code;
    FString Message;
    TSharedPtr<FJsonObject> Details;
};

struct FSUnrealMcpResponse
{
    FString RequestId;
    bool bOk = false;
    TSharedPtr<FJsonObject> Data;
    TOptional<FSUnrealMcpError> Error;

    FString ToJsonLine() const;

    static FSUnrealMcpResponse MakeSuccess(const FString& RequestId, const TSharedPtr<FJsonObject>& Data);
    static FSUnrealMcpResponse MakeError(
        const FString& RequestId,
        const FString& Code,
        const FString& Message,
        const TSharedPtr<FJsonObject>& Details = nullptr);
};
