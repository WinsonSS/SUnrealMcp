param(
    [Parameter(ValueFromRemainingArguments = $true)]
    [string[]]$Args
)

$ScriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$CliPath = Join-Path $ScriptDir "..\\dist\\sunrealmcp-cli.js"

node $CliPath @Args
