#Requires -Version 5.1
# ======================================================================
# format.ps1 — 用仓库根 .clang-format 格式化 C/C++ 源码（Windows 版）
#
#   .\scripts\format.ps1                    # 就地格式化 src/ 与 tests/
#   .\scripts\format.ps1 -Check             # 只查不改；0=干净 1=有待格式化 2=环境问题
#   .\scripts\format.ps1 -Path src\utils    # 只处理指定目录或文件
#
# 永远排除：build*/ 构建产物、third/ 内联第三方（quazip/zlib/openssl/qtsingleapplication
# 自带风格，动了它们就把 vendor 代码和自家代码混在一个 diff 里，回滚都分不开）。
#
# 注意：全仓（src/ + tests/，343 个文件）已在 2026-10-20 用 clang-format 22.1.8 跑过
# 一次，此后日常约定是提交前 `-Check` 自检。格式化结果由 clang-format 版本决定，
# 升版本可能把整棵树重排，所以升版本要单独一次全量提交，不与功能改动混批。
#
# 踩坑记录（别改回去）：给 clang-format 传参时，把参数写成 `-style=$var` 这种
# “看起来像参数”的 token，PowerShell 不展开变量，clang-format 收到的值是字面
# `$style`，报 Invalid value for -style。所以下面一律走 splat 数组（@args 会展开）。
# ======================================================================
param(
    [switch]$Check,
    [string[]]$Path
)

$ErrorActionPreference = 'Stop'

$repoRoot = Split-Path -Parent $PSScriptRoot
$config = Join-Path $repoRoot '.clang-format'
if (-not (Test-Path $config)) { Write-Error "找不到 $config"; exit 2 }

# ---- 找 clang-format：PATH 优先，然后 VS 自带那份，最后常见 LLVM 安装位 ----
$clangFormat = $null
$cmd = Get-Command clang-format -CommandType Application -ErrorAction SilentlyContinue
if ($cmd) { $clangFormat = $cmd.Source }
if (-not $clangFormat -and ${env:ProgramFiles(x86)}) {
    $vswhere = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio\Installer\vswhere.exe'
    if (Test-Path $vswhere) {
        $found = & $vswhere -latest -products * -find 'VC\Tools\Llvm*\bin\clang-format.exe' | Select-Object -First 1
        if ($found -and (Test-Path $found)) { $clangFormat = $found }
    }
}
if (-not $clangFormat -and (Test-Path 'C:\Program Files\LLVM\bin\clang-format.exe')) {
    $clangFormat = 'C:\Program Files\LLVM\bin\clang-format.exe'
}
if (-not $clangFormat) {
    Write-Error '找不到 clang-format：装一个（winget install LLVM.LLVM）或放进 PATH。'
    exit 2
}
Write-Host "clang-format: $clangFormat"

# ---- 收集源码：默认只碰 src/ 与 tests/ ----
$roots = if ($Path) { $Path } else { @((Join-Path $repoRoot 'src'), (Join-Path $repoRoot 'tests')) }
$extensions = '.c', '.cc', '.cpp', '.cxx', '.h', '.hpp', '.hxx'
$files = @()
foreach ($root in $roots) {
    if (-not (Test-Path $root)) { Write-Error "路径不存在: $root"; exit 2 }
    $files += Get-ChildItem -Path $root -Recurse -File | Where-Object {
        $_.Extension -in $extensions -and
        $_.FullName -notmatch '\\build(-[^\\]*)?\\' -and $_.FullName -notmatch '\\third\\'
    }
}
$files = @($files | Sort-Object -Property FullName -Unique)
if ($files.Count -eq 0) { Write-Host '没有需要格式化的 C/C++ 源文件'; exit 0 }

$allPaths = @($files | ForEach-Object { $_.FullName })

if ($Check) {
    # --dry-run --Werror：不落盘，有要改的就非零退出。
    $checkArgs = @('--dry-run', '--Werror', '--style=file') + $allPaths
    & $clangFormat @checkArgs
    if ($LASTEXITCODE -eq 0) { Write-Host ("检查 {0} 个文件: 全部符合 .clang-format" -f $files.Count); exit 0 }
    Write-Host ("检查 {0} 个文件: 有需要格式化的（明细见上）" -f $files.Count)
    exit 1
}

# ---- 就地格式化：分批传参，文件多了也不撑爆命令行长度上限 ----
for ($i = 0; $i -lt $files.Count; $i += 100) {
    $last = [Math]::Min($i + 99, $files.Count - 1)
    $batch = @('-i', '--style=file') + @($allPaths[$i..$last])
    & $clangFormat @batch
    if ($LASTEXITCODE -ne 0) { Write-Error "clang-format 执行失败（第 $i 个文件起的批次）"; exit 2 }
}
Write-Host ("已格式化 {0} 个文件，用 git diff 查看改动" -f $files.Count)
