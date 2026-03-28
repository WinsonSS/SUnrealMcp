#pragma once

#include "Containers/UnrealString.h"
#include "Dom/JsonObject.h"
#include "Misc/Optional.h"

#include "Mcp/SUnrealMcpProtocol.h"

enum class ESUnrealMcpTaskState : uint8
{
    Pending,
    Running,
    Succeeded,
    Failed,
    Cancelled
};

class SUNREALMCP_API ISUnrealMcpTask
{
public:
    virtual ~ISUnrealMcpTask() = default;

    virtual FString GetTaskName() const = 0;
    virtual void Start() = 0;
    virtual void Tick(float DeltaTime) = 0;
    virtual void Cancel() = 0;
    virtual ESUnrealMcpTaskState GetState() const = 0;
    virtual TSharedPtr<FJsonObject> BuildPayload() const = 0;
    virtual TOptional<FSUnrealMcpError> GetError() const = 0;
};
