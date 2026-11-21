$ErrorActionPreference = 'Stop'

$installer = Join-Path $PWD 'installer\windows\Output\Suborno-Setup-x64.exe'
$installerX86 = Join-Path $PWD 'installer\windows\Output\Suborno-Setup-x86.exe'
if (-not (Test-Path -LiteralPath $installer -PathType Leaf)) {
    throw "Installer not found: $installer"
}

$setupVersion = (Get-Item -LiteralPath $installer).VersionInfo
if ($setupVersion.ProductName -ne 'Suborno') {
    throw "Unexpected installer ProductName: $($setupVersion.ProductName)"
}
$signature = Get-AuthenticodeSignature -LiteralPath $installer
Write-Host "Installer signature status: $($signature.Status)"
if ($signature.Status -notin @('Valid', 'NotSigned')) {
    throw "Unexpected installer signature status: $($signature.Status)"
}

$installDirectory = Join-Path $env:ProgramFiles 'Suborno'
$installDirectoryX86 = Join-Path ${env:ProgramFiles(x86)} 'Suborno'
$programDataDirectory = Join-Path $env:ProgramData 'Suborno'
$serviceDirectory = Join-Path $programDataDirectory 'components\tsf\x64'
$serviceDirectoryX86 = Join-Path $programDataDirectory 'components\tsf\x86'
$serviceBuild = Join-Path $PWD 'build\windows-msvc\Release\suborno_windows_tsf.dll'
$service = Join-Path $serviceDirectory 'suborno_windows_tsf.dll'
$serviceX86 = Join-Path $serviceDirectoryX86 'suborno_windows_tsf.dll'
$userDataDirectory = Join-Path $env:APPDATA 'Suborno'
$userDictionary = Join-Path $userDataDirectory 'dictionaries\user_candidates.db'
$userLearnt = Join-Path $userDataDirectory 'dictionaries\suborno_learnt_bn'
$userLayouts = Join-Path $userDataDirectory 'layouts'
$desktopDirectory = [Environment]::GetFolderPath('CommonDesktopDirectory')
$startMenuDirectory = Join-Path ([Environment]::GetFolderPath('CommonPrograms')) 'Suborno'
$quickLaunchDirectory = Join-Path $env:APPDATA 'Microsoft\Internet Explorer\Quick Launch'
$commonFilesDirectory = [Environment]::GetFolderPath('CommonProgramFiles')
$regsvr32 = Join-Path $env:WINDIR 'System32\regsvr32.exe'
$shell = New-Object -ComObject WScript.Shell
$classRegistration = 'Registry::HKEY_CLASSES_ROOT\CLSID\{E1CC38E7-F986-4371-BA1D-A527CA600677}\InprocServer32'
$classKey = 'Registry::HKEY_CLASSES_ROOT\CLSID\{E1CC38E7-F986-4371-BA1D-A527CA600677}'

function Invoke-SubornoSetup {
    param([string]$Path)

    $process = Start-Process -FilePath $Path `
        -ArgumentList @('/VERYSILENT', '/SUPPRESSMSGBOXES', '/NORESTART', '/SP-', '/TASKS=desktopicon,quicklaunchicon') `
        -Wait -PassThru
    if ($process.ExitCode -ne 0) {
        throw "Suborno installer exited with code $($process.ExitCode)."
    }
}

function Assert-CleanInstallDirectory {
    param([string]$Path)

    $expectedFiles = @('Suborno.exe', 'THIRD_PARTY_NOTICES.md', 'unins000.dat', 'unins000.exe')
    $actualFiles = @(Get-ChildItem -LiteralPath $Path -Force -File |
        Sort-Object Name | Select-Object -ExpandProperty Name)
    if (($actualFiles -join '|') -ne (($expectedFiles | Sort-Object) -join '|')) {
        throw "Unexpected Program Files contents: $($actualFiles -join ', ')"
    }
    $noticesPath = Join-Path $Path 'THIRD_PARTY_NOTICES.md'
    if (-not (Test-Path -LiteralPath $noticesPath -PathType Leaf) -or
        (Get-Item -LiteralPath $noticesPath).Length -eq 0) {
        throw 'Third-party license notices are missing from the installed application.'
    }
    if (@(Get-ChildItem -LiteralPath $Path -Force -Directory).Count -ne 0) {
        throw 'Subdirectories were installed into the Suborno application directory.'
    }
}

