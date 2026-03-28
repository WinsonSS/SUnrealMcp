#pragma once

#include "Containers/Array.h"
#include "Containers/Ticker.h"
#include "Containers/UnrealString.h"
#include "Templates/SharedPointer.h"
#include "Templates/UniquePtr.h"

class FSocket;
class FSUnrealMcpCommandRegistry;
class FSUnrealMcpTaskRegistry;

struct FSUnrealMcpServerConfig
{
    FString BindAddress = TEXT("127.0.0.1");
    int32 Port = 55557;
    int32 ListenBacklog = 8;
    int32 MaxPendingSendBytesPerConnection = 8 * 1024 * 1024;
};

class SUNREALMCP_API FSUnrealMcpServer
{
public:
    FSUnrealMcpServer();
    ~FSUnrealMcpServer();

    FSUnrealMcpServer(const FSUnrealMcpServer&) = delete;
    FSUnrealMcpServer& operator=(const FSUnrealMcpServer&) = delete;
    FSUnrealMcpServer(FSUnrealMcpServer&&) = delete;
    FSUnrealMcpServer& operator=(FSUnrealMcpServer&&) = delete;

    bool Start(
        const FSUnrealMcpServerConfig& InConfig,
        const TSharedRef<FSUnrealMcpCommandRegistry>& InRegistry,
        const TSharedRef<FSUnrealMcpTaskRegistry>& InTaskRegistry);
    void Stop();

private:
    struct FClientConnection
    {
        explicit FClientConnection(FSocket* InSocket)
            : Socket(InSocket)
        {
        }

        FSocket* Socket = nullptr;
        FString PendingText;
        TArray<uint8> PendingSendBytes;
        int32 PendingSendOffset = 0;
    };

    bool Tick(float DeltaTime);
    void AcceptPendingConnections();
    void ServiceConnections();
    void ProcessIncomingText(FClientConnection& Connection, const FString& TextChunk);
    bool FlushPendingSends(FClientConnection& Connection);
    bool SendResponseLine(FClientConnection& Connection, const FString& ResponseLine, const FString& RequestId);
    void RemoveConnectionAt(int32 Index);

    TSharedPtr<FSUnrealMcpCommandRegistry> CommandRegistry;
    TSharedPtr<FSUnrealMcpTaskRegistry> TaskRegistry;
    TArray<TUniquePtr<FClientConnection>> Connections;
    FSocket* ListenSocket = nullptr;
    FTSTicker::FDelegateHandle TickHandle;
    FSUnrealMcpServerConfig Config;
};
