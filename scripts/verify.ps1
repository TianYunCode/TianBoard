#Requires -Version 5.1
# ======================================================================
# verify.ps1 — 一条命令做完"改动是否成立"的判断
#
#   powershell -ExecutionPolicy Bypass -F scripts\verify.ps1
#   .\scripts\verify.ps1 -Configuration release      # 换成 Ninja 那条工具链
#   .\scripts\verify.ps1 -SkipFormat                # 不跑 format 检查
#
# 为什么要有这个脚本（都是踩出来的）：
#  1. MSBuild /m 下某个工程编译失败时其它工程继续跑，主 exe 不重新链接，磁盘上还是
#     上一次的好产物 —— 这时候 ctest 照样全绿。所以判定必须看两件事：**构建日志里的
#     error/LNK 计数为 0**，以及**没有输入比产物新**（树上没改动时 MSBuild 本来
#     就不重链接，所以不能拿"exe 离现在几分钟"当判据）。两样缺一不可。
#     新鲜度分两套：src/ 与根构建文件对 TianBoard.exe，tests/tst_x.cpp 对各自的
#     tst_x.exe——测试源不是应用输入，混在一起会把格式化工具碰过的时间戳误判成"没重编"。
#  2. 构建命令的退出码很容易被后面一句 shell 吃掉（`...; grep ...; true` 会让后台
#     任务报"成功"），所以这里每步都显式取 $LASTEXITCODE。
#  3. 排版门禁（format.ps1 -Check）和构建一起跑，才不会攒出"以后再格式化"的债。
#  4. 另外三条静态门禁都在构建之后、测试之前：无平台条件编译（2b）、源码编码合规（2d）、
#     默认配置的每个 Section/Key 都有代码注册（2c）。三条都是历史上真攒出来的债。
# ======================================================================
param(
    [ValidateSet('debug', 'release')]
    [string]$Configuration = 'debug',
    [switch]$SkipFormat,
    [string]$CMake = 'D:\Qt\Tools\CMake_64\bin\cmake.exe',
    [string]$QtRoot = 'D:\Qt\6.9.3\msvc2022_64'
)

$ErrorActionPreference = 'Stop'

$Root = Split-Path -Parent $PSScriptRoot
Set-Location $Root

function Invoke-Step {
    param([string]$Name, [scriptblock]$Body)
    Write-Host "==> $Name" -ForegroundColor Cyan
    & $Body
    $code = $LASTEXITCODE
    if ($code -ne 0) { throw "$Name 失败（exit $code）" }
}

# 与 Invoke-Step 同，但失败时先把日志里的报错行贴出来：只给一个退出码，还得人去
# 翻临时文件，而这一步的意义正是"告诉你是哪个文件哪一行挂了"。日志路径取脚本级
# 的 $Log（写成参数曾经漏传，结果失败时反而在读日志上再报一次错）。
function Invoke-LoggedStep {
    param([string]$Name, [scriptblock]$Body)
    Write-Host "==> $Name" -ForegroundColor Cyan
    & $Body
    $code = $LASTEXITCODE
    if ($code -ne 0) {
        $bad = @()
        if ($script:Log -and (Test-Path $script:Log)) {
            $bad = @(Get-Content $script:Log -ErrorAction SilentlyContinue |
                Where-Object { $_ -match 'error C[0-9]{4}|error LNK[0-9]{4}|fatal error|CMake Error' })
        }
        $bad | Select-Object -First 12 | ForEach-Object { Write-Host "  $_" -ForegroundColor Red }
        if ($bad.Count -eq 0) { Write-Host "  日志里没有明显的 error 行，完整日志见 $script:Log" -ForegroundColor Yellow }
        throw "$Name 失败（exit $code）"
    }
}