function Assert-ServiceDeployment {
    param([string]$Path, [string]$Architecture)

    if (-not (Test-Path -LiteralPath $Path -PathType Leaf)) {
        throw "The TSF service is missing from ProgramData: $Path"
    }
    $serviceFiles = @(Get-ChildItem -LiteralPath (Split-Path -Parent $Path) -Force -File)
    if ($serviceFiles.Count -ne 1 -or $serviceFiles[0].Name -ne 'suborno_windows_tsf.dll') {
        throw "Unexpected ProgramData service files: $($serviceFiles.Name -join ', ')"
    }
    $registeredPath = (Get-Item -LiteralPath $classRegistration).GetValue('')
    if ([IO.Path]::GetFullPath($registeredPath) -ne [IO.Path]::GetFullPath($Path)) {
        throw "The TSF COM server points to the wrong file: $registeredPath"
    }
}

function Assert-SharedResourceDirectories {
    foreach ($resourceDirectory in @(
        'assets\fonts', 'assets\themes', 'dictionaries', 'layouts'
    )) {
        if (-not (Test-Path -LiteralPath (Join-Path $programDataDirectory $resourceDirectory) -PathType Container)) {
            throw "Machine-wide resource directory is missing: $resourceDirectory"
        }
    }
    if (Test-Path -LiteralPath (Join-Path $commonFilesDirectory 'Suborno')) {
        throw 'Suborno unexpectedly installed files under Common Files.'
    }
}

function Assert-Shortcut {
    param([string]$Path, [string]$ExpectedTarget)

    if (-not (Test-Path -LiteralPath $Path -PathType Leaf)) {
        throw "Shortcut not found: $Path"
    }
    $shortcut = $shell.CreateShortcut($Path)
    if ([IO.Path]::GetFullPath($shortcut.TargetPath) -ne [IO.Path]::GetFullPath($ExpectedTarget)) {
        throw "Shortcut target is incorrect: $Path -> $($shortcut.TargetPath)"
    }
}

function Assert-UserDataPreserved {
    if (-not (Test-Path -LiteralPath $userDictionary -PathType Leaf)) {
        throw 'The user dictionary was removed.'
    }
    if (-not (Test-Path -LiteralPath $userLearnt -PathType Container)) {
        throw 'The user learnt-word directory was removed.'
    }
    if (-not (Test-Path -LiteralPath $userLayouts -PathType Container)) {
        throw 'The user layouts directory was removed.'
    }
}

function Assert-UserDataInitialized {
    if (-not (Test-Path -LiteralPath $userDictionary -PathType Leaf) -or
        -not (Test-Path -LiteralPath $userLearnt -PathType Container) -or
        -not (Test-Path -LiteralPath $userLayouts -PathType Container)) {
        throw 'The per-user Roaming data structure was not initialized.'
    }
}

