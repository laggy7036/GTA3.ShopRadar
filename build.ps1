param([string]$Zig = "$PSScriptRoot/tools/zig-x86_64-windows-0.15.2/zig.exe")
$ErrorActionPreference='Stop'
Push-Location $PSScriptRoot
try {
    if (!(Test-Path -LiteralPath $Zig)) { throw 'Set -Zig to a Zig 0.15.2 executable.' }
    $env:ZIG_GLOBAL_CACHE_DIR=Join-Path $PSScriptRoot 'build/zig-cache'
    New-Item -ItemType Directory -Force build,dist | Out-Null
    $Version=(Get-Content -LiteralPath VERSION -Raw).Trim()
    if ($Version -notmatch '^\d+\.\d+\.\d+$') { throw "VERSION must look like 1.2.3, not '$Version'" }
    Set-Content -LiteralPath build/version.h -Value "#define SHOPRADAR_VERSION `"$Version`"" -Encoding ascii
    & $Zig c++ -target x86_64-windows-gnu -std=c++17 -O0 tests/policy.cpp -o build/policy-tests.exe
    if ($LASTEXITCODE) { throw 'Test compilation failed' }
    & ./build/policy-tests.exe
    if ($LASTEXITCODE) { throw 'Policy tests failed' }
    & $Zig c++ -target x86_64-windows-gnu -std=c++17 -O0 tests/discovery.cpp -o build/discovery-tests.exe
    if ($LASTEXITCODE) { throw 'Discovery test compilation failed' }
    & ./build/discovery-tests.exe
    if ($LASTEXITCODE) { throw 'Discovery tests failed' }
    & $Zig c++ -target x86-windows-gnu -std=c++17 -O0 tests/integration.cpp -Ibuild -o build/integration-tests.exe -luser32 -lkernel32 -Wall -Wextra
    if ($LASTEXITCODE) { throw 'Integration test compilation failed' }
    & ./build/integration-tests.exe
    if ($LASTEXITCODE) { throw 'Integration tests failed' }
    & $Zig c++ -target x86-windows-gnu -std=c++17 -O2 -shared -s src/ShopRadar.cpp -Ibuild -o dist/GTA3.ShopRadar.asi -luser32 -lkernel32 -Wall -Wextra -fno-exceptions -fno-rtti
    if ($LASTEXITCODE) { throw 'ASI compilation failed' }
    # Explicit entries preserve docs/ links and exclude local settings/tools.
    & ./scripts/package-release.ps1 -Version $Version
} finally { Pop-Location }
