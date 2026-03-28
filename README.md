# SUnrealMcp

SUnrealMcp is a two-part Unreal Engine integration for the Model Context Protocol:

- `UnrealPlugin/SUnrealMcp`: a UE5 editor plugin that exposes Unreal editor operations through a lightweight TCP JSON protocol
- `McpServer`: a Node.js MCP server that connects to the plugin and re-publishes those capabilities as MCP tools

Together they let an MCP client drive common Unreal Editor workflows such as actor editing, Blueprint editing, UMG creation, node wiring, task polling, and project-side input configuration.

## What It Does

The current stack provides:

- actor queries and editing in the current editor level
- Blueprint creation, compilation, component editing, and selected property updates
- Blueprint graph operations such as adding event/function/input nodes and connecting pins
- UMG widget creation and editing
- task-style command support with polling and cancellation
- a typed MCP tool layer built with `@modelcontextprotocol/sdk`

At a high level, the flow looks like this:

1. An MCP client calls a tool exposed by `McpServer`.
2. `McpServer` converts that tool call into a protocol request and sends it over TCP.
3. The Unreal plugin executes the matching command inside the editor.
4. The result is returned to `McpServer` and then surfaced back to the MCP client.

## Repository Layout

```text
SUnrealMcp/
├─ McpServer/                  # Node.js MCP server
│  ├─ src/
│  │  ├─ index.ts              # server bootstrap
│  │  ├─ protocol.ts           # request/response envelope
│  │  ├─ transport/            # TCP client talking to Unreal
│  │  ├─ runtime/              # tool registration helpers
│  │  └─ tools/                # MCP tool definitions
│  └─ tests/
├─ UnrealPlugin/
│  └─ SUnrealMcp/              # Unreal editor plugin
│     ├─ Source/SUnrealMcp/Public/Mcp/
│     └─ Source/SUnrealMcp/Private/
└─ README.md
```

## Requirements

- Unreal Engine 5.x editor
- Node.js 18+ recommended
- Windows development environment is the primary tested setup in this repo

## Deploying the Unreal Plugin

1. Copy `UnrealPlugin/SUnrealMcp` into your Unreal project's `Plugins/` directory.

   Resulting layout:

   ```text
   YourProject/
   └─ Plugins/
      └─ SUnrealMcp/
   ```

2. Open the project in Unreal Editor.
3. Let Unreal compile the plugin, or build the editor target from Visual Studio / `Build.bat`.
4. In Unreal Editor, check the plugin settings under `Project Settings` / `Editor Preferences` depending on your setup and confirm:
   - bind address
   - port
   - auto-start setting
   - completed task retention

The plugin settings class is defined in [SUnrealMcpSettings.h](/D:/Projects/AGLS%20v1.7.0/_external/SUnrealMcp/UnrealPlugin/SUnrealMcp/Source/SUnrealMcp/Public/SUnrealMcpSettings.h).

Default values include:

- `BindAddress = 127.0.0.1`
- `Port = 55557`
- `bAutoStartServer = true`

## Deploying the MCP Server

1. Go to `McpServer/`.
2. Install dependencies:

```bash
npm install
```

3. Build the server:

```bash
npm run build
```

4. Start it from the compiled output:

```bash
node ./build/index.js
```

The MCP server reads configuration from:

- `McpServer/config.json`
- `McpServer/config.local.json`

The default connection settings in the server bootstrap are:

- host: `127.0.0.1`
- port: `55557`
- timeout: `5000ms`
- task timeout: `30000ms`

These defaults are defined in [index.ts](/D:/Projects/AGLS%20v1.7.0/_external/SUnrealMcp/McpServer/src/index.ts).

## End-to-End Bring-Up

Recommended startup order:

1. Start Unreal Editor with the plugin enabled.
2. Confirm the plugin TCP server is listening.
3. Build and start `McpServer`.
4. Connect your MCP client to the built server entrypoint.
5. Call `ping` first to verify the stack is healthy.

If `ping` works but editor-facing tools fail, the usual causes are:

- Unreal plugin not loaded
- host/port mismatch between plugin and `McpServer`
- editor world or runtime world not available for the requested command
- a tool definition exists in `McpServer`, but the corresponding plugin command is missing or outdated

## How Tool Calls Are Mapped

The two halves intentionally have different responsibilities:

- `McpServer/src/tools/*.ts`
  defines MCP-visible tool names, schemas, defaults, and task behavior
- Unreal plugin command classes
  implement actual editor behavior and expose command names through `GetCommandName()`

In the normal sync case:

1. `registerCommandTool(...)` registers a tool in `McpServer`
2. the tool name is used as the command name unless overridden
3. `McpServer` sends `{ protocolVersion, requestId, command, params }`
4. the plugin command registry dispatches to the matching Unreal command

