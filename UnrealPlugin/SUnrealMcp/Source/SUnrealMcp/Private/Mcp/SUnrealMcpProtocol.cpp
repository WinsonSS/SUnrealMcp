#include "Mcp/SUnrealMcpProtocol.h"

#include "Containers/UnrealString.h"
#include "Dom/JsonValue.h"
#include "SUnrealMcpModule.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"

namespace SUnrealMcpProtocol
{
    static FString TruncateForLog(const FString& Text, const int32 MaxLength = 1024)
    {
        if (Text.Len() <= MaxLength)
        {
            return Text;
        }

        return FString::Printf(TEXT("%s... [truncated %d chars]"), *Text.Left(MaxLength), Text.Len() - MaxLength);
    }
}

bool FSUnrealMcpRequest::FromJson(const FString& JsonText, FSUnrealMcpRequest& OutRequest, FString& OutErrorMessage)
{
    OutRequest = FSUnrealMcpRequest{};

    TSharedPtr<FJsonObject> RootObject;
    const TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonText);
    if (!FJsonSerializer::Deserialize(Reader, RootObject) || !RootObject.IsValid())
    {
        OutErrorMessage = TEXT("Request was not valid JSON.");
        UE_LOG(
            LogSUnrealMcp,
            Warning,
            TEXT("Failed to parse MCP request JSON. Error=\"%s\" Raw=\"%s\""),
            *OutErrorMessage,
            *SUnrealMcpProtocol::TruncateForLog(JsonText));
        return false;
    }

    int32 ParsedProtocolVersion = 0;
    if (!RootObject->TryGetNumberField(TEXT("protocolVersion"), ParsedProtocolVersion) || ParsedProtocolVersion != ProtocolVersion)
    {
        OutErrorMessage = TEXT("Unsupported or missing protocolVersion.");
        UE_LOG(
            LogSUnrealMcp,
            Warning,
            TEXT("Rejected MCP request due to protocolVersion. Raw=\"%s\""),
            *SUnrealMcpProtocol::TruncateForLog(JsonText));
        return false;
    }

    if (!RootObject->TryGetStringField(TEXT("requestId"), OutRequest.RequestId) || OutRequest.RequestId.IsEmpty())
    {
        OutErrorMessage = TEXT("Missing requestId.");
        UE_LOG(
            LogSUnrealMcp,
            Warning,
            TEXT("Rejected MCP request with missing requestId. Raw=\"%s\""),
            *SUnrealMcpProtocol::TruncateForLog(JsonText));
        return false;
    }

    if (!RootObject->TryGetStringField(TEXT("command"), OutRequest.Command) || OutRequest.Command.IsEmpty())
    {
        OutErrorMessage = TEXT("Missing command.");
        UE_LOG(
            LogSUnrealMcp,
            Warning,
            TEXT("Rejected MCP request %s with missing command. Raw=\"%s\""),
            *OutRequest.RequestId,
            *SUnrealMcpProtocol::TruncateForLog(JsonText));
        return false;
    }

    const TSharedPtr<FJsonObject>* ParamsObject = nullptr;
    if (!RootObject->HasField(TEXT("params")))
    {
        OutRequest.Params = MakeShared<FJsonObject>();
    }
    else if (RootObject->TryGetObjectField(TEXT("params"), ParamsObject) && ParamsObject != nullptr && ParamsObject->IsValid())
    {
        OutRequest.Params = *ParamsObject;
    }
    else
    {
        OutErrorMessage = TEXT("Field 'params' must be a JSON object.");
        UE_LOG(
            LogSUnrealMcp,
            Warning,
            TEXT("Rejected MCP request %s because params was not an object. Raw=\"%s\""),
            *OutRequest.RequestId,
            *SUnrealMcpProtocol::TruncateForLog(JsonText));
        return false;
    }

    UE_LOG(
        LogSUnrealMcp,
        Verbose,
        TEXT("Parsed MCP request %s command=%s"),
        *OutRequest.RequestId,
        *OutRequest.Command);

    return true;
}

FString FSUnrealMcpResponse::ToJsonLine() const
{
    TSharedRef<FJsonObject> RootObject = MakeShared<FJsonObject>();
    RootObject->SetNumberField(TEXT("protocolVersion"), FSUnrealMcpRequest::ProtocolVersion);
    RootObject->SetStringField(TEXT("requestId"), RequestId);
    RootObject->SetBoolField(TEXT("ok"), bOk);

    if (bOk)
    {
        RootObject->SetObjectField(TEXT("data"), Data.IsValid() ? Data : MakeShared<FJsonObject>());
    }
    else
    {
        TSharedRef<FJsonObject> ErrorObject = MakeShared<FJsonObject>();
        const FSUnrealMcpError ErrorValue = Error.Get(FSUnrealMcpError{
            TEXT("UNKNOWN_ERROR"),
            TEXT("Command failed without an explicit error payload."),
            nullptr
        });
        ErrorObject->SetStringField(TEXT("code"), ErrorValue.Code);
        ErrorObject->SetStringField(TEXT("message"), ErrorValue.Message);
        if (ErrorValue.Details.IsValid())
        {
            ErrorObject->SetObjectField(TEXT("details"), ErrorValue.Details);
        }

        RootObject->SetObjectField(TEXT("error"), ErrorObject);
    }

    FString JsonLine;
    const TSharedRef<TJsonWriter<TCHAR, TCondensedJsonPrintPolicy<TCHAR>>> Writer =
        TJsonWriterFactory<TCHAR, TCondensedJsonPrintPolicy<TCHAR>>::Create(&JsonLine);
    FJsonSerializer::Serialize(RootObject, Writer);
    JsonLine.AppendChar(TEXT('\n'));
    return JsonLine;
}

FSUnrealMcpResponse FSUnrealMcpResponse::MakeSuccess(const FString& RequestId, const TSharedPtr<FJsonObject>& Data)
{
    FSUnrealMcpResponse Response;
    Response.RequestId = RequestId;
    Response.bOk = true;
    Response.Data = Data.IsValid() ? Data : MakeShared<FJsonObject>();
    return Response;
}

FSUnrealMcpResponse FSUnrealMcpResponse::MakeError(
    const FString& RequestId,
    const FString& Code,
    const FString& Message,
    const TSharedPtr<FJsonObject>& Details)
{
    FSUnrealMcpResponse Response;
    Response.RequestId = RequestId;
    Response.bOk = false;
    Response.Error = FSUnrealMcpError{Code, Message, Details};
    return Response;
}
