$ErrorActionPreference = 'Stop'
$root = Split-Path $PSScriptRoot -Parent
Push-Location $root
try {
    New-Item -ItemType Directory -Force work/tools | Out-Null
    if (!(Test-Path work/tools/arduino-cli.exe)) {
        Invoke-WebRequest 'https://downloads.arduino.cc/arduino-cli/arduino-cli_1.6.0_Windows_64bit.zip' -OutFile work/tools/arduino-cli.zip
        Expand-Archive work/tools/arduino-cli.zip -DestinationPath work/tools -Force
    }
    $config = @{
        board_manager = @{additional_urls = @('https://espressif.github.io/arduino-esp32/package_esp32_index.json')}
        directories = @{data = "$root/work/arduino-data"; downloads = "$root/work/arduino-downloads"; user = "$root/work/arduino-user"}
    }
    $config | ConvertTo-Json -Depth 5 | Set-Content work/arduino-cli.yaml -Encoding utf8
    & ./work/tools/arduino-cli.exe --config-file work/arduino-cli.yaml core update-index
    if ($LASTEXITCODE -ne 0) { throw 'Index download failed' }
    & ./work/tools/arduino-cli.exe --config-file work/arduino-cli.yaml core install esp32:esp32@3.3.7
    if ($LASTEXITCODE -ne 0) { throw 'Core installation failed' }
} finally { Pop-Location }
