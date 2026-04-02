param(
    [switch]$NoBuild,
    [int]$TimeoutSeconds = 30,
    [int]$DiffLines = 40
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

. (Join-Path $PSScriptRoot "TestWorkflow.ps1")

$generateExit = Invoke-GenerateOutputs -NoBuild:$NoBuild -TimeoutSeconds $TimeoutSeconds
if ($generateExit -ne 0) {
    exit $generateExit
}

$compareExit = Invoke-CompareOutputs -DiffLines $DiffLines
exit $compareExit
