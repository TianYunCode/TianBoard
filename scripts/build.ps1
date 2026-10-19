#Requires -Version 5.1
# ======================================================================
# build.ps1 — CMake + Ninja 构建（必须在 VS2022 x64 开发者环境里跑）
#
#   powershell -ExecutionPolicy Bypass -F scripts\build.ps1
#   powershell -ExecutionPolicy Bypass -F scripts\build.ps1 -Configuration debug -Deploy
#   .\scripts\build.ps1 -Fresh            # 删掉二进制目录重新配置
#
# 与 CMakePresets.json 的对应关系：debug -> build-debug/，release ->
# build-release/（RelWithDebInfo，带 pdb）。产物一律落 build\bin\TianBoard.exe，
# etc/ library/ customizations/ Language/ 由 POST_BUILD 铺在 exe 同级 —— 程序按
# 可执行文件所在目录解析这些资源，测试（tst_smoke）也按这个前提校验。
#
# IDE / 现有工具链不必用本脚本：那一对 VS 预设（vs2022-debug / vs2022-release）
# 用多配置生成器，中间产物在 build/，产物同样落 build\bin\。
#
# cl/cmake/ninja 的路径默认 D:\ 盘安装位置，换机用环境变量覆盖：
#   TIANBOARD_QT_ROOT / TIANBOARD_CMAKE / TIANBOARD_NINJA / TIANBOARD_VSDEVCMD
# ======================================================================
param(
    [ValidateSet('debug', 'release')]
    [string]$Configuration = 'release',
    [switch]$Deploy,
    [switch]$NoTests,
    # 清一遍再配置（极少需要：CMakeLists 变动 CMake 会自动重新配置；只有换工具链
    # 或缓存坏了才用）。
    [switch]$Fresh
)

$ErrorActionPreference = 'Stop'

$Root = Split-Path -Parent $PSScriptRoot
$QtRoot  = if ($env:TIANBOARD_QT_ROOT)  { $env:TIANBOARD_QT_ROOT }  else { 'D:\Qt\6.9.3\msvc2022_64' }
$CMake   = if ($env:TIANBOARD_CMAKE)    { $env:TIANBOARD_CMAKE }    else { 'D:\Qt\Tools\CMake_64\bin\cmake.exe' }
$Ninja   = if ($env:TIANBOARD_NINJA)    { $env:TIANBOARD_NINJA }    else { 'D:\Qt\Tools\Ninja\ninja.exe' }
$VsDevCmd = if ($env:TIANBOARD_VSDEVCMD) { $env:TIANBOARD_VSDEVCMD } else { 'D:\Microsoft Visual Studio\2022\Community\Common7\Tools\VsDevCmd.bat' }
$WinDeployQt = Join-Path $QtRoot 'bin\windeployqt.exe'
$BuildDir = Join-Path $Root "build-$Configuration"

foreach ($required in @($CMake, $Ninja, $VsDevCmd)) {
    if (-not (Test-Path $required)) { throw "Missing build tool: $required" }
}

if ($Configuration -eq 'release') { $BuildType = 'RelWithDebInfo' } else { $BuildType = 'Debug' }

