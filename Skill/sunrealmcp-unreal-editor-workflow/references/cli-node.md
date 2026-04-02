# CLI Node Commands

Use this file only for Blueprint graph editing or graph inspection.

CLI help for this family should be available through:

- `sunrealmcp-cli help node`
- `sunrealmcp-cli node --help`

## Stable

### `node add-event`

Parameters:

- `--blueprint-path <value>`
- `--event-name <value>`
- `--node-position <x,y>` optional, default `0,0`

### `node add-function`

Parameters:

- `--blueprint-path <value>`
- `--target <value>`
- `--function-name <value>`
- `--params-json <json>` optional, default `{}`
- `--node-position <x,y>` optional, default `0,0`

### `node connect`

Parameters:

- `--blueprint-path <value>`
- `--source-node-id <value>`
- `--source-pin <value>`
- `--target-node-id <value>`
- `--target-pin <value>`

### `node add-variable`

Parameters:

- `--blueprint-path <value>`
- `--variable-name <value>`
- `--variable-type <Boolean|Integer|Float|Vector|Rotator|String|Name|Text>`
- `--is-exposed <true|false>` optional, default `false`

### `node find`

Parameters:

- `--blueprint-path <value>`
- `--node-type <value>` optional
- `--event-type <value>` optional

## Candidate

### `node inspect-graph`

Parameters:

- `--blueprint-path <value>`
- `--graph-type <value>` optional
- `--graph-name <value>` optional
- `--include-pins <true|false>` optional, default `true`

### `node add-input-action`

Parameters:

- `--blueprint-path <value>`
- `--input-action-path <value>`
- `--node-position <x,y>` optional, default `0,0`

### `node add-self-component-reference`

Parameters:

- `--blueprint-path <value>`
- `--component-name <value>`
- `--node-position <x,y>` optional, default `0,0`

### `node add-self-reference`

Parameters:

- `--blueprint-path <value>`
- `--node-position <x,y>` optional, default `0,0`
