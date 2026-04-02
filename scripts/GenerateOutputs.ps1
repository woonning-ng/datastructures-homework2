param(
    [switch]$NoBuild,
    [int]$TimeoutSeconds = 30
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

. (Join-Path $PSScriptRoot "TestWorkflow.ps1")

$exitCode = Invoke-GenerateOutputs -NoBuild:$NoBuild -TimeoutSeconds $TimeoutSeconds
exit $exitCode
