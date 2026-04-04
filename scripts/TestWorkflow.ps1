Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

function Get-RepositoryRoot {
    return (Resolve-Path (Join-Path $PSScriptRoot "..")).Path
}

function Convert-ToBashPath {
    param(
        [Parameter(Mandatory = $true)]
        [string]$Path
    )

    $resolvedPath = (Resolve-Path -LiteralPath $Path).Path
    if ($resolvedPath -match '^([A-Za-z]):\\(.*)$') {
        $drive = $matches[1].ToLower()
        $tail = $matches[2] -replace '\\', '/'
        return "/mnt/$drive/$tail"
    }

    throw "Cannot convert path to bash format: $Path"
}

function Get-TestCases {
    $repoRoot = Get-RepositoryRoot
    $testCasesDir = Join-Path $repoRoot "test_cases"
    $manifestPath = Join-Path $testCasesDir "README.md"
    $generatedDir = Join-Path $repoRoot "generated_outputs"

    if (-not (Test-Path -LiteralPath $manifestPath)) {
        throw "Missing test case manifest: $manifestPath"
    }

    $testCases = @()
    $headers = $null

    foreach ($rawLine in Get-Content -LiteralPath $manifestPath) {
        $line = $rawLine.Trim()

        if (-not $line.StartsWith("|")) {
            $headers = $null
            continue
        }

        $cells = @($line.Trim("|").Split("|") | ForEach-Object { $_.Trim() })
        if ($cells.Count -eq 0) {
            continue
        }

        if (($cells -contains "Input file") -and ($cells -contains "Output file")) {
            $headers = $cells
            continue
        }

        if ($null -eq $headers) {
            continue
        }

        $isSeparator = $true
        foreach ($cell in $cells) {
            if ($cell -notmatch '^[\-:]+$') {
                $isSeparator = $false
                break
            }
        }
        if ($isSeparator) {
            continue
        }

        $row = @{}
        for ($i = 0; $i -lt [Math]::Min($headers.Count, $cells.Count); $i++) {
            $row[$headers[$i]] = $cells[$i]
        }

        if (-not $row.ContainsKey("Input file") -or -not $row.ContainsKey("Target") -or -not $row.ContainsKey("Output file")) {
            continue
        }

        $inputName = $row["Input file"].Replace('`', '').Trim()
        $outputName = $row["Output file"].Replace('`', '').Trim()
        $targetText = $row["Target"].Replace('`', '').Trim()

        $inputPath = Join-Path $testCasesDir $inputName
        $expectedPath = Join-Path $testCasesDir $outputName
        $generatedPath = Join-Path $generatedDir ([System.IO.Path]::GetFileName($expectedPath))

        $testCases += [PSCustomObject]@{
            Name = [System.IO.Path]::GetFileNameWithoutExtension($inputPath)
            InputPath = $inputPath
            TargetVertices = [int]$targetText
            ExpectedPath = $expectedPath
            GeneratedPath = $generatedPath
        }
    }

    if ($testCases.Count -eq 0) {
        throw "No test cases discovered from $manifestPath"
    }

    return $testCases
}

function Get-ExecutablePath {
    $repoRoot = Get-RepositoryRoot
    $candidates = @(
        (Join-Path $repoRoot "simplify.exe"),
        (Join-Path $repoRoot "simplify")
    )

    foreach ($candidate in $candidates) {
        if (Test-Path -LiteralPath $candidate) {
            return $candidate
        }
    }

    throw "Could not find built executable 'simplify' or 'simplify.exe'."
}

function Read-FileBytesSafe {
    param(
        [Parameter(Mandatory = $true)]
        [string]$Path
    )

    try {
        return [System.IO.File]::ReadAllBytes($Path)
    }
    catch {
        return $null
    }
}

function Read-FileTextSafe {
    param(
        [Parameter(Mandatory = $true)]
        [string]$Path
    )

    try {
        return Get-Content -LiteralPath $Path -Raw
    }
    catch {
        return $null
    }
}

function Invoke-BuildProject {
    $repoRoot = Get-RepositoryRoot
    $bashRepoRoot = Convert-ToBashPath -Path $repoRoot

    & bash -lc "cd '$bashRepoRoot' && make"
    if ($LASTEXITCODE -ne 0) {
        throw "Build failed with exit code $LASTEXITCODE."
    }
}

