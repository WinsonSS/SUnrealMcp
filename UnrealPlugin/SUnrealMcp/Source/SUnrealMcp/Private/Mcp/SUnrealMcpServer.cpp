#include "Mcp/SUnrealMcpServer.h"

#include "Common/TcpSocketBuilder.h"
#include "Containers/StringConv.h"
#include "Interfaces/IPv4/IPv4Address.h"
#include "Interfaces/IPv4/IPv4Endpoint.h"
#include "Mcp/SUnrealMcpCommandRegistry.h"
#include "Mcp/SUnrealMcpProtocol.h"
#include "Mcp/SUnrealMcpTaskRegistry.h"
#include "SUnrealMcpModule.h"
#include "Sockets.h"
#include "SocketSubsystem.h"

namespace SUnrealMcpServerLog
{
    static FString TruncateForLog(const FString& Text, const int32 MaxLength = 1024)
    {
        if (Text.Len() <= MaxLength)
        {
            return Text;
        }

        return FString::Printf(TEXT("%s... [truncated %d chars]"), *Text.Left(MaxLength), Text.Len() - MaxLength);
    }

    static FString ExtractRequestIdOrEmpty(const FString& JsonText)
    {
        TSharedPtr<FJsonObject> RootObject;
        const TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonText);
        if (!FJsonSerializer::Deserialize(Reader, RootObject) || !RootObject.IsValid())
        {
            return TEXT("");
        }

        FString RequestId;
        if (!RootObject->TryGetStringField(TEXT("requestId"), RequestId))
        {
            return TEXT("");
        }

        return RequestId;
    }
}

FSUnrealMcpServer::FSUnrealMcpServer() = default;

FSUnrealMcpServer::~FSUnrealMcpServer()
{
    Stop();
}

bool FSUnrealMcpServer::Start(
    const FSUnrealMcpServerConfig& InConfig,
    const TSharedRef<FSUnrealMcpCommandRegistry>& InRegistry,
    const TSharedRef<FSUnrealMcpTaskRegistry>& InTaskRegistry)
{
    Stop();

    Config = InConfig;
    CommandRegistry = InRegistry;
    TaskRegistry = InTaskRegistry;

    FIPv4Address BindAddress;
    if (!FIPv4Address::Parse(Config.BindAddress, BindAddress))
    {
        UE_LOG(LogSUnrealMcp, Error, TEXT("Invalid bind address: %s"), *Config.BindAddress);
        return false;
    }

    const FIPv4Endpoint Endpoint(BindAddress, Config.Port);
    ListenSocket = FTcpSocketBuilder(TEXT("SUnrealMcpListenSocket"))
        .AsReusable()
        .BoundToEndpoint(Endpoint)
        .Listening(Config.ListenBacklog);

    if (ListenSocket == nullptr)
    {
        UE_LOG(LogSUnrealMcp, Error, TEXT("Failed to create listen socket on %s:%d"), *Config.BindAddress, Config.Port);
        return false;
    }

    ListenSocket->SetNonBlocking(true);

    TickHandle = FTSTicker::GetCoreTicker().AddTicker(
        FTickerDelegate::CreateRaw(this, &FSUnrealMcpServer::Tick));

    return true;
}

void FSUnrealMcpServer::SetCommandRegistry(const TSharedRef<FSUnrealMcpCommandRegistry>& InRegistry)
{
    CommandRegistry = InRegistry;
}

void FSUnrealMcpServer::Stop()
{
    if (TickHandle.IsValid())
    {
        FTSTicker::GetCoreTicker().RemoveTicker(TickHandle);
        TickHandle.Reset();
    }

    for (TUniquePtr<FClientConnection>& Connection : Connections)
    {
        if (Connection->Socket != nullptr)
        {
            Connection->Socket->Close();
            ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->DestroySocket(Connection->Socket);
            Connection->Socket = nullptr;
        }
    }

    Connections.Reset();

    if (ListenSocket != nullptr)
    {
        ListenSocket->Close();
        ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->DestroySocket(ListenSocket);
        ListenSocket = nullptr;
    }

    CommandRegistry.Reset();
    TaskRegistry.Reset();
}

bool FSUnrealMcpServer::Tick(float DeltaTime)
{
    if (TaskRegistry.IsValid())
    {
        TaskRegistry->Tick(DeltaTime);
    }

    AcceptPendingConnections();
    ServiceConnections();
    return true;
}