# ---- 1. 配置 + 构建 ----
# Ninja 那条工具链需要 VsDevCmd 环境，交给 build.ps1 自己处理；VS 生成器不需要。
if ($Configuration -eq 'release') {
    $BuildDir = Join-Path $Root 'build-release'
    $Exe = Join-Path $BuildDir 'bin\RelWithDebInfo\TianBoard.exe'
    $Log = Join-Path $env:TEMP 'tianboard-verify-release.log'

    Invoke-LoggedStep 'configure + build (Ninja, RelWithDebInfo)' {
        # 边跑边往屏上打，同时留一份日志供失败时摘错误行。
        & powershell -NoProfile -ExecutionPolicy Bypass -F (Join-Path $PSScriptRoot 'build.ps1') `
            -Configuration release -NoTests 2>&1 | Tee-Object -FilePath $Log
    }
} else {
    $BuildDir = Join-Path $Root 'build'
    $Exe = Join-Path $BuildDir 'bin\Debug\TianBoard.exe'
    $Log = Join-Path $env:TEMP 'tianboard-verify-debug.log'

    Invoke-LoggedStep 'configure (vs2022-debug)' {
        & $CMake --preset vs2022-debug *> $Log
    }
    Invoke-LoggedStep 'build (Debug)' {
        # 构建默认目标（ALL_BUILD）：只 --target TianBoard 的话测试可执行不会被编，
        # 全新构建目录下的 ctest 会全报 Not Run。而"链接没发生"由下一步的时间戳判据抓。
        & $CMake --build $BuildDir --config Debug -- /m /v:m *> $Log
    }
}

# ---- 2. 真正的判定：日志干净 + 产物是新的 ----
$logText = Get-Content $Log -Raw -ErrorAction SilentlyContinue
$badLines = @()
if ($logText) {
    $badLines = @($logText -split "`r?`n" | Where-Object { $_ -match 'error C[0-9]{4}|error LNK[0-9]{4}|fatal error' })
}
if ($badLines.Count -gt 0) {
    $badLines | Select-Object -First 8 | ForEach-Object { Write-Host "  $_" -ForegroundColor Red }
    throw "构建日志里有 $($badLines.Count) 条 error（日志：$Log）"
}
if (-not (Test-Path $Exe)) { throw "没有产物：$Exe —— 构建根本没链接出可执行文件" }

# 判"产物是新的"不能看它离现在多久：树上没改动时 MSBuild 本来就不会重新链接。
# 正确的判据是**没有任何输入的修改时间晚于产物**——那样一旦某个 .cpp 编挂了、
# 链接没发生，这个判据就会红。
#
# 判据分两套，因为 tests/ 下的源文件不是 TianBoard.exe 的输入：tst_*.cpp 只进各自的
# 测试可执行，格式化工具碰一下它们的时间戳，不该把应用判成"没重编"。
$exeTime = (Get-Item $Exe).LastWriteTime
$inputs = @()
foreach ($dir in @('src')) {
    $inputs += Get-ChildItem -Path (Join-Path $Root $dir) -Recurse -File |
        Where-Object { $_.Extension -in '.cpp', '.h', '.qrc' -or $_.Name -eq 'CMakeLists.txt' }
}
$inputs += Get-Item (Join-Path $Root 'CMakeLists.txt'), (Join-Path $Root 'CMakePresets.json')
$stale = @($inputs | Where-Object { $_.LastWriteTime -gt $exeTime })
if ($stale.Count -gt 0) {
    $stale | Select-Object -First 6 | ForEach-Object { Write-Host "  比产物新: $($_.FullName.Substring($Root.Length + 1))" -ForegroundColor Red }
    throw "有 $($stale.Count) 个输入文件比 $([IO.Path]::GetFileName($Exe)) 新 —— 构建没把它们编进去，测试结果不可信"
}

# 每个测试用例跟自己那份可执行比；tests/CMakeLists.txt 跟最新的那份比。
$BinDir = Split-Path -Parent $Exe
$testSources = @()
foreach ($t in (Get-ChildItem -Path $BinDir -Filter 'tst_*.exe' -File)) {
    $own = Join-Path (Join-Path $Root 'tests') ($t.BaseName + '.cpp')
    if ((Test-Path $own) -and ((Get-Item $own).LastWriteTime -gt $t.LastWriteTime)) {
        throw "$($t.Name) 比 tests/$($t.BaseName).cpp 旧 —— 这个用例没重新构建，测试结果不可信"
    }
    $testSources += $t
}
$testsCmake = Get-Item (Join-Path $Root 'tests/CMakeLists.txt')
if ($testSources.Count -gt 0) {
    $newestTest = ($testSources | Measure-Object -Property LastWriteTime -Maximum).Maximum
    if ($testsCmake.LastWriteTime -gt $newestTest) {
        throw "tests/CMakeLists.txt 比所有测试可执行都新 —— 测试目标没重新构建"
    }
}
Write-Host "  OK: 0 error；$([IO.Path]::GetFileName($Exe)) 与测试产物都不比自己的输入旧（$($exeTime.ToString('HH:mm:ss'))）" -ForegroundColor Green

# ---- 2b. 平台条件编译门禁 ----
# 本工程只有 Win32 一套实现（src/platform/ 全是 *_win.cpp）：Q_OS_*/Q_WS_* 的非 Windows
# 分支永远编译不到，Windows 分支的条件恒真。平台差异只靠文件名与 platform/ 模块表达，
# 规矩见 docs/coding-style.md 第二节；这里把"重新引入分支"挡在提交前。
$platformHits = @()
foreach ($dir in @('src', 'tests')) {
    $platformHits += Get-ChildItem -Path (Join-Path $Root $dir) -Recurse -File |
        Where-Object { $_.Extension -in '.cpp', '.h' } |
        Select-String -Pattern 'Q_OS_|Q_WS_' -List
}
if ($platformHits.Count -gt 0) {
    $platformHits | Select-Object -First 8 | ForEach-Object {
        Write-Host "  $($_.Path.Substring($Root.Length + 1)): $($_.Line.Trim())" -ForegroundColor Red
    }
    throw "还有 $($platformHits.Count) 个文件含平台条件编译（Q_OS_/Q_WS_），见 docs/coding-style.md 第二节"
}
Write-Host '  OK: src/ 与 tests/ 没有平台条件编译' -ForegroundColor Green

# ---- 2d. 源码编码门禁 ----
# 全仓源码必须是合法 UTF-8，且除了 TAB/换行之外不许出现控制字节。批量改注释的脚本
# 很容易写出这类污染：正则为把替换写成 '\1' 而不是 r'\1' 时，Python 会塞进一个 0x01
# 顶掉缩进——clang-format 照收不误，最后只有 MSVC 报一句看不懂的“未知字符 '0x1'”。
# 唯一例外 src/app/resource.h：它必须以 ANSI 代码页存在，rc.exe 才认（UTF-8 会让图标
# 与版本信息静默丢失，见 docs/code-suspicions.md 第三节）。
$encodingHits = @()
foreach ($dir in @('src', 'tests')) {
    foreach ($f in Get-ChildItem -Path (Join-Path $Root $dir) -Recurse -File |
        Where-Object { $_.Extension -in '.cpp', '.h' -and $_.Name -ne 'resource.h' }) {
        $bytes = [IO.File]::ReadAllBytes($f.FullName)
        $strict = New-Object System.Text.UTF8Encoding($false, $true)
        try {
            $text = $strict.GetString($bytes)
        } catch {
            $encodingHits += "$($f.Name) 不是合法 UTF-8"
            continue
        }
        $m = [regex]::Matches($text, '[\x00-\x08\x0B\x0C\x0E-\x1F]')
        if ($m.Count -gt 0) {
            $encodingHits += ("{0} 含 {1} 个控制字节（首个在第 {2} 字符）" -f
                $f.Name, $m.Count, $m[0].Index)
        }
    }
}
if ($encodingHits.Count -gt 0) {
    $encodingHits | Select-Object -First 8 | ForEach-Object { Write-Host "  $_" -ForegroundColor Red }
    throw "有 $($encodingHits.Count) 个源文件编码不合规"
}
Write-Host '  OK: src/ 与 tests/ 源码全是合法 UTF-8 且无控制字节' -ForegroundColor Green

# ---- 2c. 默认配置与代码注册的一致性 ----
# res/etc/TianBoard.config 随安装包发布，里面每个 Section/Key 都必须有对应的
# `new Setting(this, "Section", "Key", ...)`；反过来代码里的项不必都写进默认配置
# （缺省值在代码里）。历史上攒出过 17 条没人读的键，靠人眼对不完，所以做成门禁。
$configPath = Join-Path $Root 'res/etc/TianBoard.config'
$configKeys = New-Object System.Collections.Generic.List[string]
$section = ''
foreach ($line in Get-Content $configPath) {
    $t = $line.Trim()
    if ($t -match '^\[(.+)\]$') { $section = $Matches[1]; continue }
    if ($t -eq '' -or $t.StartsWith(';') -or $t.StartsWith('#')) { continue }
    if ($t -match '^([^=]+)=') {
        [void]$configKeys.Add("$section/$($Matches[1].Trim())")
    }
}
$settingsSource = Get-Content (Join-Path $Root 'src/settings/Settings.cpp') -Raw
# 两种注册形态都要认：Setting/ColorListSetting 这类成员对象，以及直接走
# value("Section/Key", ...) / setValue("Section/Key", ...) 的标量项。
$codeKeys = New-Object System.Collections.Generic.List[string]
foreach ($m in [regex]::Matches($settingsSource, 'new \w*Setting\(\s*this\s*,\s*"([^"]+)"\s*,\s*"([^"]+)"')) {
    [void]$codeKeys.Add("$($m.Groups[1].Value)/$($m.Groups[2].Value)")
}
foreach ($m in [regex]::Matches($settingsSource, '(?:value|setValue)\(\s*"([^"]+)/([^"]+)"')) {
    [void]$codeKeys.Add("$($m.Groups[1].Value)/$($m.Groups[2].Value)")
}
$orphanKeys = @($configKeys | Where-Object { $codeKeys -notcontains $_ })
if ($orphanKeys.Count -gt 0) {
    $orphanKeys | Select-Object -First 10 | ForEach-Object {
        Write-Host "  配置里有、代码里没有: $_" -ForegroundColor Red
    }
    throw "res/etc/TianBoard.config 里有 $($orphanKeys.Count) 个键没有对应的 Setting 注册"
}
Write-Host "  OK: 默认配置 $($configKeys.Count) 个键全部有代码注册（代码共 $($codeKeys.Count) 个设置项）" -ForegroundColor Green

# ---- 3. 测试 ----
# ctest 是 cmake 的同级程序，不能用 cmake.exe 带 --test-dir 去叫它。
$Ctest = Join-Path (Split-Path -Parent $CMake) 'ctest.exe'
if (-not (Test-Path $Ctest)) { throw "找不到 ctest：$Ctest" }
$ctestConfig = if ($Configuration -eq 'release') { 'RelWithDebInfo' } else { 'Debug' }
Invoke-Step "ctest ($Configuration)" {
    & $Ctest --test-dir $BuildDir -C $ctestConfig --output-on-failure
}

# ---- 4. 排版门禁 ----
if (-not $SkipFormat) {
    Invoke-Step 'format -Check' {
        & powershell -NoProfile -ExecutionPolicy Bypass -F (Join-Path $PSScriptRoot 'format.ps1') -Check
    }
}

Write-Host "`n全部通过：构建 0 error、产物不比输入旧、无平台条件编译、编码合规、配置项全对得上、测试全绿、排版合规。" -ForegroundColor Green