function Invoke-GenerateOutputs {
    param(
        [switch]$NoBuild,
        [int]$TimeoutSeconds = 30
    )

    if (-not $NoBuild) {
        Invoke-BuildProject
    }

    $testCases = Get-TestCases
    $generatedDir = Join-Path (Get-RepositoryRoot) "generated_outputs"
    New-Item -ItemType Directory -Force -Path $generatedDir | Out-Null

    $executable = Get-ExecutablePath
    $bashExecutable = Convert-ToBashPath -Path $executable
    $failures = 0

    foreach ($testCase in $testCases) {
        $generatedParent = Split-Path -Parent $testCase.GeneratedPath
        New-Item -ItemType Directory -Force -Path $generatedParent | Out-Null

        $stderrPath = [System.IO.Path]::GetTempFileName()
        $bashInputPath = Convert-ToBashPath -Path $testCase.InputPath
        $bashGeneratedPath = Convert-ToBashPath -Path $testCase.GeneratedPath
        $bashStderrPath = Convert-ToBashPath -Path $stderrPath
        try {
            & bash -lc "cd '$(Convert-ToBashPath -Path (Get-RepositoryRoot))' && timeout '$TimeoutSeconds' '$bashExecutable' '$bashInputPath' '$($testCase.TargetVertices)' > '$bashGeneratedPath' 2> '$bashStderrPath'"
            if ($LASTEXITCODE -ne 0) {
                $failures++
                if ($LASTEXITCODE -eq 124) {
                    Write-Host "FAIL $($testCase.Name): timed out after $TimeoutSeconds second(s)"
                }
                else {
                    Write-Host "FAIL $($testCase.Name): program exited with code $LASTEXITCODE"
                }
                Write-Host "  input: $($testCase.InputPath)"
                Write-Host "  generated: $($testCase.GeneratedPath)"
                $stderrLines = @(Get-Content -LiteralPath $stderrPath)
                if ($stderrLines.Count -gt 0) {
                    Write-Host "  stderr:"
                    foreach ($line in $stderrLines) {
                        Write-Host "    $line"
                    }
                }
                continue
            }
        }
        finally {
            Remove-Item -LiteralPath $stderrPath -ErrorAction SilentlyContinue
        }

        Write-Host "GENERATED $($testCase.Name) (target $($testCase.TargetVertices)) -> generated_outputs\$([System.IO.Path]::GetFileName($testCase.GeneratedPath))"
    }

    Write-Host ""
    if ($failures -gt 0) {
        Write-Host "Generation finished with $failures failed run(s)."
        return 1
    }

    Write-Host "Generated $($testCases.Count) output file(s)."
    return 0
}

function Invoke-CompareOutputs {
    param(
        [int]$DiffLines = 40
    )

    $testCases = Get-TestCases
    $maxDiffLines = [int]$DiffLines
    $passed = 0
    $failed = 0

    foreach ($testCase in $testCases) {
        if (-not (Test-Path -LiteralPath $testCase.GeneratedPath)) {
            $failed++
            Write-Host "FAIL $($testCase.Name)"
            Write-Host "  expected: $($testCase.ExpectedPath)"
            Write-Host "  actual:   $($testCase.GeneratedPath)"
            Write-Host "  reason: generated output file is missing"
            continue
        }

        $expectedBytes = Read-FileBytesSafe -Path $testCase.ExpectedPath
        $actualBytes = Read-FileBytesSafe -Path $testCase.GeneratedPath

        if ($null -eq $expectedBytes -or $null -eq $actualBytes) {
            $failed++
            Write-Host "FAIL $($testCase.Name)"
            Write-Host "  expected: $($testCase.ExpectedPath)"
            Write-Host "  actual:   $($testCase.GeneratedPath)"
            Write-Host "  reason: expected or actual file could not be read"
            continue
        }

        $expectedHash = (Get-FileHash -LiteralPath $testCase.ExpectedPath -Algorithm SHA256).Hash
        $actualHash = (Get-FileHash -LiteralPath $testCase.GeneratedPath -Algorithm SHA256).Hash

        if ($expectedHash -eq $actualHash) {
            $passed++
            Write-Host "PASS $($testCase.Name)"
            continue
        }

        $failed++
        Write-Host "FAIL $($testCase.Name)"
        Write-Host "  expected: $($testCase.ExpectedPath)"
        Write-Host "  actual:   $($testCase.GeneratedPath)"

        $expectedText = Read-FileTextSafe -Path $testCase.ExpectedPath
        $actualText = Read-FileTextSafe -Path $testCase.GeneratedPath
        if ($null -eq $expectedText -or $null -eq $actualText) {
            Write-Host "  reason: diff unavailable because one of the files could not be read as text"
            continue
        }
        $expectedLines = [System.Text.RegularExpressions.Regex]::Split($expectedText, "\r?\n")
        $actualLines = [System.Text.RegularExpressions.Regex]::Split($actualText, "\r?\n")
        $maxCount = [Math]::Max($expectedLines.Count, $actualLines.Count)
        $diffOutputLines = New-Object System.Collections.Generic.List[string]

        for ($index = 0; $index -lt $maxCount; $index++) {
            $expectedLine = if ($index -lt $expectedLines.Count) { $expectedLines[$index] } else { "<missing>" }
            $actualLine = if ($index -lt $actualLines.Count) { $actualLines[$index] } else { "<missing>" }

            if ($expectedLine -eq $actualLine) {
                continue
            }

            $lineNumber = $index + 1
            $diffOutputLines.Add("@@ line $lineNumber @@")
            $diffOutputLines.Add("- $expectedLine")
            $diffOutputLines.Add("+ $actualLine")

            if ($diffOutputLines.Count -ge $maxDiffLines) {
                break
            }
        }

        if ($diffOutputLines.Count -gt 0) {
            Write-Host "  diff:"
            foreach ($line in $diffOutputLines) {
                Write-Host "    $line"
            }
        }
    }

    Write-Host ""
    Write-Host "Summary: $passed passed, $failed failed, $($passed + $failed) total"
    if ($failed -gt 0) {
        return 1
    }
    return 0
}
