param(
    [string]$DatasetDir = "evaluation/datasets",
    [string]$ResultsDir = "evaluation/results",
    [int]$Repeats = 2,
    [switch]$Rebuild
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

$repoRoot = Split-Path -Parent $PSScriptRoot
Set-Location $repoRoot

& (Join-Path $PSScriptRoot "build_simplify.ps1") @(@{ Rebuild = $Rebuild.IsPresent }.GetEnumerator() | Where-Object { $_.Value } | ForEach-Object { "-$($_.Key)" })

$exePath = if ($IsWindows) { Join-Path $repoRoot "simplify.exe" } else { Join-Path $repoRoot "simplify" }
if (-not (Test-Path $exePath)) {
    throw "Unable to find built executable at $exePath"
}

New-Item -ItemType Directory -Force -Path $ResultsDir | Out-Null
$manifestPath = Join-Path $DatasetDir "manifest.csv"
if (-not (Test-Path $manifestPath)) {
    throw "Dataset manifest missing at $manifestPath. Run generate_datasets.ps1 first."
}
$manifest = Import-Csv -LiteralPath $manifestPath

function Get-MinimumPracticalTarget {
    param([int]$InputVertices, [int]$RingCount, [double]$Ratio)
    $target = [int][Math]::Floor($InputVertices * $Ratio)
    return [Math]::Max($RingCount * 3, $target)
}

function Parse-SimplifyOutput {
    param([string[]]$Lines)
    $vertexLines = $Lines | Where-Object { $_ -match '^\d+,\d+,' }
    $inputAreaLine = $Lines | Where-Object { $_ -like 'Total signed area in input:*' } | Select-Object -First 1
    $outputAreaLine = $Lines | Where-Object { $_ -like 'Total signed area in output:*' } | Select-Object -First 1
    $displacementLine = $Lines | Where-Object { $_ -like 'Total areal displacement:*' } | Select-Object -First 1
    [pscustomobject]@{
        actual_output_vertices = $vertexLines.Count
        input_area = if ($inputAreaLine) { [double]($inputAreaLine.Split(':')[1].Trim()) } else { [double]::NaN }
        output_area = if ($outputAreaLine) { [double]($outputAreaLine.Split(':')[1].Trim()) } else { [double]::NaN }
        areal_displacement = if ($displacementLine) { [double]($displacementLine.Split(':')[1].Trim()) } else { [double]::NaN }
    }
}

function Invoke-Benchmark {
    param(
        [string]$Executable,
        [string]$DatasetPath,
        [int]$TargetVertices
    )

    $stdoutPath = Join-Path $env:TEMP ("simplify_stdout_{0}.txt" -f [Guid]::NewGuid().ToString("N"))
    $stderrPath = Join-Path $env:TEMP ("simplify_stderr_{0}.txt" -f [Guid]::NewGuid().ToString("N"))
    $statsPath = $null
    try {
        if (($IsLinux -or $IsMacOS) -and (Test-Path "/usr/bin/time")) {
            $statsPath = Join-Path $env:TEMP ("simplify_time_{0}.txt" -f [Guid]::NewGuid().ToString("N"))
            $process = Start-Process -FilePath "/usr/bin/time" -ArgumentList @("-v", "-o", $statsPath, $Executable, $DatasetPath, "$TargetVertices") -NoNewWindow -Wait -PassThru -RedirectStandardOutput $stdoutPath -RedirectStandardError $stderrPath
            $stats = Get-Content -LiteralPath $statsPath
            $peakKb = (($stats | Where-Object { $_ -match '^Maximum resident set size' }) -split ':' | Select-Object -Last 1).Trim()
            $elapsedText = (($stats | Where-Object { $_ -match '^Elapsed \(wall clock\) time' }) -split ':' | Select-Object -Last 1).Trim()
            $parsed = Parse-SimplifyOutput -Lines (Get-Content -LiteralPath $stdoutPath)
            return [pscustomobject]@{
                exit_code = $process.ExitCode
                elapsed_ms = [Math]::Round([TimeSpan]::Parse($elapsedText).TotalMilliseconds, 3)
                peak_memory_bytes = [int64]$peakKb * 1024
                parsed = $parsed
            }
        }

        $stopwatch = [System.Diagnostics.Stopwatch]::StartNew()
        $process = Start-Process -FilePath $Executable -ArgumentList @($DatasetPath, "$TargetVertices") -NoNewWindow -PassThru -RedirectStandardOutput $stdoutPath -RedirectStandardError $stderrPath
        $peakWorkingSet = 0L
        while (-not $process.HasExited) {
            try {
                $process.Refresh()
                if ($process.WorkingSet64 -gt $peakWorkingSet) {
                    $peakWorkingSet = $process.WorkingSet64
                }
            } catch {
            }
            Start-Sleep -Milliseconds 10
        }
        $process.Refresh()
        if ($process.WorkingSet64 -gt $peakWorkingSet) {
            $peakWorkingSet = $process.WorkingSet64
        }
        $stopwatch.Stop()
        $parsed = Parse-SimplifyOutput -Lines (Get-Content -LiteralPath $stdoutPath)
        return [pscustomobject]@{
            exit_code = $process.ExitCode
            elapsed_ms = [Math]::Round($stopwatch.Elapsed.TotalMilliseconds, 3)
            peak_memory_bytes = [int64]$peakWorkingSet
            parsed = $parsed
        }
    }
    finally {
        foreach ($path in @($stdoutPath, $stderrPath, $statsPath)) {
            if ($path -and (Test-Path $path)) {
                Remove-Item $path -Force
            }
        }
    }
}

$raw = [System.Collections.Generic.List[object]]::new()
$defaultSummary = [System.Collections.Generic.List[object]]::new()
$sweepRaw = [System.Collections.Generic.List[object]]::new()
$sweepSummary = [System.Collections.Generic.List[object]]::new()

foreach ($dataset in $manifest) {
    $datasetFile = Join-Path (Join-Path $repoRoot $DatasetDir) ("{0}.csv" -f $dataset.dataset_name)
    $target = Get-MinimumPracticalTarget -InputVertices ([int]$dataset.input_vertices) -RingCount ([int]$dataset.ring_count) -Ratio ([double]$dataset.default_target_ratio)
    $rows = @()
    for ($run = 1; $run -le $Repeats; $run++) {
        $result = Invoke-Benchmark -Executable $exePath -DatasetPath $datasetFile -TargetVertices $target
        $status = if ($result.exit_code -ne 0) { "error" } elseif ($result.parsed.actual_output_vertices -le $target) { "reached_target" } else { "stopped_early" }
        $row = [pscustomobject]@{
            benchmark_type = "default_target"
            dataset_name = $dataset.dataset_name
            family = $dataset.family
            input_vertices = [int]$dataset.input_vertices
            ring_count = [int]$dataset.ring_count
            hole_count = [int]$dataset.hole_count
            target_vertices = $target
            target_ratio = [double]$dataset.default_target_ratio
            run_index = $run
            elapsed_ms = $result.elapsed_ms
            peak_memory_bytes = $result.peak_memory_bytes
            actual_output_vertices = $result.parsed.actual_output_vertices
            input_area = $result.parsed.input_area
            output_area = $result.parsed.output_area
            areal_displacement = $result.parsed.areal_displacement
            status = $status
        }
        $rows += $row
        $raw.Add($row)
    }

    $defaultSummary.Add([pscustomobject]@{
        dataset_name = $dataset.dataset_name
        family = $dataset.family
        input_vertices = [int]$dataset.input_vertices
        ring_count = [int]$dataset.ring_count
        hole_count = [int]$dataset.hole_count
        target_vertices = $target
        target_ratio = [double]$dataset.default_target_ratio
        avg_elapsed_ms = [Math]::Round((($rows | Measure-Object elapsed_ms -Average).Average), 3)
        avg_peak_memory_bytes = [int64][Math]::Round((($rows | Measure-Object peak_memory_bytes -Average).Average), 0)
        avg_areal_displacement = [Math]::Round((($rows | Measure-Object areal_displacement -Average).Average), 9)
        avg_actual_output_vertices = [Math]::Round((($rows | Measure-Object actual_output_vertices -Average).Average), 3)
        statuses = (($rows | Select-Object -ExpandProperty status | Sort-Object -Unique) -join ";")
    })
}

$sweepTargets = @(0.90, 0.75, 0.50, 0.35, 0.25)
foreach ($dataset in ($manifest | Where-Object { $_.sweep_focus -eq "yes" })) {
    $datasetFile = Join-Path (Join-Path $repoRoot $DatasetDir) ("{0}.csv" -f $dataset.dataset_name)
    foreach ($ratio in $sweepTargets) {
        $target = Get-MinimumPracticalTarget -InputVertices ([int]$dataset.input_vertices) -RingCount ([int]$dataset.ring_count) -Ratio $ratio
        $rows = @()
        for ($run = 1; $run -le $Repeats; $run++) {
            $result = Invoke-Benchmark -Executable $exePath -DatasetPath $datasetFile -TargetVertices $target
            $status = if ($result.exit_code -ne 0) { "error" } elseif ($result.parsed.actual_output_vertices -le $target) { "reached_target" } else { "stopped_early" }
            $row = [pscustomobject]@{
                benchmark_type = "target_sweep"
                dataset_name = $dataset.dataset_name
                family = $dataset.family
                input_vertices = [int]$dataset.input_vertices
                ring_count = [int]$dataset.ring_count
                hole_count = [int]$dataset.hole_count
                target_vertices = $target
                target_ratio = $ratio
                run_index = $run
                elapsed_ms = $result.elapsed_ms
                peak_memory_bytes = $result.peak_memory_bytes
                actual_output_vertices = $result.parsed.actual_output_vertices
                input_area = $result.parsed.input_area
                output_area = $result.parsed.output_area
                areal_displacement = $result.parsed.areal_displacement
                status = $status
            }
            $rows += $row
            $sweepRaw.Add($row)
        }

        $sweepSummary.Add([pscustomobject]@{
            dataset_name = $dataset.dataset_name
            family = $dataset.family
            input_vertices = [int]$dataset.input_vertices
            target_vertices = $target
            target_ratio = $ratio
            avg_elapsed_ms = [Math]::Round((($rows | Measure-Object elapsed_ms -Average).Average), 3)
            avg_peak_memory_bytes = [int64][Math]::Round((($rows | Measure-Object peak_memory_bytes -Average).Average), 0)
            avg_areal_displacement = [Math]::Round((($rows | Measure-Object areal_displacement -Average).Average), 9)
            avg_actual_output_vertices = [Math]::Round((($rows | Measure-Object actual_output_vertices -Average).Average), 3)
            statuses = (($rows | Select-Object -ExpandProperty status | Sort-Object -Unique) -join ";")
        })
    }
}

$raw | Export-Csv -NoTypeInformation -LiteralPath (Join-Path $ResultsDir "benchmark_raw.csv")
$defaultSummary | Sort-Object input_vertices, dataset_name | Export-Csv -NoTypeInformation -LiteralPath (Join-Path $ResultsDir "benchmark_summary.csv")
$sweepRaw | Export-Csv -NoTypeInformation -LiteralPath (Join-Path $ResultsDir "displacement_raw.csv")
$sweepSummary | Sort-Object dataset_name, target_vertices | Export-Csv -NoTypeInformation -LiteralPath (Join-Path $ResultsDir "displacement_summary.csv")

$tableLines = @(
    "# Benchmark Summary",
    "",
    "| Dataset | Family | Input vertices | Target | Avg runtime (ms) | Avg peak memory (MiB) | Avg areal displacement | Status |",
    "|---|---|---:|---:|---:|---:|---:|---|"
)
foreach ($row in ($defaultSummary | Sort-Object input_vertices, dataset_name)) {
    $tableLines += "| $($row.dataset_name) | $($row.family) | $($row.input_vertices) | $($row.target_vertices) | $($row.avg_elapsed_ms) | {0:N3} | {1:E6} | $($row.statuses) |" -f ($row.avg_peak_memory_bytes / 1MB), $row.avg_areal_displacement
}
Set-Content -LiteralPath (Join-Path $ResultsDir "benchmark_table.md") -Value $tableLines
Write-Host "Wrote benchmark CSVs and markdown tables to $ResultsDir"
