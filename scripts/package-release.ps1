$ErrorActionPreference='Stop'
$projectRoot=Split-Path -Parent $PSScriptRoot
Add-Type -AssemblyName System.IO.Compression,System.IO.Compression.FileSystem
$releasePath=Join-Path $projectRoot 'dist/GTA3.ShopRadar-0.1.1.zip'
[IO.File]::Delete($releasePath)
$package=[IO.Compression.ZipFile]::Open($releasePath,[IO.Compression.ZipArchiveMode]::Create)
try {
    $files=@{
        'GTA3.ShopRadar.asi'='dist/GTA3.ShopRadar.asi'
        'GTA3.ShopRadar.ini'='dist/GTA3.ShopRadar.ini'
        'README.md'='README.md'; 'INSTALL.md'='INSTALL.md'; 'CHANGELOG.md'='CHANGELOG.md'
        'LICENSE'='LICENSE'; 'THIRD_PARTY.md'='THIRD_PARTY.md'; 'SECURITY.md'='SECURITY.md'
        'docs/VALIDATION.md'='docs/VALIDATION.md'; 'docs/RESEARCH.md'='docs/RESEARCH.md'
        'docs/BUGS.md'='docs/BUGS.md'
        'docs/media/palette.svg'='docs/media/palette.svg'
        'docs/media/settings-menu.png'='docs/media/settings-menu.png'
        'docs/media/radar-staunton-street.jpg'='docs/media/radar-staunton-street.jpg'
        'docs/media/radar-staunton-alley.jpg'='docs/media/radar-staunton-alley.jpg'
        'docs/media/bomb-icons.svg'='docs/media/bomb-icons.svg'
    }
    foreach($name in ($files.Keys | Sort-Object)) {
        [IO.Compression.ZipFileExtensions]::CreateEntryFromFile($package,(Join-Path $projectRoot $files[$name]),$name) | Out-Null
    }
} finally { $package.Dispose() }
$digest=(Get-FileHash -LiteralPath $releasePath -Algorithm SHA256).Hash.ToLowerInvariant()
[IO.File]::WriteAllText((Join-Path $projectRoot 'dist/GTA3.ShopRadar-0.1.1.zip.sha256'),"$digest  GTA3.ShopRadar-0.1.1.zip`n")
