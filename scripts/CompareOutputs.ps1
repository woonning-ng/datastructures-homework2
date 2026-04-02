param(
    [int]$DiffLines = 40
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

. (Join-Path $PSScriptRoot "TestWorkflow.ps1")

$exitCode = Invoke-CompareOutputs -DiffLines $DiffLines
exit $exitCode
