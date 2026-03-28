#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"

#include "SUnrealMcpSettings.generated.h"

UCLASS(Config=EditorPerProjectUserSettings, DefaultConfig, meta=(DisplayName="SUnrealMcp"))
class SUNREALMCP_API USUnrealMcpSettings : public UDeveloperSettings
{
    GENERATED_BODY()

public:
    USUnrealMcpSettings();

    virtual FName GetCategoryName() const override;

    UPROPERTY(Config, EditAnywhere, Category="Server")
    FString BindAddress = TEXT("127.0.0.1");

    UPROPERTY(Config, EditAnywhere, Category="Server", meta=(ClampMin="1", ClampMax="65535"))
    int32 Port = 55557;

    UPROPERTY(Config, EditAnywhere, Category="Server", meta=(ClampMin="1", ClampMax="128"))
    int32 ListenBacklog = 8;

    UPROPERTY(Config, EditAnywhere, Category="Server")
    bool bAutoStartServer = true;

    UPROPERTY(Config, EditAnywhere, Category="Tasks", meta=(ClampMin="1.0", ClampMax="3600.0"))
    double CompletedTaskRetentionSeconds = 300.0;
};
