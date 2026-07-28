[CmdletBinding()]
param(
    [string]$GMake = 'C:\ti\ccs2041\ccs\utils\bin\gmake.exe'
)

$ErrorActionPreference = 'Stop'
$root = $PSScriptRoot
$settings = Join-Path $root 'wless_sm\wless_sm_settings.h'
$release = Join-Path $root 'RELEASE'
$output = Join-Path $release 'WLESS_CHG_F28003x.out'
$roleOutput = Join-Path $release 'WLESS_CHG_F28003x_STATION.out'

if (-not (Test-Path -LiteralPath $GMake)) {
    throw "gmake non trovato: $GMake"
}

$source = [IO.File]::ReadAllText($settings)
$updated = [regex]::Replace(
    $source,
    '(?m)^#define[ \t]+WLESS_SM_BUILD_VEHICLE[ \t]+[01][ \t]*$',
    '#define WLESS_SM_BUILD_VEHICLE          0'
)
if ($updated -eq $source -and $source -notmatch '(?m)^#define[ \t]+WLESS_SM_BUILD_VEHICLE[ \t]+0[ \t]*$') {
    throw 'Macro WLESS_SM_BUILD_VEHICLE non trovata.'
}
[IO.File]::WriteAllText($settings, $updated)

Write-Host 'Clean completo e build STATION...'
Push-Location $release
try {
    & $GMake -k clean
    if ($LASTEXITCODE -ne 0) {
        throw "Clean fallito con codice $LASTEXITCODE"
    }

    & $GMake -k -j 20 all -r -O
    if ($LASTEXITCODE -ne 0) {
        throw "Build STATION fallita con codice $LASTEXITCODE"
    }
}
finally {
    Pop-Location
}

if (-not (Test-Path -LiteralPath $output)) {
    throw "Artefatto non prodotto: $output"
}
Copy-Item -LiteralPath $output -Destination $roleOutput -Force
Write-Host "Build STATION completata: $roleOutput"
