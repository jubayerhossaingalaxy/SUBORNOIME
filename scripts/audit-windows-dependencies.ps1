$ErrorActionPreference = 'Stop'

$vswhere = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio\Installer\vswhere.exe'
if (-not (Test-Path -LiteralPath $vswhere)) {
    throw "Visual Studio locator not found: $vswhere"
}

$installationPath = & $vswhere -latest -products '*' -property installationPath
if ($LASTEXITCODE -ne 0 -or -not $installationPath) {
    throw 'Unable to locate the Visual Studio installation.'
}

$toolsetRoot = Join-Path $installationPath 'VC\Tools\MSVC'
$toolset = Get-ChildItem -LiteralPath $toolsetRoot -Directory |
    Sort-Object { [version]$_.Name } -Descending |
    Select-Object -First 1
if (-not $toolset) {
    throw "MSVC toolset not found under $toolsetRoot."
}

$dumpbin = Join-Path $toolset.FullName 'bin\Hostx64\x64\dumpbin.exe'
if (-not (Test-Path -LiteralPath $dumpbin)) {
    throw "dumpbin.exe not found: $dumpbin"
}

$binaries = @(
    'build\windows-msvc\Release\Suborno.exe',
    'build\windows-msvc\Release\suborno_windows_tsf.dll'
)
$forbidden = '^(icu[^\\]*|sqlite3|ucrtbase|vcruntime[^\\]*|msvcp[^\\]*|concrt[^\\]*|vcomp[^\\]*|api-ms-win-crt-[^\\]*)\.dll$'
foreach ($binary in $binaries) {
    if (-not (Test-Path -LiteralPath $binary -PathType Leaf)) {
        throw "Windows binary not found: $binary"
    }

    $output = & $dumpbin /DEPENDENTS $binary 2>&1
    if ($LASTEXITCODE -ne 0) {
        throw "dumpbin failed for $binary."
    }
    $dependencies = @(
        $output | ForEach-Object {
            if ("$_" -match '^\s+([A-Za-z0-9_.-]+\.dll)\s*$') {
                $Matches[1]
            }
        }
    )
    $forbiddenDependencies = @($dependencies | Where-Object { $_ -match $forbidden })
    if ($forbiddenDependencies.Count -ne 0) {
        throw "$binary has non-static third-party/runtime dependencies: $($forbiddenDependencies -join ', ')"
    }
    Write-Host "$binary dependencies: $($dependencies -join ', ')"
}
