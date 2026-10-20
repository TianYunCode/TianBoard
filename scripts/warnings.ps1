#Requires -Version 5.1
# ======================================================================
# warnings.ps1 — 用 MSVC /W4 全量编一遍，把自有代码的编译告警按编号列出来
#
#   .\scripts\warnings.ps1                 # 配 build-w4/ + 全量编 + 汇总告警
#   .\scripts\warnings.ps1 -Clean        # 先清 build-w4/ 再编（要精确基线时用）
#   .\scripts\warnings.ps1 -Strict        # 有一条自有代码告警就非零退出
#
# 为什么单独一个脚本而不是把 /W4 开进日常构建：
#   1. /W4 对 Qt 头文件也会报，Qt 那批 include 是以 /external:I 传的，不计入；
#      third/（quazip、zlib）走普通 -I，会被 /W4 刷屏，所以按路径过滤掉。
#   2. 日常构建（verify.ps1）要的是"能编过 + 测试绿"，把告警当错误会让一次
#      Qt 升级就整仓编不过；这里只做"看清并记账"，门禁仍在 verify.ps1。
#   3. 独立目录 build-w4/ 与 build/、build-release/ 互不干扰，产物也不参与门禁判定。
#
# 判据口径：以 src/ 或 tests/ 开头的源文件上的告警才算自有代码；C4996（Qt 已废弃
# API）历史上攒了 59 处，已全部换掉，所以 -Strict 现在是可达的。
# ======================================================================
param(
    [switch]$Clean,
    [switch]$Strict,
    [switch]$SkipBuild,
    [string]$CMake = 'D:\Qt\Tools\CMake_64\bin\cmake.exe'
)

$ErrorActionPreference = 'Stop'

$Root = Split-Path -Parent $PSScriptRoot
Set-Location $Root

$BuildDir = Join-Path $Root 'build-w4'
$Log = Join-Path $env:TEMP 'tianboard-w4.log'
$WarningFlags = '-W4 -wd4100'   # 4100：形参未引用，Qt 的重写里全是这种，纯噪声

if (-not $SkipBuild) {
    if (-not (Test-Path (Join-Path $BuildDir 'CMakeCache.txt'))) {
        Write-Host '==> configure build-w4 (/W4)' -ForegroundColor Cyan
        & $CMake --preset vs2022-debug -B $BuildDir -DCMAKE_CXX_FLAGS=$WarningFlags *> $Log
        if ($LASTEXITCODE -ne 0) { throw "配置失败，见 $Log" }
    }
    if ($Clean) {
        & $CMake --build $BuildDir --config Debug --target clean --parallel *> $Log
    }
    Write-Host '==> build (Debug, /W4)' -ForegroundColor Cyan
    & $CMake --build $BuildDir --config Debug --target TianBoard --parallel 2>&1 |
        Tee-Object -FilePath $Log |
        ForEach-Object { if ("$_" -match 'error C|LNK[0-9]|fatal error') { Write-Host "  $_" -ForegroundColor Red } }
    if ($LASTEXITCODE -ne 0) { throw "/W4 构建失败（exit $LASTEXITCODE），日志：$Log" }
}

# ---- 汇总：只统计 src/ 与 tests/ 下的告警 ----
$prefix = (Resolve-Path $Root).Path + [IO.Path]::DirectorySeparatorChar
$rows = @()
foreach ($line in (Get-Content $Log)) {
    $text = "$line".Trim('"')
    if ($text -notmatch ': (warning|error) C[0-9]{4}:') { continue }
    if ($text -match '^(.*?)\((\d+),\d+\): (warning|error) (C[0-9]{4}): ?(.*)$') {
        # 立刻把捕获组装进变量：PowerShell 每做一次 -match/-notmatch 都会重写 $Matches，
        # 而下面按路径过滤时还有一次 -notmatch，等到这里再取 $Matches 就全是空的了
        # （表现是行号统一打成 0，看不出真因）。
        $path   = $Matches[1].Trim()
        $lineno = [int]$Matches[2]
        $kind   = $Matches[3]
        $code   = $Matches[4]
        $msg    = ($Matches[5] -split ' \[')[0]
        if (-not $path.StartsWith($prefix, [StringComparison]::OrdinalIgnoreCase)) { continue }
        $rel = $path.Substring($prefix.Length).Replace('\', '/')
        if ($rel -notmatch '^(src|tests)/') { continue }
        $rows += [pscustomobject]@{ Code = $code; File = $rel; Line = $lineno; Kind = $kind; Text = $msg }
    }
}

$errors = @($rows | Where-Object { $_.Kind -eq 'error' })
if ($errors.Count -gt 0) {
    $errors | Select-Object -First 10 | ForEach-Object { Write-Host "  $($_.File):$($_.Line) $($_.Code)" -ForegroundColor Red }
    throw "/W4 构建有 $($errors.Count) 个错误"
}

if ($rows.Count -eq 0) {
    Write-Host '  OK: src/ 与 tests/ 在 /W4 下零告警' -ForegroundColor Green
    exit 0
}

Write-Host ("自有代码告警 {0} 条：" -f $rows.Count) -ForegroundColor Yellow
$rows | Group-Object Code | Sort-Object Count -Descending | ForEach-Object {
    Write-Host ("  {0,4}  {1}" -f $_.Count, $_.Name)
    $_.Group | Select-Object -First 6 | ForEach-Object { Write-Host ("          {0}:{1}  {2}" -f $_.File, $_.Line, $_.Text.Substring(0, [Math]::Min(96, $_.Text.Length))) }
}
if ($Strict) { throw '/W4 下自有代码仍有告警（-Strict）' }
exit 0
