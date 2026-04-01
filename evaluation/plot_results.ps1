param(
    [string]$ResultsDir = "evaluation/results",
    [string]$PlotsDir = "evaluation/plots"
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

New-Item -ItemType Directory -Force -Path $PlotsDir | Out-Null

function Get-Color {
    param([string]$Family)
    switch ($Family) {
        "convex_large" { "#0b6e4f" }
        "wavy_boundary" { "#c84c09" }
        "near_collinear" { "#005f99" }
        "many_holes" { "#7d4e99" }
        "narrow_gap" { "#9f1d35" }
        default { "#4d4d4d" }
    }
}

function Write-LineChartSvg {
    param(
        [string]$Path,
        [object[]]$Points,
        [object[]]$Series,
        [string]$Title,
        [string]$XAxis,
        [string]$YAxis,
        [string]$XField,
        [string]$YField
    )
    $width = 900; $height = 560; $left = 90; $right = 30; $top = 50; $bottom = 80
    $xMin = ($Points | Measure-Object -Property $XField -Minimum).Minimum
    $xMax = ($Points | Measure-Object -Property $XField -Maximum).Maximum
    $yMin = 0.0
    $yMax = ($Points | Measure-Object -Property $YField -Maximum).Maximum * 1.1
    if ($xMax -eq $xMin) { $xMax = $xMin + 1 }
    if ($yMax -eq $yMin) { $yMax = $yMin + 1 }

    function SX([double]$x) { $left + (($x - $xMin) / ($xMax - $xMin)) * ($width - $left - $right) }
    function SY([double]$y) { $height - $bottom - (($y - $yMin) / ($yMax - $yMin)) * ($height - $top - $bottom) }

    $svg = [System.Collections.Generic.List[string]]::new()
    $svg.Add("<svg xmlns='http://www.w3.org/2000/svg' width='$width' height='$height' viewBox='0 0 $width $height'>")
    $svg.Add("<rect width='100%' height='100%' fill='white'/>")
    $svg.Add("<text x='450' y='28' text-anchor='middle' font-size='20' font-family='Segoe UI'>$Title</text>")
    $svg.Add("<line x1='$left' y1='$($height - $bottom)' x2='$($width - $right)' y2='$($height - $bottom)' stroke='black' stroke-width='1.5'/>")
    $svg.Add("<line x1='$left' y1='$top' x2='$left' y2='$($height - $bottom)' stroke='black' stroke-width='1.5'/>")

    for ($i = 0; $i -le 5; $i++) {
        $xValue = $xMin + ($xMax - $xMin) * $i / 5.0
        $xCoord = SX $xValue
        $svg.Add("<line x1='$xCoord' y1='$($height - $bottom)' x2='$xCoord' y2='$($height - $bottom + 6)' stroke='black'/>")
        $svg.Add(("<text x='{0}' y='{1}' text-anchor='middle' font-size='11' font-family='Segoe UI'>{2:N0}</text>" -f $xCoord, ($height - $bottom + 24), $xValue))
    }
    for ($i = 0; $i -le 5; $i++) {
        $yValue = $yMin + ($yMax - $yMin) * $i / 5.0
        $yCoord = SY $yValue
        $svg.Add("<line x1='$($left - 6)' y1='$yCoord' x2='$left' y2='$yCoord' stroke='black'/>")
        $svg.Add(("<text x='{0}' y='{1}' text-anchor='end' font-size='11' font-family='Segoe UI'>{2:N2}</text>" -f ($left - 10), ($yCoord + 4), $yValue))
    }

    $svg.Add("<text x='450' y='545' text-anchor='middle' font-size='14' font-family='Segoe UI'>$XAxis</text>")
    $svg.Add("<text x='22' y='280' text-anchor='middle' transform='rotate(-90 22 280)' font-size='14' font-family='Segoe UI'>$YAxis</text>")

    foreach ($seriesEntry in $Series) {
        $sorted = $seriesEntry.points | Sort-Object { [double]($_.$XField) }
        if ($sorted.Count -ge 2) {
            $svgPath = "M " + (($sorted | ForEach-Object { "{0},{1}" -f (SX ([double]($_.$XField))), (SY ([double]($_.$YField))) }) -join " L ")
            $svg.Add("<path d='$svgPath' fill='none' stroke='$($seriesEntry.color)' stroke-width='2'/>")
        }
        foreach ($point in $sorted) {
            $svg.Add("<circle cx='$(SX ([double]($point.$XField)))' cy='$(SY ([double]($point.$YField)))' r='4' fill='$($seriesEntry.color)'/>")
        }
    }

    $legendX = $width - 220
    $legendY = 60
    foreach ($seriesEntry in $Series) {
        $svg.Add("<line x1='$legendX' y1='$legendY' x2='$($legendX + 18)' y2='$legendY' stroke='$($seriesEntry.color)' stroke-width='3'/>")
        $svg.Add("<text x='$($legendX + 25)' y='$($legendY + 4)' font-size='12' font-family='Segoe UI'>$($seriesEntry.label)</text>")
        $legendY += 20
    }

    $svg.Add("</svg>")
    Set-Content -LiteralPath $Path -Value $svg
}

$benchmark = Import-Csv -LiteralPath (Join-Path $ResultsDir "benchmark_summary.csv")
$displacement = Import-Csv -LiteralPath (Join-Path $ResultsDir "displacement_summary.csv")
$fits = Import-Csv -LiteralPath (Join-Path $ResultsDir "model_fits.csv")

function Flatten-SeriesPoints {
    param([object[]]$Series)
    $all = @()
    foreach ($seriesEntry in $Series) {
        $all += @($seriesEntry.points)
    }
    return ,$all
}

$runtimeSeries = foreach ($group in ($benchmark | Group-Object family)) {
    [pscustomobject]@{
        label = $group.Name
        color = Get-Color $group.Name
        points = @($group.Group | ForEach-Object { [pscustomobject]@{ input_vertices = [double]$_.input_vertices; value = [double]$_.avg_elapsed_ms } })
    }
}
$xMin = ($benchmark | Measure-Object input_vertices -Minimum).Minimum
$xMax = ($benchmark | Measure-Object input_vertices -Maximum).Maximum
$power = $fits | Where-Object { $_.metric -eq "runtime_ms" -and $_.model -eq "c*n^k" } | Select-Object -First 1
$nlogn = $fits | Where-Object { $_.metric -eq "runtime_ms" -and $_.model -eq "c*n*log(n)" } | Select-Object -First 1
$fitRuntimePower = @()
$fitRuntimeNlogn = @()
for ($n = [int]$xMin; $n -le [int]$xMax; $n += 8) {
    $fitRuntimePower += [pscustomobject]@{ input_vertices = [double]$n; value = [double]$power.c * [Math]::Pow($n, [double]$power.k) }
    $fitRuntimeNlogn += [pscustomobject]@{ input_vertices = [double]$n; value = [double]$nlogn.c * $n * [Math]::Log([Math]::Max($n, 2)) }
}
$runtimeSeries += [pscustomobject]@{ label = "fit: c*n^k"; color = "#111111"; points = $fitRuntimePower }
$runtimeSeries += [pscustomobject]@{ label = "fit: c*n*log(n)"; color = "#888888"; points = $fitRuntimeNlogn }
$runtimePoints = Flatten-SeriesPoints $runtimeSeries
Write-LineChartSvg -Path (Join-Path $PlotsDir "runtime_vs_input_size.svg") -Points $runtimePoints -Series $runtimeSeries -Title "Running time vs input size" -XAxis "Input vertices" -YAxis "Average runtime (ms)" -XField "input_vertices" -YField "value"

$memorySeries = foreach ($group in ($benchmark | Group-Object family)) {
    [pscustomobject]@{
        label = $group.Name
        color = Get-Color $group.Name
        points = @($group.Group | ForEach-Object { [pscustomobject]@{ input_vertices = [double]$_.input_vertices; value = ([double]$_.avg_peak_memory_bytes / 1MB) } })
    }
}
$powerMem = $fits | Where-Object { $_.metric -eq "peak_memory_bytes" -and $_.model -eq "c*n^k" } | Select-Object -First 1
$nlognMem = $fits | Where-Object { $_.metric -eq "peak_memory_bytes" -and $_.model -eq "c*n*log(n)" } | Select-Object -First 1
$fitMemPower = @()
$fitMemNlogn = @()
for ($n = [int]$xMin; $n -le [int]$xMax; $n += 8) {
    $fitMemPower += [pscustomobject]@{ input_vertices = [double]$n; value = ([double]$powerMem.c * [Math]::Pow($n, [double]$powerMem.k) / 1MB) }
    $fitMemNlogn += [pscustomobject]@{ input_vertices = [double]$n; value = ([double]$nlognMem.c * $n * [Math]::Log([Math]::Max($n, 2)) / 1MB) }
}
$memorySeries += [pscustomobject]@{ label = "fit: c*n^k"; color = "#111111"; points = $fitMemPower }
$memorySeries += [pscustomobject]@{ label = "fit: c*n*log(n)"; color = "#888888"; points = $fitMemNlogn }
$memoryPoints = Flatten-SeriesPoints $memorySeries
Write-LineChartSvg -Path (Join-Path $PlotsDir "memory_vs_input_size.svg") -Points $memoryPoints -Series $memorySeries -Title "Peak memory vs input size" -XAxis "Input vertices" -YAxis "Average peak memory (MiB)" -XField "input_vertices" -YField "value"

$dispSeries = foreach ($group in ($displacement | Group-Object dataset_name)) {
    [pscustomobject]@{
        label = $group.Name
        color = Get-Color $group.Group[0].family
        points = @($group.Group | ForEach-Object { [pscustomobject]@{ target_vertices = [double]$_.target_vertices; value = [double]$_.avg_areal_displacement } })
    }
}
$dispPoints = Flatten-SeriesPoints $dispSeries
Write-LineChartSvg -Path (Join-Path $PlotsDir "areal_displacement_vs_target.svg") -Points $dispPoints -Series $dispSeries -Title "Areal displacement vs target vertex count" -XAxis "Target vertices" -YAxis "Average areal displacement" -XField "target_vertices" -YField "value"

Write-Host "Wrote SVG plots to $PlotsDir"
