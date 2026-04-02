# CLI UMG Commands

Use this file only for Widget Blueprint creation, widget-tree edits, or widget binding operations.

CLI help for this family should be available through:

- `sunrealmcp-cli help umg`
- `sunrealmcp-cli umg --help`

## Stable

### `umg create-widget-blueprint`

Parameters:

- `--widget-path <value>`
- `--parent-class <value>` optional, default `UserWidget`

## Candidate

### `umg add-text-block`

Parameters:

- `--widget-path <value>`
- `--text-block-name <value>`
- `--text <value>` optional
- `--position <x,y>` optional
- `--size <width,height>` optional
- `--font-size <number>` optional
- `--color <r,g,b,a>` optional

### `umg add-button`

Parameters:

- `--widget-path <value>`
- `--button-name <value>`
- `--text <value>` optional
- `--position <x,y>` optional
- `--size <width,height>` optional
- `--font-size <number>` optional
- `--color <r,g,b,a>` optional
- `--background-color <r,g,b,a>` optional

### `umg bind-event`

Parameters:

- `--widget-path <value>`
- `--widget-component-name <value>`
- `--event-name <value>`
- `--function-name <value>` optional

### `umg add-to-viewport`

Parameters:

- `--widget-path <value>`
- `--z-order <number>` optional, default `0`

### `umg set-text-block-binding`

Parameters:

- `--widget-path <value>`
- `--text-block-name <value>`
- `--binding-property <value>`
- `--binding-type <value>` optional, default `Text`
