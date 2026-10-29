[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)]
    [string]$ComPort,

    [Parameter(Mandatory = $true)]
    [int]$Bps,

    [Parameter(Mandatory = $true)]
    [string]$Message,

    [int]$TimeoutMs = 3000
)

try {
    # 1. Inizializzazione della porta seriale
    $serialPort = New-Object System.IO.Ports.SerialPort $ComPort, $Bps, ([System.IO.Ports.Parity]::None), 8, ([System.IO.Ports.StopBits]::One)
    $serialPort.ReadTimeout = $TimeoutMs
    $serialPort.WriteTimeout = $TimeoutMs

    # 2. Apertura della connessione
    Write-Host "Apertura della porta $ComPort a $Bps baud..." -ForegroundColor Cyan
    $serialPort.Open()

    # 3. Invio del messaggio
    $stringToSend = "$Message`n"
    Write-Host "Invio in corso: $Message" -ForegroundColor Yellow
    $serialPort.Write($stringToSend)

    # Breve pausa iniziale per dare tempo al dispositivo di elaborare
    Start-Sleep -Milliseconds 300

    # 4. Consumo completo del buffer di ricezione
    Write-Host "Lettura dell'intero buffer in corso..." -ForegroundColor Cyan
    
    $receivedData = [System.Text.StringBuilder]::new()
    $sw = [System.Diagnostics.Stopwatch]::StartNew()
    
    # Ciclo di lettura dinamico finché ci sono dati o scade il timeout
    while ($sw.ElapsedMilliseconds -lt $TimeoutMs) {
        if ($serialPort.BytesToRead -gt 0) {
            [void]$receivedData.Append($serialPort.ReadExisting())
            # Pausa brevissima per permettere ad altri eventuali byte in arrivo di accumularsi nel buffer
            Start-Sleep -Milliseconds 50
        } else {
            # Se il buffer è momentaneamente vuoto ma abbiamo già ricevuto qualcosa, 
            # diamo un attimo di respiro per capire se la trasmissione è finita
            if ($receivedData.Length -gt 0) {
                Start-Sleep -Milliseconds 100
                if ($serialPort.BytesToRead -eq 0) { break }
            } else {
                Start-Sleep -Milliseconds 50
            }
        }
    }
    $sw.Stop()

    # 5. Output del risultato
    if ($receivedData.Length -gt 0) {
        Write-Host "Buffer svuotato con successo. Dati ricevuti:" -ForegroundColor Green
        Write-Host "----------------------------------------" -ForegroundColor DarkGray
        Write-Host $receivedData.ToString() -ForegroundColor White
        Write-Host "----------------------------------------" -ForegroundColor DarkGray
    } else {
        Write-Warning "Nessun dato ricevuto nel buffer entro il tempo limite."
    }

}
catch {
    Write-Error "Errore durante la comunicazione seriale: $_"
}
finally {
    # 6. Chiusura e rilascio delle risorse
    if ($serialPort -and $serialPort.IsOpen) {
        $serialPort.Close()
        Write-Host "Porta seriale chiusa correttamente." -ForegroundColor Gray
    }
    if ($serialPort) {
        $serialPort.Dispose()
    }
}