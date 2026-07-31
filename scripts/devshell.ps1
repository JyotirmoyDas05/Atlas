# Script to enter MSVC x64 build environment in current PowerShell session
if ($env:VCINSTALLDIR) {
    Write-Host "[devshell] MSVC environment already active - skipping."
    return
}

$vswhere = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio\Installer\vswhere.exe'
if (-not (Test-Path $vswhere)) {
    Write-Error "[devshell] Visual Studio Installer not found. Please install Visual Studio 2022 Community with 'Desktop development with C++'."
    return
}

$vsPath = & $vswhere -latest -products * -property installationPath
if (-not $vsPath) {
    Write-Error "[devshell] Visual Studio installation not found."
    return
}

# Try locating vcvars64.bat in standard location
$vcvars = Join-Path $vsPath 'VC\Auxiliary\Build\vcvars64.bat'
if (-not (Test-Path $vcvars)) {
    Write-Host "[devshell] MSVC C++ toolset not found at: $vcvars" -ForegroundColor Yellow
    Write-Host "[devshell] To fix this, open 'Visual Studio Installer', click 'Modify', and check 'Desktop development with C++'." -ForegroundColor Cyan
    Write-Error "[devshell] Missing C++ x64 build tools in Visual Studio installation."
    return
}

cmd /c "`"$vcvars`" >nul 2>&1 && set" | ForEach-Object {
    if ($_ -match '^([^=]+)=(.*)$') {
        Set-Item -Path "Env:$($matches[1])" -Value $matches[2]
    }
}

Write-Host "[devshell] MSVC x64 environment active ($env:VSCMD_ARG_TGT_ARCH)." -ForegroundColor Green