void FSUnrealMcpServer::AcceptPendingConnections()
{
    if (ListenSocket == nullptr)
    {
        return;
    }

    bool bHasPendingConnection = false;
    while (ListenSocket->HasPendingConnection(bHasPendingConnection) && bHasPendingConnection)
    {
        FSocket* ClientSocket = ListenSocket->Accept(TEXT("SUnrealMcpClientSocket"));
        if (ClientSocket == nullptr)
        {
            continue;
        }

        ClientSocket->SetNonBlocking(true);
        Connections.Add(MakeUnique<FClientConnection>(ClientSocket));
        UE_LOG(LogSUnrealMcp, Verbose, TEXT("Accepted MCP client connection"));
    }
}

void FSUnrealMcpServer::ServiceConnections()
{
    for (int32 ConnectionIndex = Connections.Num() - 1; ConnectionIndex >= 0; --ConnectionIndex)
    {
        FClientConnection& Connection = *Connections[ConnectionIndex];
        if (Connection.Socket == nullptr)
        {
            RemoveConnectionAt(ConnectionIndex);
            continue;
        }

        if (!FlushPendingSends(Connection))
        {
            UE_LOG(LogSUnrealMcp, Warning, TEXT("Dropping MCP client connection after send failure"));
            RemoveConnectionAt(ConnectionIndex);
            continue;
        }

        uint32 PendingDataSize = 0;
        if (!Connection.Socket->HasPendingData(PendingDataSize))
        {
            ESocketConnectionState ConnectionState = Connection.Socket->GetConnectionState();
            if (ConnectionState != SCS_Connected)
            {
                UE_LOG(LogSUnrealMcp, Verbose, TEXT("Removing MCP client connection in state %d"), static_cast<int32>(ConnectionState));
                RemoveConnectionAt(ConnectionIndex);
            }
            continue;
        }

        TArray<uint8> Buffer;
        Buffer.SetNumUninitialized(FMath::Min<uint32>(PendingDataSize, 512 * 1024));

        int32 BytesRead = 0;
        if (!Connection.Socket->Recv(Buffer.GetData(), Buffer.Num(), BytesRead) || BytesRead <= 0)
        {
            UE_LOG(LogSUnrealMcp, Warning, TEXT("Dropping MCP client connection after recv failure"));
            RemoveConnectionAt(ConnectionIndex);
            continue;
        }

        const FUTF8ToTCHAR TextConverter(reinterpret_cast<const ANSICHAR*>(Buffer.GetData()), BytesRead);
        const FString TextChunk(TextConverter.Length(), TextConverter.Get());
        ProcessIncomingText(Connection, TextChunk);
    }
}

void FSUnrealMcpServer::ProcessIncomingText(FClientConnection& Connection, const FString& TextChunk)
{
    Connection.PendingText.Append(TextChunk);

    while (true)
    {
        int32 NewLineIndex = INDEX_NONE;
        if (!Connection.PendingText.FindChar(TEXT('\n'), NewLineIndex))
        {
            break;
        }

        FString JsonLine = Connection.PendingText.Left(NewLineIndex).TrimStartAndEnd();
        Connection.PendingText.RightChopInline(NewLineIndex + 1, EAllowShrinking::No);

        if (JsonLine.IsEmpty())
        {
            continue;
        }

        FSUnrealMcpRequest Request;
        FString ErrorMessage;
        FSUnrealMcpResponse Response;

        if (!FSUnrealMcpRequest::FromJson(JsonLine, Request, ErrorMessage))
        {
            Response = FSUnrealMcpResponse::MakeError(
                SUnrealMcpServerLog::ExtractRequestIdOrEmpty(JsonLine),
                TEXT("INVALID_REQUEST"),
                ErrorMessage);
            UE_LOG(
                LogSUnrealMcp,
                Warning,
                TEXT("Rejected invalid MCP request line. Raw=\"%s\""),
                *SUnrealMcpServerLog::TruncateForLog(JsonLine));
        }
        else
        {
            UE_LOG(LogSUnrealMcp, Verbose, TEXT("Executing MCP command %s (%s)"), *Request.Command, *Request.RequestId);
            Response = (CommandRegistry.IsValid() && TaskRegistry.IsValid())
                ? CommandRegistry->Execute(Request, FSUnrealMcpExecutionContext{TaskRegistry.ToSharedRef()})
                : FSUnrealMcpResponse::MakeError(
                    Request.RequestId,
                    TEXT("SERVER_NOT_READY"),
                    TEXT("Command registry or task registry is not available."));
        }

        if (!Response.bOk)
        {
            const FString ErrorCode = Response.Error.IsSet() ? Response.Error->Code : TEXT("UNKNOWN_ERROR");
            const FString ErrorMessageText = Response.Error.IsSet() ? Response.Error->Message : TEXT("Unknown error");
            UE_LOG(
                LogSUnrealMcp,
                Warning,
                TEXT("MCP command failed requestId=%s code=%s message=%s"),
                *Response.RequestId,
                *ErrorCode,
                *ErrorMessageText);
        }

        FString ResponseLine = Response.ToJsonLine();
        {
            FTCHARToUTF8 Utf8Check(*ResponseLine);
            const int32 ResponseBytes = Utf8Check.Length();
            if (ResponseBytes > Config.MaxPendingSendBytesPerConnection)
            {
                UE_LOG(
                    LogSUnrealMcp,
                    Warning,
                    TEXT("Response too large for requestId=%s command=%s bytes=%d limit=%d"),
                    *Response.RequestId,
                    *Request.Command,
                    ResponseBytes,
                    Config.MaxPendingSendBytesPerConnection);

                TSharedPtr<FJsonObject> Details = MakeShared<FJsonObject>();
                Details->SetNumberField(TEXT("response_bytes"), ResponseBytes);
                Details->SetNumberField(TEXT("limit_bytes"), Config.MaxPendingSendBytesPerConnection);

                const FSUnrealMcpResponse ErrorResponse = FSUnrealMcpResponse::MakeError(
                    Response.RequestId,
                    TEXT("RESPONSE_TOO_LARGE"),
                    FString::Printf(TEXT("Response from command '%s' exceeds send buffer limit (%d bytes > %d bytes)."),
                        *Request.Command, ResponseBytes, Config.MaxPendingSendBytesPerConnection),
                    Details);
                ResponseLine = ErrorResponse.ToJsonLine();
            }
        }

        if (!SendResponseLine(Connection, ResponseLine, Response.RequestId))
        {
            UE_LOG(
                LogSUnrealMcp,
                Warning,
                TEXT("Failed to send MCP response requestId=%s payload=\"%s\""),
                *Response.RequestId,
                *SUnrealMcpServerLog::TruncateForLog(ResponseLine));
        }
    }
}

