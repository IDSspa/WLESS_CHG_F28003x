[CmdletBinding()]
param(
    [string]$DSLite = 'C:\ti\ccs2041\ccs\ccs_base\DebugServer\bin\DSLite.exe'
)

$ErrorActionPreference = 'Stop'
$root = $PSScriptRoot
$baseConfig = Join-Path $root 'TMS320F280039C.ccxml'
$firmware = Join-Path $root 'RELEASE\WLESS_CHG_F28003x_STATION.out'

if (-not (Test-Path -LiteralPath $DSLite)) {
    throw "DSLite non trovato: $DSLite"
}
if (-not (Test-Path -LiteralPath $firmware)) {
    throw "Firmware STATION assente. Eseguire prima .\build_station.ps1"
}

if ((Read-Host 'Confermare che sia collegata SOLO la controlCARD STAT0001 digitando S') -ne 'S') {
    throw 'Flash annullato: controlCARD STATION non confermata.'
}

$env:TI_APPDATA_DIR = Join-Path $root '.ti_appdata'
Write-Warning 'La selezione della controlCARD dipende esclusivamente dalla conferma dell''operatore.'
Write-Host 'Programmazione firmware STATION sull''unica controlCARD collegata...'
& $DSLite load -c $baseConfig -f $firmware
if ($LASTEXITCODE -ne 0) {
    throw "Flash STATION fallito con codice $LASTEXITCODE"
}

Write-Host 'Flash STATION completato. Eseguire il power-cycle prima della verifica UART.'
