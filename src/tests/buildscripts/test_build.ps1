$ErrorActionPreference = 'Stop'
$scriptPath = Join-Path (Get-Location) 'build.ps1'
$shell = (Get-Process -Id $PID).Path

& $shell -NoProfile -NonInteractive -ExecutionPolicy Bypass -File $scriptPath -Help
if ($LASTEXITCODE -ne 0) { throw 'Help failed' }
& $shell -NoProfile -NonInteractive -ExecutionPolicy Bypass -File $scriptPath -UnknownOption
if ($LASTEXITCODE -eq 0) { throw 'Unknown option accepted' }

$tokens = $null
$parseErrors = $null
$ast = [System.Management.Automation.Language.Parser]::ParseFile($scriptPath, [ref]$tokens, [ref]$parseErrors)
if ($parseErrors.Count) { throw ($parseErrors | Out-String) }
foreach ($statement in $ast.EndBlock.Statements) {
    if ($statement -is [System.Management.Automation.Language.FunctionDefinitionAst]) {
        Invoke-Expression $statement.Extent.Text
    }
}

function Get-ChildItem {
    param($Path, [switch]$Directory, $ErrorAction)
    @('6.8.3', '6.9.3', '6.10.0') | ForEach-Object {
        [pscustomobject]@{ Name = $_; FullName = "C:\Qt\$_" }
    }
}
function Test-Path {
    param($Path)
    if ($Path -eq 'C:\Qt') { return $true }
    if ($Path -match '6\.10\.0.*Qt6LinguistTools') { return $false }
    return $true
}

$qt = Get-InstalledQt -Minimum ([version]'6.9')
if ($qt.Version -ne '6.9.3' -or $qt.Compiler -ne 'msvc2022_64') {
    throw 'Qt discovery failed to skip an incomplete newer kit'
}
if (Get-InstalledQt -Minimum ([version]'6.11')) { throw 'Qt below minimum accepted' }
if (Get-InstalledQt -Minimum ([version]'6.10')) { throw 'Qt without LinguistTools accepted' }

function Invoke-AqtList {
    param([string[]]$Arguments)
    if ($Arguments[0] -eq 'list-qt' -and $Arguments -notcontains '--arch') {
        return '6.8.3 6.9.3 6.10.0 6.11.0'
    }
    if ($Arguments -contains '6.10.0') { return 'win64_mingw' }
    return 'win64_msvc2022_64'
}
function git {
    $global:LASTEXITCODE = 0
    if ($args -contains 'refs/tags/v6.11.0') { $global:LASTEXITCODE = 2 }
}
function Get-Command {
    param($Name, $ErrorAction)
    return $true
}

$qt = Get-QtVersion -Minimum ([version]'6.9')
if ($qt.Version -ne '6.9.3' -or $qt.Arch -ne 'win64_msvc2022_64') {
    throw 'Qt selection must reject prereleases and versions without MSVC'
}
Write-Host 'PowerShell build script tests passed'