bool FSUnrealMcpServer::FlushPendingSends(FClientConnection& Connection)
{
    if (Connection.Socket == nullptr)
    {
        return false;
    }

    while (Connection.PendingSendOffset < Connection.PendingSendBytes.Num())
    {
        int32 BytesSent = 0;
        const uint8* SendBuffer = Connection.PendingSendBytes.GetData() + Connection.PendingSendOffset;
        const int32 BytesRemaining = Connection.PendingSendBytes.Num() - Connection.PendingSendOffset;
        const bool bSendResult = Connection.Socket->Send(SendBuffer, BytesRemaining, BytesSent);

        if (!bSendResult || BytesSent <= 0)
        {
            if (Connection.Socket->GetConnectionState() == SCS_Connected)
            {
                return true;
            }

            return false;
        }

        Connection.PendingSendOffset += BytesSent;
    }

    Connection.PendingSendBytes.Reset();
    Connection.PendingSendOffset = 0;
    return true;
}

bool FSUnrealMcpServer::SendResponseLine(FClientConnection& Connection, const FString& ResponseLine, const FString& RequestId)
{
    if (Connection.Socket == nullptr)
    {
        return false;
    }

    FTCHARToUTF8 Utf8(*ResponseLine);
    const int32 IncomingBytes = Utf8.Length();
    const int32 QueuedBytes = Connection.PendingSendBytes.Num() - Connection.PendingSendOffset;
    if ((QueuedBytes + IncomingBytes) > Config.MaxPendingSendBytesPerConnection)
    {
        UE_LOG(
            LogSUnrealMcp,
            Warning,
            TEXT("Pending send buffer exceeded limit for requestId=%s queued=%d incoming=%d limit=%d"),
            *RequestId,
            QueuedBytes,
            IncomingBytes,
            Config.MaxPendingSendBytesPerConnection);
        return false;
    }

    Connection.PendingSendBytes.Append(reinterpret_cast<const uint8*>(Utf8.Get()), Utf8.Length());

    if (!FlushPendingSends(Connection))
    {
        UE_LOG(LogSUnrealMcp, Warning, TEXT("Socket send failed for MCP response requestId=%s"), *RequestId);
        return false;
    }

    return true;
}

void FSUnrealMcpServer::RemoveConnectionAt(int32 Index)
{
    if (!Connections.IsValidIndex(Index))
    {
        return;
    }

    TUniquePtr<FClientConnection> Connection = MoveTemp(Connections[Index]);
    Connections.RemoveAtSwap(Index);

    if (Connection.IsValid() && Connection->Socket != nullptr)
    {
        Connection->Socket->Close();
        ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->DestroySocket(Connection->Socket);
        Connection->Socket = nullptr;
    }
}
