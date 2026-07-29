[CmdletBinding()]
param(
    [string]$GMake = 'C:\ti\ccs2041\ccs\utils\bin\gmake.exe'
)

$ErrorActionPreference = 'Stop'
$root = $PSScriptRoot
$release = Join-Path $root 'RELEASE'
$output = Join-Path $release 'WLESS_CHG_F28003x.out'
$roleOutput = Join-Path $release 'WLESS_CHG_F28003x_VEHICLE.out'
$roleDefine = 'GEN_OPTS__FLAG=--define=WLESS_SM_BUILD_VEHICLE=1'

if (-not (Test-Path -LiteralPath $GMake)) {
    throw "gmake non trovato: $GMake"
}

Write-Host 'Clean completo e build VEHICLE...'
Push-Location $release
try {
    & $GMake -k clean
    if ($LASTEXITCODE -ne 0) {
        throw "Clean fallito con codice $LASTEXITCODE"
    }

    & $GMake -k -j 20 all -r -O $roleDefine
    if ($LASTEXITCODE -ne 0) {
        throw "Build VEHICLE fallita con codice $LASTEXITCODE"
    }
}
finally {
    Pop-Location
}

if (-not (Test-Path -LiteralPath $output)) {
    throw "Artefatto non prodotto: $output"
}
Copy-Item -LiteralPath $output -Destination $roleOutput -Force
Write-Host "Build VEHICLE completata: $roleOutput"
