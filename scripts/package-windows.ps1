param(
    [string]$RepoDir = "",
    [string]$BuildDir = ""
)

$ErrorActionPreference = "Stop"

if (-not $RepoDir) {
    $RepoDir = (Resolve-Path (Join-Path $PSScriptRoot "..")).Path
}

if (-not $BuildDir) {
    $BuildDir = Join-Path $RepoDir "build-win"
}

if (-not (Test-Path (Join-Path $RepoDir ".git"))) {
    Write-Error "Repository not found: $RepoDir"
}

Write-Host "[1/3] Configure"
cmake -S $RepoDir -B $BuildDir -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTS=OFF

Write-Host "[2/3] Build"
cmake --build $BuildDir --config Release

Write-Host "[3/3] Package"
cpack --config (Join-Path $BuildDir "CPackConfig.cmake") -G ZIP -C Release

Write-Host "Packages generated in: $BuildDir"
