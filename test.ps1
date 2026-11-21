param([ValidateRange(1, 10)][int]$Repeat = 1)
$ErrorActionPreference = 'Stop'
Push-Location $PSScriptRoot
try {
    New-Item -ItemType Directory -Force -Path bin | Out-Null
    & g++ -std=c++17 -O2 -Wall -Wextra -Wpedantic -static -I include tests/registry_tests.cpp src/registry.cpp -o bin/scholar_tests.exe
    if ($LASTEXITCODE -ne 0) { throw 'Test compilation failed.' }
    & windres app.rc -o bin/app-resource.o
    if ($LASTEXITCODE -ne 0) { throw 'Test resource compilation failed.' }
    & g++ -std=c++17 -O2 -Wall -Wextra -Wpedantic -static -municode -mwindows -DSCHOLAR_DESKTOP_TESTS -I include src/desktop.cpp src/registry.cpp bin/app-resource.o -lcomctl32 -lgdi32 -lshell32 -o bin/desktop_tests.exe
    if ($LASTEXITCODE -ne 0) { throw 'Desktop test compilation failed.' }
    for ($round = 1; $round -le $Repeat; $round++) {
        Write-Host "Regression pass $round of $Repeat" -ForegroundColor Cyan
        & ./bin/scholar_tests.exe
        if ($LASTEXITCODE -ne 0) { throw 'Core tests failed.' }
        $desktopRun = Start-Process -FilePath (Join-Path $PSScriptRoot 'bin/desktop_tests.exe') -WindowStyle Hidden -PassThru
        if (-not $desktopRun.WaitForExit(30000)) { $desktopRun.Kill(); throw 'Desktop tests timed out after 30 seconds.' }
        Get-Content bin/desktop-test-results.txt
        if ($desktopRun.ExitCode -ne 0) { throw 'Desktop tests failed.' }
        & ./tests/console_smoke.ps1
    }
} finally { Pop-Location }
