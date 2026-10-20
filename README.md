# TianBoard
TianBoard是一款在Windows平台基于VS2022、Qt6.9开发的一款开放源码的白板教学软件，主要用于学校和大学的交互式电子白板。
它既可以与交互式白板一起使用，也可以在双屏幕场景中通过笔、平板显示器和光束进行使用。
主界面分四块：演示板（板书与绘图）、网页（内嵌浏览器）、文档（资源库与页面缩略图）、桌面（屏幕批注）。

## 构建

需要 Visual Studio 2022（含桌面 C++ 工具集）和 Qt 6.9 msvc2022_64。用 VS2022 解决方案生成器：

```
cmake -S . -B build -G "Visual Studio 17 2022" -A x64 -DCMAKE_PREFIX_PATH=D:/Qt/6.9.3/msvc2022_64
cmake --build build --config Release
cmake --build build --config Release --target deploy   # windeployqt 拷 Qt 运行时
```

产物在 `build/bin/<配置>/TianBoard.exe`（如 `build/bin/Debug/`、`build/bin/Release/`）——静态库属于
ARCHIVE 输出，若把各配置钉死到同一个 `bin/`，Debug 与 Release 的同名 `.lib` 会互相覆盖，
所以按配置分目录。同目录会铺好 `etc/`、`library/`、`customizations/`、
`Language/`——程序按可执行文件所在目录解析这些资源，因此必须同级。

QuaZip 与 zlib 源码内联在 `third/quazip/`，由 CMake 静态编译，构建时不需要联网。工程不依赖
OpenSSL：原先用的那两处（代理密码的 AES、两个 md5 助手）一个换成 Windows 凭据管理器、一个
根本没有调用点，`third/openssl/` 已整个删除。

## 自测

```
.\scripts\verify.ps1                    # 配置→构建→判错误/新鲜度→查平台分支→ctest→format -Check
.\scripts\verify.ps1 -Configuration release
```

`verify.ps1` 存在的原因是踩过一次真实的假绿：MSBuild `/m` 下某个工程编译失败时其它工程继续，
`TianBoard.exe` 不重新链接，磁盘上还是上一次的旧产物，`ctest` 拿旧 exe 照样全绿。所以它
在跑测试之前先卡三条硬判据：**构建日志里 `error C`/`LNK`/`fatal error` 计数为 0**、
**没有输入比产物新**（`src/` 与根构建文件对 `TianBoard.exe`，每个 `tst_x.cpp` 对各自的
`tst_x.exe`），以及**源码里没有平台条件编译**（`Q_OS_*`/`Q_WS_*`，理由见
[docs/coding-style.md](docs/coding-style.md) 第二节）。手动跑的话两条命令就够：

```
cmake --build build --config Debug --target TianBoard -- /m /v:m
cd build && ctest -C Debug --output-on-failure
```

`tests/` 下是 Qt Test 用例，一共 9 个，`ctest -C Debug` 全部通过。`tst_stringutils`、`tst_geometryutils`、`tst_version` 只链接被测的那几个 `utils/*.cpp`，`tst_credentialstore` 只链接 `platform/CredentialStore_win.cpp`——它们都脱离整个应用栈单独构建、单独运行，这也是把模块依赖拆干净之后才有的好处。`tst_credentialstore` 验的是凭据封装里最容易写错的几处：UTF-16（含 emoji）往返、target 不存在时读回空、覆盖写、只有用户名、两边都空即删除、超过 `CRED_MAX_CREDENTIAL_BLOB_SIZE` 必须在写入前拒绝；它用自己的 PID 当 target 并在收尾删掉，不会碰到真实的代理凭据。`tst_pdfrenderer` 用 QPdfWriter 现场生成一份两页 PDF，验证 `whiteboard/QPdfRenderer` 的契约：页码从 1 起、`pageSizeF` 按 DPI 把点换算成像素、`render` 真的把页面画出来、只画 `exposedRect` 指定的局部、坏文件被判为无效而不是崩溃。`tst_assets` 检查 `res/TianBoard.qrc` 登记的每个文件确实存在、`res/images` 下每个文件都能解码、品牌图形按设计渲染（瓦片外透明、瓦片内是蓝色渐变、T 形笔画是白的），并把 `library/` 与 `images/` 下每个 SVG 真渲染一遍：接管消息处理器捕获 QtSvg 的解析告警（超限的 path 只会打印一句 `Invalid path data; path truncated.` 然后画出半个图形），同时要求画布上确实有像素。`tst_smoke` 读取 `TIANBOARD_EXE`（由 ctest 的 ENVIRONMENT 属性注入）真正拉起 TianBoard.exe，确认它跑完启动链并活过 6 秒、收到关闭请求后能自己退出（实测约 2 秒；20 秒内没走完就重试，连续 3 次都慢才算失败）、且启动日志里没有失效的 SIGNAL/SLOT 连接；应用是单实例的，若新进程把请求转交给已在跑的实例就会自己正常退出，所以它也会换一次时机重试，三次都被接管才 `QSKIP`（不是失败）；崩溃与非零退出码仍然照实变红。它同时校验 `etc/`、`library/`、`customizations/` 与 `etc/TianBoard.config` 确实和可执行文件同级。`tst_ui_modes` 同样拉起 exe，带上 `--ui-smoke` 让应用自己依次切到 文档 / 网页 / 画板 三种主模式再退出：Debug 下任何 Qt 断言都会弹对话框卡住事件循环，进程就不会按时退出——这正是文档模式代理索引错用（启动后点“文档”必现）的回归测试。`tst_language` 加载 `Language/TianBoard_zh_CN.qm`，断言 `MainWindow` 的 “Preferences / Documents / Undo” 确实取到“设置 / 文档 / 撤销”，断言设置对话框的版本号与关于页文案落在 `PreferencesDialog` 上下文里且译文保留 `%1` 占位符，再断言摘掉翻译器后回落到源文本。

