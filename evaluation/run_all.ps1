Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

& (Join-Path $PSScriptRoot "generate_datasets.ps1")
& (Join-Path $PSScriptRoot "run_benchmarks.ps1")
& (Join-Path $PSScriptRoot "fit_models.ps1")
& (Join-Path $PSScriptRoot "plot_results.ps1")

Write-Host "Full evaluation pipeline completed."
