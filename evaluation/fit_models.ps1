param(
    [string]$ResultsDir = "evaluation/results"
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

function Get-Mean {
    param([double[]]$Values)
    $sum = 0.0
    foreach ($value in $Values) {
        $sum += $value
    }
    return $sum / [Math]::Max(1, $Values.Count)
}

function Fit-PowerLaw {
    param([double[]]$X, [double[]]$Y)
    $logX = @()
    $logY = @()
    for ($i = 0; $i -lt $X.Count; $i++) {
        $logX += [Math]::Log([Math]::Max($X[$i], 1e-12))
        $logY += [Math]::Log([Math]::Max($Y[$i], 1e-12))
    }

    $meanX = Get-Mean $logX
    $meanY = Get-Mean $logY
    $num = 0.0
    $den = 0.0
    for ($i = 0; $i -lt $logX.Count; $i++) {
        $num += ($logX[$i] - $meanX) * ($logY[$i] - $meanY)
        $den += [Math]::Pow($logX[$i] - $meanX, 2)
    }

    $k = if ($den -eq 0.0) { 0.0 } else { $num / $den }
    $logC = $meanY - $k * $meanX
    return [pscustomobject]@{ c = [Math]::Exp($logC); k = $k }
}

function Fit-NLogN {
    param([double[]]$X, [double[]]$Y)
    $num = 0.0
    $den = 0.0
    for ($i = 0; $i -lt $X.Count; $i++) {
        $basis = $X[$i] * [Math]::Log([Math]::Max($X[$i], 2.0))
        $num += $basis * $Y[$i]
        $den += $basis * $basis
    }
    return [pscustomobject]@{ c = if ($den -eq 0.0) { 0.0 } else { $num / $den } }
}

function Get-FitMetrics {
    param(
        [double[]]$Observed,
        [double[]]$Predicted
    )
    $mean = Get-Mean $Observed
    $ssRes = 0.0
    $ssTot = 0.0
    for ($i = 0; $i -lt $Observed.Count; $i++) {
        $ssRes += [Math]::Pow($Observed[$i] - $Predicted[$i], 2)
        $ssTot += [Math]::Pow($Observed[$i] - $mean, 2)
    }
    return [pscustomobject]@{
        rmse = [Math]::Sqrt($ssRes / [Math]::Max(1, $Observed.Count))
        r_squared = if ($ssTot -eq 0.0) { 1.0 } else { 1.0 - ($ssRes / $ssTot) }
    }
}

$summaryPath = Join-Path $ResultsDir "benchmark_summary.csv"
if (-not (Test-Path $summaryPath)) {
    throw "Benchmark summary missing at $summaryPath"
}

$rows = Import-Csv -LiteralPath $summaryPath | Where-Object { $_.family -ne "scaled_irregular" } | Sort-Object {[int]$_.input_vertices}, dataset_name
$x = @($rows | ForEach-Object { [double]$_.input_vertices })
$fits = [System.Collections.Generic.List[object]]::new()

foreach ($metric in @(
    @{ Name = "runtime_ms"; Column = "avg_elapsed_ms"; Scale = 1.0 },
    @{ Name = "peak_memory_bytes"; Column = "avg_peak_memory_bytes"; Scale = 1.0 }
)) {
    $columnName = [string]$metric.Column
    $y = @($rows | ForEach-Object { [double]$_.PSObject.Properties[$columnName].Value * [double]$metric.Scale })

    $power = Fit-PowerLaw -X $x -Y $y
    $powerPred = @()
    foreach ($n in $x) {
        $powerPred += $power.c * [Math]::Pow($n, $power.k)
    }
    $powerMetrics = Get-FitMetrics -Observed $y -Predicted $powerPred
    $fits.Add([pscustomobject]@{
        metric = $metric.Name
        model = "c*n^k"
        c = [Math]::Round($power.c, 12)
        k = [Math]::Round($power.k, 6)
        rmse = [Math]::Round($powerMetrics.rmse, 6)
        r_squared = [Math]::Round($powerMetrics.r_squared, 6)
    })

    $nlogn = Fit-NLogN -X $x -Y $y
    $nlognPred = @()
    foreach ($n in $x) {
        $nlognPred += $nlogn.c * $n * [Math]::Log([Math]::Max($n, 2.0))
    }
    $nlognMetrics = Get-FitMetrics -Observed $y -Predicted $nlognPred
    $fits.Add([pscustomobject]@{
        metric = $metric.Name
        model = "c*n*log(n)"
        c = [Math]::Round($nlogn.c, 12)
        k = ""
        rmse = [Math]::Round($nlognMetrics.rmse, 6)
        r_squared = [Math]::Round($nlognMetrics.r_squared, 6)
    })
}

$fits | Export-Csv -NoTypeInformation -LiteralPath (Join-Path $ResultsDir "model_fits.csv")

$lines = @(
    "# Model Fits",
    "",
    "| Metric | Model | c | k | RMSE | R^2 |",
    "|---|---|---:|---:|---:|---:|"
)
foreach ($row in $fits) {
    $lines += "| $($row.metric) | $($row.model) | $($row.c) | $($row.k) | $($row.rmse) | $($row.r_squared) |"
}
Set-Content -LiteralPath (Join-Path $ResultsDir "model_fits.md") -Value $lines
Write-Host "Wrote model fits to $ResultsDir"
