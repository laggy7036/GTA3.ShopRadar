param(
    [string]$RepoRoot=(Split-Path -Parent $PSScriptRoot),
    [string]$ReleaseZip=''
)
$ErrorActionPreference='Stop'
$root=(Resolve-Path -LiteralPath $RepoRoot).Path
$findings=[Collections.Generic.List[object]]::new()
$manifest=[Collections.Generic.List[object]]::new()
function Add-Finding([string]$Path,[string]$Reason) { $findings.Add([pscustomobject]@{path=$Path;reason=$Reason}) }
$allowed='^(\.gitignore|\.gitattributes|LICENSE|AGENTS\.md|README\.md|INSTALL\.md|CHANGELOG\.md|THIRD_PARTY\.md|SECURITY\.md|build\.ps1|scripts/(check-publication|package-release)\.ps1|src/[A-Za-z0-9_.-]+\.(cpp|h)|tests/[A-Za-z0-9_.-]+\.(cpp|ps1)|dist/GTA3\.ShopRadar\.ini|\.github/[A-Za-z0-9_./-]+\.(yml|yaml|md)|docs/[A-Za-z0-9_.-]+\.md|docs/media/[A-Za-z0-9_.-]+\.(svg|png|jpg|jpeg))$'
$patterns=@{
    'GitHub credential'='(?:gh[pousr]_[A-Za-z0-9]{30,}|github_pat_[A-Za-z0-9_]{30,})'
    'API credential'='sk-(?:proj-|svcacct-)?[A-Za-z0-9_-]{20,}'
    'AWS access key'='AKIA[0-9A-Z]{16}'
    'Private key'='-----BEGIN (?:RSA |EC |OPENSSH )?PRIVATE KEY-----'
    'Credential assignment'='(?i)(?:api[_-]?key|access[_-]?token|password|secret)\s*[:=]\s*["''][^"''\s]{12,}'
    'Machine-specific path'='(?i)[CD]:[\\/](?:Users|SteamLibrary)[\\/]'
    'Email address'='[A-Za-z0-9._%+-]+@[A-Za-z0-9.-]+\.[A-Za-z]{2,}'
}
function Inspect-Bytes([string]$Name,[byte[]]$Bytes) {
    foreach($encoding in @([Text.Encoding]::UTF8,[Text.Encoding]::Unicode)) {
        $value=$encoding.GetString($Bytes)
        foreach($rule in $patterns.Keys) {
            if([regex]::IsMatch($value,$patterns[$rule])) { Add-Finding $Name $rule }
        }
    }
    if($Name.EndsWith('.svg') -and [regex]::IsMatch([Text.Encoding]::UTF8.GetString($Bytes),'(?i)<script|<foreignObject|\bonload\s*=|(?:href|src)\s*=\s*["''](?:https?:|file:|data:)')) {
        Add-Finding $Name 'Active or externally linked SVG content'
    }
}
function Inspect-Links([string]$Name,[string]$Text,[hashtable]$Files) {
    if(!$Name.EndsWith('.md')) { return }
    foreach($match in [regex]::Matches($Text,'!?\[[^\]]*\]\(([^)]+)\)')) {
        $target=$match.Groups[1].Value.Trim().Trim('<','>')
        if($target -match '^(?:https?://|mailto:|#)' -or !$target) { continue }
        try {
            # URI normalization is local; this address is never requested.
            $base=[Uri]('https://archive.invalid/'+$Name)
            $link=[Uri]::new($base,$target)
            $path=[Uri]::UnescapeDataString($link.AbsolutePath).TrimStart('/')
            if($link.Host -ne 'archive.invalid' -or !$Files.ContainsKey($path)) { Add-Finding $Name 'Local documentation link is outside the candidate/archive' }
        } catch { Add-Finding $Name 'Invalid local documentation link' }
    }
}
function Test-ModPE([byte[]]$Bytes) {
    try {
        if($Bytes.Length -lt 256 -or [BitConverter]::ToUInt16($Bytes,0) -ne 0x5a4d) { return $false }
        $pe=[BitConverter]::ToInt32($Bytes,0x3c)
        if($pe -lt 0 -or $pe+248 -gt $Bytes.Length) { return $false }
        if([BitConverter]::ToUInt32($Bytes,$pe) -ne 0x4550 -or [BitConverter]::ToUInt16($Bytes,$pe+4) -ne 0x14c -or
            !([BitConverter]::ToUInt16($Bytes,$pe+22) -band 0x2000) -or [BitConverter]::ToUInt16($Bytes,$pe+24) -ne 0x10b) { return $false }
        $sections=[BitConverter]::ToUInt16($Bytes,$pe+6)
        $table=$pe+24+[BitConverter]::ToUInt16($Bytes,$pe+20)
        function Rva-Offset([uint32]$Rva) {
            for($i=0;$i -lt $sections;$i++) {
                $s=$table+$i*40
                $va=[BitConverter]::ToUInt32($Bytes,$s+12)
                $size=[BitConverter]::ToUInt32($Bytes,$s+16)
                if($Rva -ge $va -and [uint64]$Rva -lt [uint64]$va+$size) {
                    return [int]([BitConverter]::ToUInt32($Bytes,$s+20)+$Rva-$va)
                }
            }
            throw 'Export RVA is outside a file-backed section'
        }
        $exports=Rva-Offset ([BitConverter]::ToUInt32($Bytes,$pe+120))
        $count=[BitConverter]::ToUInt32($Bytes,$exports+24)
        if($count -gt 1024) { return $false }
        $names=Rva-Offset ([BitConverter]::ToUInt32($Bytes,$exports+32))
        for($i=0;$i -lt $count;$i++) {
            $at=Rva-Offset ([BitConverter]::ToUInt32($Bytes,$names+$i*4))
            if($at+14 -le $Bytes.Length -and [Text.Encoding]::ASCII.GetString($Bytes,$at,14) -eq "InitializeASI`0") { return $true }
        }
    } catch { return $false }
    return $false
}
$gitLines=& git -C $root -c core.quotepath=false ls-files --cached --others --exclude-standard -z
if($LASTEXITCODE) { throw 'Cannot enumerate the local Git candidate' }
$candidates=(@($gitLines) -join "`n") -split "`0" | Where-Object { $_ } | Sort-Object -Unique
$candidateNames=@{}
foreach($name in $candidates) { if($name.Replace('\','/') -match $allowed) { $candidateNames[$name.Replace('\','/')]=$true } }
$ignoredTracked=& git -C $root ls-files -ci --exclude-standard
if($LASTEXITCODE) { throw 'Cannot check tracked ignored files' }
foreach($name in $ignoredTracked) { Add-Finding $name 'Ignored file is already tracked' }
foreach($name in $candidates) {
    $name=$name.Replace('\','/')
    if($name -notmatch $allowed -or $name.Split('/') -contains '..') { Add-Finding $name 'Outside publication allowlist'; continue }
    $full=Join-Path $root $name
    $item=Get-Item -LiteralPath $full -Force
    $cursor=$item
    $linked=$false
    while($cursor -and $cursor.FullName -ne $root) {
        if($cursor.Attributes -band [IO.FileAttributes]::ReparsePoint) { $linked=$true; break }
        $cursor=if($cursor.PSIsContainer) { $cursor.Parent } else { $cursor.Directory }
    }
    if($linked) { Add-Finding $name 'Linked path requires separate review'; continue }
    if($item.Length -gt 10MB) { Add-Finding $name 'Unexpectedly large source/media file'; continue }
    $bytes=[IO.File]::ReadAllBytes($full)
    Inspect-Bytes $name $bytes
    Inspect-Links $name ([Text.Encoding]::UTF8.GetString($bytes)) $candidateNames
    $manifest.Add([pscustomobject]@{path=$name;bytes=$bytes.Length;sha256=(Get-FileHash -LiteralPath $full).Hash.ToLowerInvariant()})
}
$releaseEntries=@('GTA3.ShopRadar.asi','GTA3.ShopRadar.ini','README.md','INSTALL.md','CHANGELOG.md','LICENSE','THIRD_PARTY.md','SECURITY.md','docs/VALIDATION.md','docs/RESEARCH.md','docs/BUGS.md','docs/media/palette.svg','docs/media/bomb-icons.svg')
$releaseNames=@{}
foreach($name in $releaseEntries) { $releaseNames[$name]=$true }
if($ReleaseZip) {
    Add-Type -AssemblyName System.IO.Compression.FileSystem
    $zip=[IO.Compression.ZipFile]::OpenRead((Resolve-Path -LiteralPath $ReleaseZip).Path)
    try {
        $seen=@{}
        foreach($entry in $zip.Entries) {
            $name=$entry.FullName
            if($name -cnotin $releaseEntries -or $seen.ContainsKey($name)) { Add-Finding $name 'Unexpected or duplicate release entry'; continue }
            $seen[$name]=$true
            if($entry.Length -gt 10MB) { Add-Finding $name 'Oversized release entry'; continue }
            $stream=$entry.Open(); $memory=[IO.MemoryStream]::new()
            try { $stream.CopyTo($memory); $bytes=$memory.ToArray() } finally { $stream.Dispose(); $memory.Dispose() }
            Inspect-Bytes $name $bytes
            Inspect-Links $name ([Text.Encoding]::UTF8.GetString($bytes)) $releaseNames
            if($name -eq 'GTA3.ShopRadar.asi' -and !(Test-ModPE $bytes)) { Add-Finding $name 'Not an x86 PE DLL exporting InitializeASI' }
        }
        foreach($name in $releaseEntries) { if(!$seen.ContainsKey($name)) { Add-Finding $name 'Required release entry missing' } }
    } finally { $zip.Dispose() }
}
$build=Join-Path $root 'build'
New-Item -ItemType Directory -Force -Path $build | Out-Null
$report=[pscustomobject]@{candidateFiles=@($manifest.ToArray());findings=@($findings.ToArray());releaseChecked=[bool]$ReleaseZip;networkUsed=$false}
$report | ConvertTo-Json -Depth 5 | Set-Content -LiteralPath (Join-Path $build 'publication-audit.json') -Encoding utf8
if($findings.Count) {
    $findings.ToArray() | Sort-Object path,reason | Format-Table path,reason -AutoSize
    throw "Publication check failed with $($findings.Count) findings. Matched values were not printed."
}
Write-Output "PASS: $($manifest.Count) candidate files checked; no configured disclosure patterns found. Offline report: build/publication-audit.json"
Write-Output 'Review images and future commit metadata/history manually before publishing. No upload performed.'