## 语言切换

程序启动时按 `-lang <代码>` → 上次保存的选择 → 系统语言 的顺序取词，取不到对应语言包就用英文（源语言）。运行中可在 **设置 → 常规 → 语言** 里切换，选完立即生效并记住，不需要重启。

实现集中在 `core/LanguageManager`：它持有两个 `QTranslator`（应用自身与 Qt 自带的按钮/对话框文案），`setLanguage()` 负责装卸、写回 `App/PreferredLanguage` 并广播 `languageChanged`。界面侧靠 `ui/MainWindowUi`、`PreferencesDialog::Ui` 等手写表单里保留的 `retranslateUi()` 重新取词，因此新增界面时要像它们一样把文案集中到一个 retranslate 函数里，切换才会覆盖到。

语言代码与显示名在 `LanguageManager` 的 `sNativeNames` 表里，新增语言时只要放一份 `res/i18n/lang_<代码>.ts`，`lupdate`/`lrelease` 会自动带上，界面下拉框也会自动出现这一项。

```
# 抽取待翻译文本（新增 tr() 之后跑一次）
D:/Qt/6.9.3/msvc2022_64/bin/lupdate.exe -no-obsolete \
    src/app src/settings src/core src/model src/whiteboard src/content src/api src/web \
    src/desktop src/recording src/ui src/platform src/utils res \
    -ts res/i18n/lang_en.ts res/i18n/lang_zh_CN.ts

# 用 Qt Linguist 打开 lang_zh_CN.ts 填译文，或直接改 <translation> 内容
# 构建时 CMake 会对 res/i18n/*.ts 逐个跑 lrelease，产出 build/Language/TianBoard_<代码>.qm
```

改名（`Recording*`、`WebWidget*`、`Iwb*`）会让 `tr()` 的**上下文名**跟着变，lupdate 只能按
“同文本”启发式搬运少数旧译文，多数会退回未翻译。批量改名后把旧目录留一份，用
`lconvert.exe <新.ts> <旧.ts> -o <合并.ts>`（后一个输入的译文优先）合回去，再逐条补齐
用户看得见的部分。

当前抽取到 477 条源文本，`zh_CN` 已译完 254 条（覆盖主窗口工具栏/菜单动作与提示、设置对话框
各标签页、关于页与版本号、录屏控制器与调色板、IWB 导入导出、下载管理与桌面调色板），其余 223 条自动回落到英文。
`Ctrl+T`、`PgUp` 这类快捷键串是按键名本身，不译。

## 目录结构

```
src/app          可执行目标本体：main.cpp、TianBoard.rc、resource.h
src/settings     设置项本身：Settings 注册表 + Setting<T> 模板
src/core         应用与各类服务/控制器（Application、DisplayManager、
                 LanguageManager、PersistenceManager、DocumentManager…）
src/model        文档域：DocumentProxy、DocumentContainer、DocumentController
src/whiteboard   画板：BoardView/BoardController + 图形场景、图形项、绘图工具与
                 PDF 页图形项/渲染器
src/content      文档格式读写与导入导出：.tbz/.tbx、IWB 子集与转换、SVG/RDF 元数据、
                 缩略图数据块、PDF 导入导出、图片导入、Web 导出
src/api          小部件与文档对外 API 门面（WebWidget*API）
src/web          内嵌浏览器与网页控制器
src/desktop      屏幕批注（平台无关部分）
src/recording    录屏：画面与声音采集、录制调色板、Windows Media 编码器
src/ui           全部界面控件与调色板
src/platform     平台相关实现（不带界面的部分）：PlatformUtils*、WindowCapture*、
                 KeyboardSender*、CredentialStore*
src/utils        纯工具：StringUtils、GeometryUtils、Version、FileSystemUtils、Enums.h…
third/           内联的 quazip/zlib 与 qtsingleapplication
res/             images / etc / library / customizations / style / i18n(.ts) / qrc
tests/           Qt Test 用例（9 个）
docs/            架构说明、编码规范、疑点台账
packaging/       Inno Setup 脚本与打包脚本
scripts/         build.ps1 / format.ps1 / verify.ps1
```

每个模块的 `CMakeLists.txt` 显式列出全部源文件（不用 GLOB），新增文件不改清单就不进构建。
`src/utils`（出边为零）、`src/settings` 与 `src/platform`（都只依赖 utils）已经从 `OBJECT`
收紧成真 `STATIC` 库；其余模块因为还有 11 组双向 include 而暂时保持 `OBJECT`，拆环顺序见
[docs/architecture.md](docs/architecture.md) 第四、五节。

代码风格由根目录 `.clang-format` 兜住：全仓（`src/` + `tests/`，344 个文件）已用
clang-format 22.1.8 排过一次，日常提交前跑 `.\scripts\format.ps1 -Check`（0 才是干净）。
`third/` 永久排除，格式化结果绑定 clang-format 版本，升版本要单独一次全量提交。
