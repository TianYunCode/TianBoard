#Requires -Version 5.1
# ======================================================================
# make-installer.ps1 — 打安装包: Release 构建 -> 组装 stage -> ISCC 出 setup.exe
#
#   powershell -ExecutionPolicy Bypass -F packaging\make-installer.ps1 -Build
#   powershell -ExecutionPolicy Bypass -F packaging\make-installer.ps1 -Version 1.1.0
#
# 打包对象只有一个可执行：build\bin\TianBoard.exe 与它同级的运行期目录
# etc/ library/ customizations/ Language/，加上 windeployqt 收来的 Qt 运行库。
# TianBoard 没有 DLL 插件（模块在链接期就并进 exe），所以没有 plugins/ 目录。
#
# 版本号单一事实来源 = 根 CMakeLists.txt 的 project(TianBoard VERSION x.y.z)。
# 注意 src/core/Application.cpp 里那句 "V1.1.0"（关于对话框显示的串）是硬编码的，
# 发版时要一起核对 —— 脚本会把它打印出来提醒，不自动改。
# ======================================================================
param(
    [switch]$Build,
    [string]$Version,
    [string]$QtBin = 'D:\Qt\6.9.3\msvc2022_64\bin',
    [string]$VsDevCmd = 'D:\Microsoft Visual Studio\2022\Community\Common7\Tools\VsDevCmd.bat',
    [string]$Iscc = '',
    [string]$MsvcRedist = 'D:\Microsoft Visual Studio\2022\Community\VC\Redist\MSVC'
)

$ErrorActionPreference = 'Stop'

$Root = Split-Path -Parent (Split-Path -Parent $MyInvocation.MyCommand.Path)
$BinDir = Join-Path $Root 'build\bin'
$DistDir = Join-Path $Root 'build\dist'
$StageDir = Join-Path $DistDir 'stage'
$IssPath = Join-Path $Root 'packaging\TianBoard.iss'

# ---- 版本号 ----
if (-not $Version) {
    $cmake = Get-Content (Join-Path $Root 'CMakeLists.txt') -Raw
    if ($cmake -notmatch 'project\(\s*TianBoard[\s\S]{0,200}?VERSION\s+(\d+(?:\.\d+)+)') {
        throw '在根 CMakeLists.txt 的 project(TianBoard ... VERSION ...) 里没找到版本号。'
    }
    $Version = $Matches[1]
}
"Version: $Version"

$hardcoded = Select-String -Path (Join-Path $Root 'src\core\Application.cpp') -Pattern '"V(\d+\.\d+\.\d+)"'
if ($hardcoded) {
    "  关于对话框里的硬编码版本串: $($hardcoded.Matches[0].Groups[1].Value) —— 与本安装包版本 $($Version) 不一致时请人工核对(脚本不自动改源码)。"
}

# ---- 1. Release 构建 ----
if ($Build) {
    "Building release (RelWithDebInfo)..."
    & "$Root\scripts\build.ps1" -Configuration release -NoTests
    if ($LASTEXITCODE -ne 0) { throw "Build failed, exit code $LASTEXITCODE" }
}

$Exe = Join-Path $BinDir 'TianBoard.exe'
if (-not (Test-Path $Exe)) { throw "Missing build artifact: $Exe (run with -Build)" }

# ---- 2. 组装 stage：先原样拷 build\bin（exe + POST_BUILD 铺的四个运行期目录），
#      再在 stage 里跑 windeployqt，避免污染构建目录 ----
"Staging to $StageDir..."
if (Test-Path $StageDir) { Remove-Item $StageDir -Recurse -Force }
New-Item -ItemType Directory -Path $StageDir | Out-Null
Copy-Item -Path (Join-Path $BinDir '*') -Destination $StageDir -Recurse -Force

$stageExe = Join-Path $StageDir 'TianBoard.exe'
"Running windeployqt..."
$wdqt = Join-Path $QtBin 'windeployqt.exe'
if (-not (Test-Path $wdqt)) { throw "Missing deployment tool: $wdqt" }
$env:PATH = "$QtBin;$env:PATH"
# windeployqt 会往 stderr 写告警；用 cmd 合并两路输出，别让 PowerShell 当成终止错误。
# 显式点名的模块是“强制部署”：本应用链接 WebEngine（Chromium）、Svg、Multimedia、
# Pdf、PrintSupport，靠自动探测容易漏（Qt6 里 -webenginewidgets 才是有效参数名，
# 没有 -webengine 这个写法）。工程零 QML 业务，不传 -quick/-qml。
$wdqtArgs = '--release --no-translations --compiler-runtime -svg -multimedia -pdf -printsupport -webenginewidgets'
$nul = & cmd.exe /c "`"$wdqt`" $wdqtArgs `"$stageExe`" 2>&1" | Out-String -Stream | ForEach-Object {
    $line = $_.Trim()
    if ($line -match 'error|Error|Fatal|fatal') { "  windeployqt: $line" }
}

# ---- MSVC 运行库 app-local（与 SPM_PTS 同法：拷 DLL，删掉 vc_redist.exe）----
"Copying MSVC runtime..."
$crtDir = Get-ChildItem $MsvcRedist -Directory -ErrorAction SilentlyContinue |
    Where-Object { $_.Name -match '^\d+\.' -and (Test-Path (Join-Path $_.FullName 'x64\Microsoft.VC143.CRT')) } |
    Sort-Object Name -Descending | Select-Object -First 1
