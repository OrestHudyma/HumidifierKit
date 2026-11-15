$ErrorActionPreference = 'Stop'
Push-Location (Split-Path $PSScriptRoot -Parent)
try {
    & .\work\tools\arduino-cli.exe --config-file work\arduino-cli.yaml compile --fqbn 'esp32:esp32:esp32h2:ZigbeeMode=zczr,PartitionScheme=zigbee_zczr,CDCOnBoot=cdc,FlashSize=4M,DebugLevel=warn' --build-path build firmware\HumidifierKit
    if ($LASTEXITCODE -ne 0) { throw 'Firmware build failed' }
} finally { Pop-Location }

