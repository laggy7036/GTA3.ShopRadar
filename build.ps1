param([string]$Zig = "$PSScriptRoot/tools/zig-x86_64-windows-0.15.2/zig.exe")
$ErrorActionPreference='Stop'
Push-Location $PSScriptRoot
try {
    if (!(Test-Path -LiteralPath $Zig)) { throw 'Set -Zig to a Zig 0.15.2 executable.' }
    $env:ZIG_GLOBAL_CACHE_DIR=Join-Path $PSScriptRoot 'build/zig-cache'
    New-Item -ItemType Directory -Force build,dist | Out-Null
    & $Zig c++ -target x86_64-windows-gnu -std=c++17 -O0 tests/policy.cpp -o build/policy-tests.exe
    if ($LASTEXITCODE) { throw 'Test compilation failed' }
    & ./build/policy-tests.exe
    if ($LASTEXITCODE) { throw 'Policy tests failed' }
    & $Zig c++ -target x86-windows-gnu -std=c++17 -O0 tests/integration.cpp -o build/integration-tests.exe -luser32 -lkernel32 -Wall -Wextra
    if ($LASTEXITCODE) { throw 'Integration test compilation failed' }
    & ./build/integration-tests.exe
    if ($LASTEXITCODE) { throw 'Integration tests failed' }
    & $Zig c++ -target x86-windows-gnu -std=c++17 -O2 -shared -s src/ShopRadar.cpp -o dist/GTA3.ShopRadar.asi -luser32 -lkernel32 -Wall -Wextra -fno-exceptions -fno-rtti
    if ($LASTEXITCODE) { throw 'ASI compilation failed' }
    # Explicit entries preserve docs/ links and exclude local settings/tools.
    & ./scripts/package-release.ps1
} finally { Pop-Location }