## Extending the System

### Add a New MCP Tool

If you want to expose a new capability end-to-end, you usually need changes in both halves.

On the `McpServer` side:

1. Add or update a tool definition in `McpServer/src/tools/*.ts`.
2. Use `registerCommandTool(...)` from [tool_helpers.ts](/D:/Projects/AGLS%20v1.7.0/_external/SUnrealMcp/McpServer/src/runtime/tool_helpers.ts).
3. Define:
   - `name`
   - `description`
   - `inputSchema`
   - optional `mapParams`
   - optional `executionMode: "task"`
   - optional `taskOptions`
4. Build the server with `npm run build`.

Example pattern:

```ts
registerCommandTool(server, context, {
    name: "my_command",
    description: "Does something in Unreal",
    inputSchema: {
        asset_path: z.string(),
    },
});
```

### Add a New Unreal Command

On the plugin side:

1. Add a new command class under `UnrealPlugin/SUnrealMcp/Source/SUnrealMcp/Private/Commands/...`
2. Implement `ISUnrealMcpCommand`
3. Return the wire command name from `GetCommandName()`
4. Execute editor logic in `Execute(...)`
5. Register the command via `FSUnrealMcpCommandAutoRegistrar`

Skeleton pattern:

```cpp
class FMyCommand final : public ISUnrealMcpCommand
{
public:
    virtual FString GetCommandName() const override
    {
        return TEXT("my_command");
    }

    virtual FSUnrealMcpResponse Execute(
        const FSUnrealMcpRequest& Request,
        const FSUnrealMcpExecutionContext& Context) override
    {
        return FSUnrealMcpResponse::MakeSuccess(Request.RequestId, MakeShared<FJsonObject>());
    }
};
```

### Add a Task-Style Command

Use a task when work may span multiple ticks or should support polling/cancel.

Plugin side:

- implement an `ISUnrealMcpTask`
- enqueue it through `FSUnrealMcpTaskRegistry`
- return the accepted response with a `taskId`

Server side:

- set `executionMode: "task"` in the tool definition
- rely on `get_task_status` and `cancel_task`, or override those names in `taskOptions`

The current task helper flow is implemented in [tool_helpers.ts](/D:/Projects/AGLS%20v1.7.0/_external/SUnrealMcp/McpServer/src/runtime/tool_helpers.ts).

## Protocol Notes

The transport between `McpServer` and the Unreal plugin is:

- TCP
- UTF-8 JSON
- newline-delimited messages
- request/response envelope with `protocolVersion`, `requestId`, and `ok`

Core protocol definitions live in:

- [McpServer/src/protocol.ts](/D:/Projects/AGLS%20v1.7.0/_external/SUnrealMcp/McpServer/src/protocol.ts)
- [UnrealPlugin/SUnrealMcp/Source/SUnrealMcp/Public/Mcp/SUnrealMcpProtocol.h](/D:/Projects/AGLS%20v1.7.0/_external/SUnrealMcp/UnrealPlugin/SUnrealMcp/Source/SUnrealMcp/Public/Mcp/SUnrealMcpProtocol.h)

## Testing and Validation

Useful validation steps:

- `npm run build` in `McpServer/`
- `npm test` in `McpServer/`
- compile the Unreal plugin inside a UE project
- call `ping`
- call one simple editor command such as `get_actors_in_level`

There is also a plugin-level README in:

[UnrealPlugin/SUnrealMcp/README.md](/D:/Projects/AGLS%20v1.7.0/_external/SUnrealMcp/UnrealPlugin/SUnrealMcp/README.md)

## Troubleshooting

### `npm run build` fails

Check:

- Node.js version
- dependency install state
- tool module import paths
- whether the repo is being run from built output or source

### The MCP server starts but cannot talk to Unreal

Check:

- Unreal plugin loaded successfully
- plugin server auto-start enabled
- host/port match on both sides
- Windows firewall / local socket restrictions

### A tool exists in `McpServer` but fails with `UNKNOWN_COMMAND`

That usually means the plugin side is missing the matching Unreal command, or the command name in `GetCommandName()` does not match the tool's effective command name.

## Notes for Contributors

- Keep protocol and transport changes minimal unless there is a clear compatibility reason.
- Prefer adding new capabilities as commands/tools rather than changing the core envelope.
- When changing command semantics, review both:
  - the MCP tool definition
  - the Unreal command implementation
- If you add task commands, verify accepted response, status response, cancellation behavior, and timeout handling together.

## License

See [LICENSE](/D:/Projects/AGLS%20v1.7.0/_external/SUnrealMcp/LICENSE).