# ---- 头文件改动必须让所有 include 它的 .cpp 重编 ----
# 中文版 cl.exe 的 /showIncludes 提示是“注意: 包含文件:”，而 ninja 的 deps = msvc
# 只认英文前缀 "Note: including file:"，于是头文件依赖被记成 0 —— 改一个 .h 只有
# 直接改到的 .cpp 重编，其它 include 它的 .cpp 不动。后果不只是“少编一个文件”：
# 同一个类在不同 .obj 里成员偏移不一致，读成员读到隔壁字段，编译链接一句话都不报，
# 跑起来才崩。所以按时间戳兜底：源码树里有比最旧 .obj 新的头文件就整体重建，
# 宁可多编几十秒，也不产出布局不一致的目标文件。判据用 .obj 而不是 build.ninja ——
# 后者 configure 过一次时间戳就变成“刚刚”，会放过本次。
if (-not $Fresh -and (Test-Path $BuildDir)) {
    $objs = @(Get-ChildItem -Path $BuildDir -Recurse -Filter '*.obj' -File -ErrorAction SilentlyContinue)
    if ($objs.Count -eq 0) {
        Write-Host "构建目录里没有目标文件, 跳过头文件时间戳检查" -ForegroundColor DarkGray
    } else {
        $oldestObjTime = ($objs | Sort-Object LastWriteTimeUtc | Select-Object -First 1).LastWriteTimeUtc
        $roots = @('src', 'tests') | ForEach-Object { Join-Path $Root $_ } | Where-Object { Test-Path $_ }
        # 不用 -Include（配合 -Path <目录> 有怪癖），换成显式扩展名判断。
        $newerHeaders = @(Get-ChildItem -Path $roots -Recurse -File -ErrorAction SilentlyContinue |
            Where-Object { ($_.Extension -eq '.h' -or $_.Extension -eq '.hpp') -and $_.LastWriteTimeUtc -gt $oldestObjTime })
        if ($newerHeaders.Count -gt 0) {
            $sample = ($newerHeaders | Select-Object -First 3 | ForEach-Object { $_.Name }) -join ', '
            Write-Host "有 $($newerHeaders.Count) 个头文件比最旧的目标文件新($sample), 整体重建以免目标文件布局不一致" -ForegroundColor Yellow
            Remove-Item -Recurse -Force $BuildDir
        }
    }
}

if ($Fresh -and (Test-Path $BuildDir)) { Remove-Item -Recurse -Force $BuildDir }
New-Item -ItemType Directory -Force -Path $BuildDir | Out-Null

# configure + build 都要在 cl/link 可见的 VS 开发者环境里跑：用一个临时 .cmd
# 先 call VsDevCmd 再执行。
$CommandFile = Join-Path $BuildDir 'build.cmd'
@"
@echo off
call "$VsDevCmd" -arch=x64 -host_arch=x64
if errorlevel 1 exit /b %errorlevel%
"$CMake" --preset $Configuration
if errorlevel 1 exit /b %errorlevel%
"$CMake" --build "$BuildDir"
exit /b %errorlevel%
"@ | Set-Content -Path $CommandFile -Encoding Ascii

& cmd.exe /d /c $CommandFile
if ($LASTEXITCODE -ne 0) { throw "Build failed with exit code $LASTEXITCODE" }

# ---- 校验产物布局：exe 与四个运行期目录同级 ----
$BinDir = Join-Path $Root 'build\bin'
$Exe = Join-Path $BinDir 'TianBoard.exe'
if (-not (Test-Path $Exe)) { throw "Missing build artifact: $Exe" }
$missing = @('etc', 'library', 'customizations') | Where-Object { -not (Test-Path (Join-Path $BinDir $_)) }
if ($missing) { throw "Missing runtime directories next to the exe: $($missing -join ', ') (POST_BUILD 铺的，检查 res/)" }

if (-not $NoTests) {
    # ctest 从同一个二进制目录跑；-C 只在多配置生成器上有意义，Ninja 是单配置。
    & $CMake --build "$BuildDir" --target test
    if ($LASTEXITCODE -ne 0) { throw "ctest failed with exit code $LASTEXITCODE" }
}

if ($Deploy) {
    if (-not (Test-Path $WinDeployQt)) { throw "Missing deployment tool: $WinDeployQt" }
    # 参数与 CMake 的 deploy 目标保持一致：本应用用 QtWebEngine（Chromium 进程与
    # 资源由 windeployqt 自己按链接关系带上），翻译走自建的 Language/ 目录。
    & $WinDeployQt --no-translations --compiler-runtime $Exe
    if ($LASTEXITCODE -ne 0) { throw "windeployqt failed: $LASTEXITCODE" }
}

Write-Host "Build complete: $Exe" -ForegroundColor Green
Write-Host "Runtime dirs next to exe: etc, library, customizations$(if (Test-Path (Join-Path $BinDir 'Language')) { ', Language' })" -ForegroundColor Green
