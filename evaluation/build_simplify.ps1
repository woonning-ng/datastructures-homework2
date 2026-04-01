param(
    [switch]$Rebuild
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

$repoRoot = Split-Path -Parent $PSScriptRoot
Set-Location $repoRoot

function Find-VsDevCmd {
    foreach ($candidate in @(
        "C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\Tools\VsDevCmd.bat",
        "C:\Program Files\Microsoft Visual Studio\18\Community\Common7\Tools\VsDevCmd.bat"
    )) {
        if (Test-Path $candidate) {
            return $candidate
        }
    }
    return $null
}

if ($IsWindows) {
    $exe = Join-Path $repoRoot "simplify.exe"
    if ((-not $Rebuild) -and (Test-Path $exe)) {
        Write-Host "Using existing $exe"
        return
    }

    $vsDevCmd = Find-VsDevCmd
    if (-not $vsDevCmd) {
        throw "Unable to locate VsDevCmd.bat for MSVC build."
    }

    $command = 'call "{0}" -arch=x64 && cl /std:c++17 /EHsc /O2 /W4 /Fe:simplify.exe main.cpp Helper.cpp math.cpp Polygon.cpp Simplifier.cpp' -f $vsDevCmd
    cmd.exe /c $command
    if ($LASTEXITCODE -ne 0) {
        throw "MSVC build failed."
    }
    return
}

if (Get-Command make -ErrorAction SilentlyContinue) {
    if ($Rebuild) {
        make clean
    }
    make
    if ($LASTEXITCODE -ne 0) {
        throw "make failed."
    }
    return
}

throw "No supported build path found. Install make on Unix-like systems or use the Windows Visual Studio build path."
