$ErrorActionPreference='Stop'
$projectRoot=Split-Path -Parent $PSScriptRoot
$checker=Join-Path $projectRoot 'scripts/check-publication.ps1'
$fixture=Join-Path $projectRoot ('build/publication-fixture-'+[Guid]::NewGuid().ToString('N'))
New-Item -ItemType Directory -Path $fixture -Force | Out-Null
& git -C $fixture init --quiet
if($LASTEXITCODE) { throw 'Cannot initialize test-owned fixture' }
[IO.File]::WriteAllText((Join-Path $fixture '.gitignore'),".env`n/build/`n")
[IO.File]::WriteAllText((Join-Path $fixture 'README.md'),'# Clean fixture')
$checks=0
function Expect-Check([bool]$ShouldPass,[string]$Zip='') {
    $passed=$true
    try { & $checker -RepoRoot $fixture -ReleaseZip $Zip *> $null } catch { $passed=$false }
    if($passed -ne $ShouldPass) { throw "Publication regression failed at check $script:checks" }
    $script:checks++
}
Expect-Check $true
# Constructed nonfunctional token. Never print the matched value.
$fake=('gh'+'p_')+('A'*36)
[IO.File]::WriteAllText((Join-Path $fixture '.env'),$fake)
Expect-Check $true # ignored/untracked environment file is not a candidate
[IO.File]::WriteAllText((Join-Path $fixture 'README.md'),$fake)
Expect-Check $false
[IO.File]::WriteAllText((Join-Path $fixture 'README.md'),'# Clean fixture')
[IO.File]::WriteAllText((Join-Path $fixture 'README.md'),'[Missing document](missing.md)')
Expect-Check $false
[IO.File]::WriteAllText((Join-Path $fixture 'README.md'),'# Clean fixture')
& git -C $fixture add -f .env
if($LASTEXITCODE) { throw 'Cannot stage test-owned fixture file' }
Expect-Check $false # ignore rules do not remove tracked secrets
& git -C $fixture rm --cached --force --quiet .env
if($LASTEXITCODE) { throw 'Cannot unstage test-owned fixture file' }
[IO.File]::WriteAllText((Join-Path $fixture 'unexpected.txt'),'not approved')
Expect-Check $false
Remove-Item -LiteralPath (Join-Path $fixture 'unexpected.txt')
New-Item -ItemType Directory -Path (Join-Path $fixture 'docs/media') -Force | Out-Null
[IO.File]::WriteAllText((Join-Path $fixture 'docs/media/unsafe.svg'),'<svg><script>test</script></svg>')
Expect-Check $false
Remove-Item -LiteralPath (Join-Path $fixture 'docs/media/unsafe.svg')
$zip=Join-Path $fixture 'build/release.zip'
Copy-Item -LiteralPath (Join-Path $projectRoot 'dist/GTA3.ShopRadar-0.1.0.zip') -Destination $zip
Expect-Check $true $zip
Add-Type -AssemblyName System.IO.Compression.FileSystem
$archive=[IO.Compression.ZipFile]::Open($zip,[IO.Compression.ZipArchiveMode]::Update)
try { $archive.CreateEntry('../unexpected.txt') | Out-Null } finally { $archive.Dispose() }
Expect-Check $false $zip
Write-Output "PASS: $checks publication gate cases; test-owned ignored fixture retained under build. No network used."
