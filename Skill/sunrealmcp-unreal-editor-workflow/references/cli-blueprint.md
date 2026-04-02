# CLI Blueprint Commands

Use this file only for Blueprint asset creation, compilation, component edits, and Blueprint property changes.

CLI help for this family should be available through:

- `sunrealmcp-cli help blueprint`
- `sunrealmcp-cli blueprint --help`

## Stable

### `blueprint create`

Parameters:

- `--blueprint-path <value>`
- `--parent-class <value>`

### `blueprint add-component`

Parameters:

- `--blueprint-path <value>`
- `--component-type <value>`
- `--component-name <value>`
- `--location <x,y,z>` optional
- `--rotation <pitch,yaw,roll>` optional
- `--scale <x,y,z>` optional
- `--component-properties-json <json>` optional

### `blueprint compile`

Parameters:

- `--blueprint-path <value>`

### `blueprint set-property`

Parameters:

- `--blueprint-path <value>`
- `--property-name <value>`
- `--property-json <json>`

### `blueprint set-component-property`

Parameters:

- `--blueprint-path <value>`
- `--component-name <value>`
- `--property-name <value>`
- `--property-json <json>`

## Candidate

### `blueprint set-static-mesh-properties`

Parameters:

- `--blueprint-path <value>`
- `--component-name <value>`
- `--static-mesh <value>`

### `blueprint set-physics-properties`

Parameters:

- `--blueprint-path <value>`
- `--component-name <value>`
- `--simulate-physics <true|false>`
- `--gravity-enabled <true|false>`
- `--mass <number>`
- `--linear-damping <number>`
- `--angular-damping <number>`