# Seed the prior deployment's layout and registration to exercise the real upgrade path.
if (Test-Path -LiteralPath $installDirectory) {
    throw "Expected a clean CI runner; installation directory already exists: $installDirectory"
}
if (Test-Path -LiteralPath $programDataDirectory) {
    throw "Expected a clean CI runner; ProgramData directory already exists: $programDataDirectory"
}
New-Item -ItemType Directory -Force -Path $installDirectory | Out-Null
$legacyService = Join-Path $installDirectory 'suborno_windows_tsf.dll'
Copy-Item -LiteralPath $serviceBuild -Destination $legacyService
Copy-Item -LiteralPath (Join-Path $PWD 'THIRD_PARTY_NOTICES.md') `
    -Destination (Join-Path $installDirectory 'THIRD_PARTY_NOTICES.md')
foreach ($legacyFile in @('sqlite3.dll', 'icuuc.dll', 'vcruntime140.dll', 'msvcp140.dll')) {
    Set-Content -LiteralPath (Join-Path $installDirectory $legacyFile) -Value 'legacy deployment residue'
}
$legacyRegistration = Start-Process -FilePath $regsvr32 `
    -ArgumentList "/s `"$legacyService`"" -Wait -PassThru
if ($legacyRegistration.ExitCode -ne 0) {
    throw "Legacy TSF registration simulation failed with exit code $($legacyRegistration.ExitCode)."
}

# Phase 1: x64 install, upgrade, smoke, and uninstall.
Invoke-SubornoSetup -Path $installer
$application = Join-Path $installDirectory 'Suborno.exe'
Assert-CleanInstallDirectory -Path $installDirectory
Assert-ServiceDeployment -Path $service -Architecture x64
Assert-SharedResourceDirectories

$desktopShortcut = Join-Path $desktopDirectory 'Suborno.lnk'
$startMenuShortcut = Join-Path $startMenuDirectory 'Suborno.lnk'
$quickLaunchShortcut = Join-Path $quickLaunchDirectory 'Suborno.lnk'
Assert-Shortcut -Path $desktopShortcut -ExpectedTarget $application
Assert-Shortcut -Path $startMenuShortcut -ExpectedTarget $application
Assert-Shortcut -Path $quickLaunchShortcut -ExpectedTarget $application

$smoke = Start-Process -FilePath $application -ArgumentList '--smoke-test' -Wait -PassThru
if ($smoke.ExitCode -ne 0) {
    throw "Installed GUI smoke test failed with exit code $($smoke.ExitCode)."
}
Assert-UserDataInitialized

Invoke-SubornoSetup -Path $installer
Assert-CleanInstallDirectory -Path $installDirectory
Assert-ServiceDeployment -Path $service -Architecture x64

$startupKey = 'HKCU:\Software\Microsoft\Windows\CurrentVersion\Run'
New-Item -Path $startupKey -Force | Out-Null
Set-ItemProperty -Path $startupKey -Name 'Suborno' -Value "`"$application`" --startup"
$uninstaller = Join-Path $installDirectory 'unins000.exe'
if (-not (Test-Path -LiteralPath $uninstaller -PathType Leaf)) {
    throw "Uninstaller not found: $uninstaller"
}
$uninstall = Start-Process -FilePath $uninstaller `
    -ArgumentList @('/VERYSILENT', '/SUPPRESSMSGBOXES', '/NORESTART') -Wait -PassThru
if ($uninstall.ExitCode -ne 0) {
    throw "Suborno uninstaller exited with code $($uninstall.ExitCode)."
}

if (Test-Path -LiteralPath $installDirectory) {
    throw 'Suborno installation directory remains after uninstall.'
}
if (Test-Path -LiteralPath $programDataDirectory) {
    throw 'Suborno service files remain in ProgramData after uninstall.'
}
Assert-UserDataPreserved
if (Test-Path -LiteralPath $desktopShortcut) {
    throw 'Desktop shortcut remains after uninstall.'
}
if (Test-Path -LiteralPath $startMenuShortcut) {
    throw 'Start Menu shortcut remains after uninstall.'
}
if (Test-Path -LiteralPath $quickLaunchShortcut) {
    throw 'Quick Launch shortcut remains after uninstall.'
}
if (Test-Path -LiteralPath $classKey) {
    throw 'The TSF COM server registration remains after uninstall.'
}
if (Get-ItemProperty -Path $startupKey -Name 'Suborno' -ErrorAction SilentlyContinue) {
    throw 'Suborno sign-in startup entry remains after uninstall.'
}

Write-Host 'x64 install, upgrade, exact folder contents, shortcuts, TSF registration, GUI smoke, reinstall, and uninstall checks passed.'

# Phase 2: x86 and x64 must share ProgramData resources and the same AppData user data.
if (-not (Test-Path -LiteralPath $installerX86 -PathType Leaf)) {
    Write-Host 'x86 installer not present; skipping cross-architecture co-existence checks.'
    Write-Host 'All installer checks passed.'
    exit 0
}

if (Test-Path -LiteralPath $installDirectoryX86) {
    throw "Expected a clean CI runner; x86 installation directory already exists: $installDirectoryX86"
}

# Install x86 first.
Invoke-SubornoSetup -Path $installerX86
$applicationX86 = Join-Path $installDirectoryX86 'Suborno.exe'
Assert-CleanInstallDirectory -Path $installDirectoryX86
Assert-ServiceDeployment -Path $serviceX86 -Architecture x86
Assert-SharedResourceDirectories
Assert-UserDataPreserved

# Install x64 on top; both architectures must coexist with shared ProgramData resources.
Invoke-SubornoSetup -Path $installer
Assert-CleanInstallDirectory -Path $installDirectory
Assert-ServiceDeployment -Path $service -Architecture x64
if (-not (Test-Path -LiteralPath $serviceX86 -PathType Leaf)) {
    throw 'The x86 TSF service was removed by the x64 installation.'
}
Assert-SharedResourceDirectories
Assert-UserDataPreserved

$smokeX64 = Start-Process -FilePath $application -ArgumentList '--smoke-test' -Wait -PassThru
if ($smokeX64.ExitCode -ne 0) {
    throw "Installed x64 GUI smoke test failed with exit code $($smokeX64.ExitCode)."
}

# Uninstall x86: its application directory goes, shared ProgramData resources and user data stay.
$uninstallerX86 = Join-Path $installDirectoryX86 'unins000.exe'
if (-not (Test-Path -LiteralPath $uninstallerX86 -PathType Leaf)) {
    throw "x86 uninstaller not found: $uninstallerX86"
}
$uninstallX86 = Start-Process -FilePath $uninstallerX86 `
    -ArgumentList @('/VERYSILENT', '/SUPPRESSMSGBOXES', '/NORESTART') -Wait -PassThru
