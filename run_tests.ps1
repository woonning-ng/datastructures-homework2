Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

$repoRoot = $PSScriptRoot
Set-Location $repoRoot

$exe = if (Test-Path ".\simplify.exe") { ".\simplify.exe" } elseif (Test-Path ".\simplify") { ".\simplify" } else { $null }
if (-not $exe) {
    throw "Could not find simplify executable. Build the project first."
}

$cases = @(
    @{ Input = "test_cases/input_rectangle_with_two_holes.csv"; Target = 11; Expected = "test_cases/output_rectangle_with_two_holes.txt" },
    @{ Input = "test_cases/input_cushion_with_hexagonal_hole.csv"; Target = 13; Expected = "test_cases/output_cushion_with_hexagonal_hole.txt" },
    @{ Input = "test_cases/input_blob_with_two_holes.csv"; Target = 17; Expected = "test_cases/output_blob_with_two_holes.txt" },
    @{ Input = "test_cases/input_wavy_with_three_holes.csv"; Target = 21; Expected = "test_cases/output_wavy_with_three_holes.txt" },
    @{ Input = "test_cases/input_original_01.csv"; Target = 99; Expected = "test_cases/output_original_01.txt" },
    @{ Input = "test_cases/input_original_02.csv"; Target = 99; Expected = "test_cases/output_original_02.txt" },
    @{ Input = "test_cases/input_original_03.csv"; Target = 99; Expected = "test_cases/output_original_03.txt" },
    @{ Input = "test_cases/input_original_04.csv"; Target = 99; Expected = "test_cases/output_original_04.txt" },
    @{ Input = "test_cases/input_original_05.csv"; Target = 99; Expected = "test_cases/output_original_05.txt" },
    @{ Input = "test_cases/input_original_06.csv"; Target = 99; Expected = "test_cases/output_original_06.txt" },
    @{ Input = "test_cases/input_original_07.csv"; Target = 99; Expected = "test_cases/output_original_07.txt" },
    @{ Input = "test_cases/input_original_08.csv"; Target = 99; Expected = "test_cases/output_original_08.txt" },
    @{ Input = "test_cases/input_original_09.csv"; Target = 99; Expected = "test_cases/output_original_09.txt" },
    @{ Input = "test_cases/input_original_10.csv"; Target = 99; Expected = "test_cases/output_original_10.txt" }
)

$passCount = 0
$failCount = 0

foreach ($case in $cases) {
    Write-Host ("TEST  {0}  target={1}" -f $case.Input, $case.Target)

    $actual = & $exe $case.Input $case.Target
    $expected = Get-Content -LiteralPath $case.Expected
    $diff = Compare-Object -ReferenceObject $expected -DifferenceObject $actual -SyncWindow 0

    if ($null -eq $diff) {
        Write-Host "PASS" -ForegroundColor Green
        $passCount++
    } else {
        Write-Host "FAIL" -ForegroundColor Red
        $diff | Select-Object -First 12 | Format-Table -AutoSize
        $failCount++
    }
}

Write-Host ""
Write-Host ("Passed: {0}" -f $passCount)
Write-Host ("Failed: {0}" -f $failCount)

if ($failCount -gt 0) {
    exit 1
}
