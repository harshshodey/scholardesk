$ErrorActionPreference = 'Stop'
Push-Location $PSScriptRoot
try {
    New-Item -ItemType Directory -Force -Path bin | Out-Null
    & g++ -std=c++17 -O2 -Wall -Wextra -Wpedantic -static -I include src/main.cpp src/console.cpp src/registry.cpp -o bin/ScholarDeskConsole.exe
    if ($LASTEXITCODE -ne 0) { throw 'Build failed. Install a C++17 MinGW-w64 compiler and add g++ to PATH.' }
    & windres app.rc -o bin/app-resource.o
    if ($LASTEXITCODE -ne 0) { throw 'Resource compilation failed. Ensure MinGW-w64 windres is in PATH.' }
    & g++ -std=c++17 -O2 -Wall -Wextra -Wpedantic -static -municode -mwindows -I include src/desktop.cpp src/registry.cpp bin/app-resource.o -lcomctl32 -lgdi32 -lshell32 -o bin/ScholarDesk.exe
    if ($LASTEXITCODE -ne 0) { throw 'Desktop build failed.' }
    Write-Host 'Build successful. Double-click Run ScholarDesk.cmd to start.' -ForegroundColor Cyan
} finally { Pop-Location }