if ($uninstallX86.ExitCode -ne 0) {
    throw "x86 uninstaller exited with code $($uninstallX86.ExitCode)."
}
if (Test-Path -LiteralPath $installDirectoryX86) {
    throw 'x86 installation directory remains after uninstall.'
}
if (-not (Test-Path -LiteralPath $installDirectory)) {
    throw 'x64 installation directory was removed by the x86 uninstall.'
}
if (-not (Test-Path -LiteralPath $service -PathType Leaf)) {
    throw 'The x64 TSF service was removed by the x86 uninstall.'
}
Assert-SharedResourceDirectories
Assert-UserDataPreserved

# Uninstall x64 last: shared ProgramData resources are removed only when no architecture remains.
$uninstall = Start-Process -FilePath $uninstaller `
    -ArgumentList @('/VERYSILENT', '/SUPPRESSMSGBOXES', '/NORESTART') -Wait -PassThru
if ($uninstall.ExitCode -ne 0) {
    throw "Suborno uninstaller exited with code $($uninstall.ExitCode)."
}
if (Test-Path -LiteralPath $installDirectory) {
    throw 'Suborno installation directory remains after uninstall.'
}
if (Test-Path -LiteralPath $programDataDirectory) {
    throw 'Suborno service files remain in ProgramData after uninstall.'
}
Assert-UserDataPreserved
if (Test-Path -LiteralPath $desktopShortcut) {
    throw 'Desktop shortcut remains after uninstall.'
}
if (Test-Path -LiteralPath $startMenuShortcut) {
    throw 'Start Menu shortcut remains after uninstall.'
}
if (Test-Path -LiteralPath $quickLaunchShortcut) {
    throw 'Quick Launch shortcut remains after uninstall.'
}
if (Test-Path -LiteralPath $classKey) {
    throw 'The TSF COM server registration remains after uninstall.'
}
if (Get-ItemProperty -Path $startupKey -Name 'Suborno' -ErrorAction SilentlyContinue) {
    throw 'Suborno sign-in startup entry remains after uninstall.'
}

Write-Host 'x86 co-existence, shared ProgramData preservation, per-architecture uninstall, and last-architecture cleanup checks passed.'
Write-Host 'All installer checks passed.'
