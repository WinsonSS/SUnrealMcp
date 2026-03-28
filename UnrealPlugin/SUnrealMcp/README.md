# SUnrealMcp Unreal Plugin

This directory contains the UE5 editor plugin side of SUnrealMcp.

Plugin root:
- `SUnrealMcp.uplugin`
- `Source/SUnrealMcp/...`

Current foundation:
- Editor-only module
- TCP listener on `127.0.0.1:55557`
- newline-delimited JSON protocol
- command registry for easy extension
- starter commands:
  - `ping`
  - `get_actors_in_level`
  - `spawn_actor`

To integrate into a UE5 project:
1. Copy this entire `UnrealPlugin` directory into your project's `Plugins/SUnrealMcp/` directory.
2. Regenerate project files if needed.
3. Build the editor target.

Suggested extension flow:
- Add a new command class under `Private/Commands`
- Register it in `FSUnrealMcpModule::StartupModule()`
- Keep transport/protocol untouched unless the protocol changes