if (-not $crtDir) { throw "在 $MsvcRedist 下找不到 Microsoft.VC143.CRT（VS2022 运行库目录），无法做 app-local 部署。" }
$crtPath = Join-Path $crtDir.FullName 'x64\Microsoft.VC143.CRT'
Copy-Item (Join-Path $crtPath '*.dll') $StageDir
foreach ($dll in @('msvcp140.dll', 'vcruntime140.dll', 'vcruntime140_1.dll', 'concrt140.dll')) {
    if (-not (Test-Path (Join-Path $StageDir $dll))) { throw "Missing MSVC runtime: $dll" }
}
Remove-Item (Join-Path $StageDir 'vc_redist.x64.exe') -Force -ErrorAction SilentlyContinue

# ---- 校验必需文件 ----
# 硬失败：少了这些要么起不来、要么资源解析不到（PlatformUtils::
# applicationResourcesDirectory() 就是可执行文件所在目录，tst_smoke 按同一前提校验）。
$must = @(
    'TianBoard.exe',
    'Qt6Core.dll', 'Qt6Gui.dll', 'Qt6Widgets.dll', 'Qt6Network.dll',
    'Qt6WebEngineCore.dll', 'Qt6WebEngineWidgets.dll',
    'Qt6Svg.dll', 'Qt6Pdf.dll', 'Qt6Multimedia.dll', 'Qt6PrintSupport.dll',
    'platforms\qwindows.dll', 'imageformats', 'styles',
    'etc\TianBoard.config', 'library', 'customizations'
)
$missing = $must | Where-Object { -not (Test-Path (Join-Path $StageDir $_)) }
if ($missing) { throw "stage 目录缺文件: $($missing -join ', ') —— 构建或 windeployqt 没铺全，别发出去。" }

# 只警告：这些依赖 windeployqt 的版本与探测方式，路径名一变就误报，卡住发版不值得。
foreach ($soft in @('QtWebEngineProcess.exe', 'Qt6Qml.dll', 'Qt6Quick.dll',
                    'styles\qmodernwindowsstyle.dll', 'vcruntime140.dll')) {
    $hit = @(Get-ChildItem $StageDir -Recurse -File -Filter (Split-Path $soft -Leaf) -ErrorAction SilentlyContinue)
    if ($hit.Count -eq 0) { "  提醒: stage 里没有 $($soft)（人工确认一下再打包）。" }
}
if (-not (Test-Path (Join-Path $StageDir 'Language'))) {
    "  警告: stage 里没有 Language/ —— 安装包将只有英文界面（lrelease 没跑或 Qt6LinguistTools 没找到）。"
}

$files = Get-ChildItem $StageDir -Recurse -File
"  stage: $($files.Count) files, $([int](($files | Measure-Object Length -Sum).Sum / 1MB)) MB"

# ---- PDB 归档（现场崩溃要能对着 pdb 看栈）----
$pdb = Join-Path $BinDir 'TianBoard.pdb'
if (Test-Path $pdb) {
    Copy-Item $pdb (Join-Path $DistDir "TianBoard.pdb-$Version") -Force
    "  Archived TianBoard.pdb -> dist\TianBoard.pdb-$Version"
}

# ---- 3. 打安装包 ----
# ISCC.exe 找不到就明确失败，绝不“跳过打包”静默返回成功。
if (-not $Iscc) {
    $candidates = @(
        'D:\Inno Setup 6\ISCC.exe',
        (Join-Path ${env:ProgramFiles} 'Inno Setup 6\ISCC.exe'),
        (Join-Path ${env:ProgramFiles(x86)} 'Inno Setup 6\ISCC.exe')
    )
    $cmd = Get-Command iscc -CommandType Application -ErrorAction SilentlyContinue
    if ($cmd) { $candidates = @($cmd.Source) + $candidates }
    $Iscc = $candidates | Where-Object { $_ -and (Test-Path $_) } | Select-Object -First 1
}
if (-not $Iscc) {
    throw @"
找不到 Inno Setup 6 的 ISCC.exe —— 安装包没有生成（不是“跳过”，是失败）。
前面组装好的 stage 目录留在: $StageDir
装好 Inno Setup 后重跑，或显式指定路径:
  powershell -ExecutionPolicy Bypass -F packaging\make-installer.ps1 -Iscc 'D:\Inno Setup 6\ISCC.exe'
安装: winget install JRSoftware.InnoSetup
"@
}
"Inno Setup building with: $Iscc"
$issArgs = @("/DAppVersion=$Version", "/DStageDir=$StageDir", "/DOutDir=$DistDir", $IssPath)
& $Iscc $issArgs 2>&1 | Out-String -Stream | Select-String -Pattern 'Error|error|Successful|Total size' | ForEach-Object { "  $($_.Line)" }

$setup = Join-Path $DistDir "TianBoard-$Version-x64-setup.exe"
if (-not (Test-Path $setup)) { throw "Didn't produce installer: $setup" }
$sizeMB = [int]((Get-Item $setup).Length / 1MB)
"Done: $setup ($sizeMB MB)"
