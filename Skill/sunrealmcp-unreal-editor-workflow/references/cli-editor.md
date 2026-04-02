# CLI Editor Commands

Use this file only for actor and level-editing operations.

CLI help for this family should be available through:

- `sunrealmcp-cli help editor`
- `sunrealmcp-cli editor --help`

## Stable

### `editor get-actors-in-level`

Parameters:

- global target options only

### `editor find-actors-by-label`

Parameters:

- `--pattern <value>`

### `editor spawn-actor`

Parameters:

- `--actor-label <value>`
- `--actor-class <value>`
- `--location <x,y,z>` optional, default `0,0,0`
- `--rotation <pitch,yaw,roll>` optional, default `0,0,0`

### `editor spawn-blueprint-actor`

Parameters:

- `--blueprint-path <value>`
- `--actor-label <value>`
- `--location <x,y,z>` optional, default `0,0,0`
- `--rotation <pitch,yaw,roll>` optional, default `0,0,0`

### `editor delete-actor`

Parameters:

- `--actor-path <value>`

### `editor get-actor-properties`

Parameters:

- `--actor-path <value>`

### `editor set-actor-property`

Parameters:

- `--actor-path <value>`
- `--property-name <value>`
- `--property-json <json>`

### `editor set-actor-transform`

Parameters:

- `--actor-path <value>`
- `--location <x,y,z>` optional
- `--rotation <pitch,yaw,roll>` optional
- `--scale <x,y,z>` optional
