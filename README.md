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
.\scripts\verify.ps1                    # 配置→构建→判错误/新鲜度→查平台分支→查编码→查配置项→ctest→format -Check
.\scripts\verify.ps1 -Configuration release
```

`verify.ps1` 存在的原因是踩过一次真实的假绿：MSBuild `/m` 下某个工程编译失败时其它工程继续，
`TianBoard.exe` 不重新链接，磁盘上还是上一次的旧产物，`ctest` 拿旧 exe 照样全绿。所以它
在跑测试之前先卡五条硬判据：**构建日志里 `error C`/`LNK`/`fatal error` 计数为 0**、
**没有输入比产物新**（`src/` 与 `res/` 里的编译输入对 `TianBoard.exe`，每个 `tst_x.cpp` 对各自的
`tst_x.exe`，`res/i18n/*.ts` 则对摆在可执行文件同级的 `Language/TianBoard_<语言>.qm`——`.ts` 不进
exe，改完不重链接是正常的，但语言包必须已经铺过去）、**源码里没有平台条件编译**（`Q_OS_*`/`Q_WS_*`，理由见
[docs/coding-style.md](docs/coding-style.md) 第二节）、**源码全是合法 UTF-8 且除 TAB/换行外
没有控制字节**（批量改注释的脚本能把缩进写成 `0x01`，clang-format 照收，最后只剩 MSVC 一句
看不懂的"未知字符 '0x1'"；`src/app/resource.h` 按 ANSI 给 `rc.exe` 读，是唯一豁免），
以及 **`res/etc/TianBoard.config` 里每个 `Section/Key` 都能在代码里找到注册**。手动跑的话两条命令就够：

```
cmake --build build --config Debug --target TianBoard -- /m /v:m
cd build && ctest -C Debug --output-on-failure
```

编译告警另有一个脚本看：`.\scripts\warnings.ps1` 在独立的 `build-w4/` 里以 MSVC `/W4`
全量编一遍，只统计 `src/` 与 `tests/` 的告警并按编号归类（`-Strict` 有一条就非零退出）。
不把 `/W4` 开进日常构建是刻意的：门禁要的是"编得过 + 测试绿"，把告警当错误会让一次
Qt 小升级就整仓编不过。当前基线是**自有代码零告警**——这轮就是靠它揪出未初始化变量被当
除数、`fill-on-light-background` 解析结果被同名局部吞掉、以及 59 处 Qt 已废弃 API。

`tests/` 下是 Qt Test 用例，一共 16 个，`ctest -C Debug` 全部通过。`tst_stringutils`、`tst_snapping`、`tst_geometryutils`、`tst_version` 只链接被测的那几个 `utils/*.cpp`，`tst_credentialstore` 只链接 `platform/CredentialStore_win.cpp`——它们都脱离整个应用栈单独构建、单独运行，这也是把模块依赖拆干净之后才有的好处。`tst_credentialstore` 验的是凭据封装里最容易写错的几处：UTF-16（含 emoji）往返、target 不存在时读回空、覆盖写、只有用户名、两边都空即删除、超过 `CRED_MAX_CREDENTIAL_BLOB_SIZE` 必须在写入前拒绝；它用自己的 PID 当 target 并在收尾删掉，不会碰到真实的代理凭据。`tst_pdfrenderer` 用 QPdfWriter 现场生成一份两页 PDF，验证 `whiteboard/QPdfRenderer` 的契约：页码从 1 起、`pageSizeF` 按 DPI 把点换算成像素、`render` 真的把页面画出来、只画 `exposedRect` 指定的局部、坏文件被判为无效而不是崩溃。`tst_assets` 检查 `res/TianBoard.qrc` 登记的每个文件确实存在、`res/images` 下每个文件都能解码、品牌图形按设计渲染（瓦片外透明、瓦片内是蓝色渐变、T 形笔画是白的），并把 `library/` 与 `images/` 下每个 SVG 真渲染一遍：接管消息处理器捕获 QtSvg 的解析告警（超限的 path 只会打印一句 `Invalid path data; path truncated.` 然后画出半个图形），同时要求画布上确实有像素。`tst_smoke` 读取 `TIANBOARD_EXE`（由 ctest 的 ENVIRONMENT 属性注入）真正拉起 TianBoard.exe，确认它跑完启动链并活过 6 秒、收到关闭请求后能自己退出（实测约 2 秒；20 秒内没走完就重试，连续 3 次都慢才算失败）、且启动日志里没有失效的 SIGNAL/SLOT 连接；应用是单实例的，若新进程把请求转交给已在跑的实例就会自己正常退出，所以它也会换一次时机重试，三次都被接管才 `QSKIP`（不是失败）；崩溃与非零退出码仍然照实变红。它同时校验 `etc/`、`library/`、`customizations/` 与 `etc/TianBoard.config` 确实和可执行文件同级。`tst_ui_modes` 同样拉起 exe，带上 `--ui-smoke` 让应用自己依次切到 文档 / 网页 / 画板 / 桌面批注 四种主模式再退出：Debug 下任何 Qt 断言都会弹对话框卡住事件循环，进程就不会按时退出——这正是文档模式代理索引错用（启动后点“文档”必现）的回归测试；桌面批注那一腿还量得到装配点装的批注层工厂有没有真装上（core 现在只认识 `IDesktopAnnotation` 契约）。`tst_format` 带上 `--format-selftest`，让应用在初始化完成后真存一页、再绕开场景缓存把盘上的字节真读回来逐项比对：一笔三段笔迹（几何、颜色、深浅底两套取色、笔宽）、一段文本、一张位图（按像素指纹比，验的是图真进了对象目录而不是只剩引用）、尺子/量角器/圆规/坐标轴四件工具图元（坐标轴还带着刻度数字显隐这个状态位），以及页面背景类型与网格尺寸，还要验顶层图元的叠放次序；随后把同一页原样再存读一轮，要求第二轮结果与第一轮完全相同——累积型误差（每存开一次就挪一点、胖一点）只看一轮常常看不出来。最后把这份文档导出成 `.iwb` 再由导入器读回一份新文档，断言文本、图片与笔迹颜色都在、图片不是空位图（这段查的是 IWB 互操作链路 `IwbDocumentAdaptor`+`IwbSubsetAdaptor`，工具图元不在 IWB 规范内，所以不比它们）。存盘与读回是界面上看不见的一条链路，属性名、命名空间或取整精度任何一侧动一下都只能靠这次往返发现；判据取退出码（非 0 即不一致的条数，75 表示已有实例在跑、按约定让路并跳过）。`tst_snap` 带上 `--snap-selftest`，查的是吸附的另一半：格距、页面背景类型与图元外框是怎么被喂进吸附数学的（空白底两轴都吸、横线稿纸只吸 Y、多角点取吸得力最强的那个、带画笔的图形先按笔宽缩回Qt 本就算进包围盒的那一半），这些要跑起整个应用才看得到，纯几何那条用例覆盖不到。`tst_shortcuts` 带上 `--shortcut-selftest`，先查装配：主窗口的动作是不是都登记进了 `core/ShortcutManager`、默认快捷键是不是还在原来的位置、出厂默认之间有没有两条抢同一个组合键；再换一张自建注册表跑完整回路：改绑能不能落盘、模拟重启后能不能读回来（含“明确不绑键”这种空值不能偷偷变回默认）、恢复默认有没有把配置键真删掉，最后把 `ui/ShortcutSettingsWidget` 拉起来，用选中行、点“应用/清除/恢复所选”这条路验一遍界面上的接线。`tst_cookierules` 只链接 `utils/CookieRules.cpp`，测白名单匹配的两种写法（带点覆盖子域与顶级域、不带点只整串相等）与策略名映射；`tst_cookies` 带 `--cookie-selftest`，查网络管理器的 Cookie 罐是不是可枚举的 `core/CookieJar`、三项隐私设置按名字读不读得到，再往罐里种三条已知 Cookie，验证“退出会清掉几个”这个计数真的按白名单把顶级域与子域留下、只算不匹配的那一条（把带点规则退回“只比后缀”，这条断言立刻变红）。`tst_import` 带 `--import-selftest`，用一个桩裁决器替用户答题，把导入时文档名撞车的三条路径变成可自动执行的案子：不撞名一次都不该问用户、点取消的那一份必须整包退回磁盘外（旧实现既不登记也不释放，磁盘上留一包没人认领的目录、下次启动还会自己冒出来）、改名两份共存且新名字写回盘上的元数据、同名替换则旧的删干净且画板跟到新文档；另外两案查 .tbx 整批导入那一腿和“没装裁决钩子时不登记也不崩”，最后一案断言删掉当前文档那一行后模型的当前节点真的归零。`tst_language` 加载 `Language/TianBoard_zh_CN.qm`，断言 `MainWindow` 的 “Preferences / Documents / Undo” 确实取到“设置 / 文档 / 撤销”，断言设置对话框的版本号与关于页文案落在 `PreferencesDialog` 上下文里且译文保留 `%1` 占位符，再断言摘掉翻译器后回落到源文本；它还断言 Qt 自带的 `qtbase_zh_CN.qm` 已随包部署且真能取到词
——消息框按钮、颜色对话框这些文案不在本项目的 `.ts` 里，漏了部署只有运行时一句告警，肉眼看不见。

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

当前抽取到 484 条源文本，`zh_CN` 已译完 481 条（主窗口工具栏/菜单动作与提示、内置工具名、文档树与
文档库、页面缩略图、导出导入与 IWB、下载列表与进度、代理与网络提示、遮罩/小部件/文本委托的右键菜单、
笔属性、标签页右键菜单、缩略图生成提示、缩放倍率、快捷键页等），剩下 3 条是**刻意不译**：2 条会被当
文件名前缀写进盘里的词（`CapturedImage`、`ImportedImage`）、以及小部件 API 的一条调试日志。原先那 21 条
`Ctrl+T`、`PgUp` 已经整个退出抽取范围——它们是按键名而不是自然语言，译了会改按键绑定的解析，
现在它们以数据的形式待在 `MainWindowUi::setupUi`（见“快捷键自定义”一节）。翻译目录里曾有 17 条带着"搬移时留下的旧译文"
（原文已换、`type="unfinished"` 还挂着，内容对不上句子的中文建议），这次按上下文与原文逐条核对后覆盖掉，
并把两条 `numerus="yes"` 的消息补成 `<numerusform>` 形式，否则 `lrelease` 直接报解析错。

## 快捷键自定义

设置对话框的 **快捷键** 页列出全部可绑命令（按 常用 / 演示板 / 工具 / 线条与颜色 / 背景 / 网页 / 文档
分组），选中一条、在下面按出新组合键、点“应用”即改；“清除”是明确不绑键，“恢复所选/全部恢复默认”
把命令交回出厂值。改动写进用户配置的 `Shortcut/<动作名>`，重启后由 `core/ShortcutManager` 在注册时
读回来；同一个组合键只允许一条命令占用，撞键当场拒绝并在页面说明是谁占的，因此改绑不会静默失效。

默认值只在 `MainWindowUi::setupUi` 里写一份，**不进 `retranslateUi`**：后者每次切换语言都会重跑，
把快捷键当文案搬进去会连带覆盖掉用户自己的改绑（移植时正是这样，`Ctrl+T` 这类串还因此混进了翻译目录）。
`Application::exec()` 里也不再事后 `setShortcuts(QKeySequence::Copy)` 之类的补一刀——那三处同样已并入
`setupUi`，注册发生在主窗口构造的最后一刻，捕获到的就是真实默认值。

规矩是一条：**默认值只有 `setupUi` 一处，覆盖只有 `ShortcutManager` 一处**。装配后期再补一句
`setShortcut` 就等于偷偷改掉用户可能已经自定义的键，`tst_shortcuts` 抽查几条默认值仍在原位，
就是为了钉住这条线。

## 浏览器隐私

内嵌浏览器的 Cookie 有三档策略（设置 → 网络 → **Cookie 隐私**）：接受所有 / 只阻止第三方（默认）/ 全部阻止，
选完立刻生效不用重启——策略装在 `QWebEngineCookieStore` 的过滤器上，由 `web/WebController` 在装配时与每次改动后重装。
“退出 TianBoard 时清除 Cookie”打开后，关程序时按域名白名单清理：白名单留空就整罐清空，写了域名则逐条判断，
以 `.` 开头的条目覆盖该域及其子域（`.example.org` 也保住 `example.org` 本身，浏览器写进盘里的顶级域没有那个点），
不写点就要求整串相等。规则本身（`utils/CookieRules`）是纯字符串运算，脱离浏览器单独测。

实现上有一个绕不开的点：`QWebEngineCookieStore` 不提供“列出当前所有 Cookie”，要按域逐条删就只能自己有一份清单。
做法是给网络管理器装 `core/CookieJar`（`QNetworkCookieJar` 的子类，只多做一件事：把 protected 的 `allCookies()` 公开出来），
把内核的 `cookieAdded`/`cookieRemoved` 镜像进去，并在装过滤器时 `loadAllCookies()` 让盘上的历史 Cookie 也尽量过一遍镜像——
顺带的效果是应用自己发出的网络请求与内嵌浏览器共用同一份会话。浏览器进程要到第一个页面才真起来，
这次枚举不保证拿全，所以设置页那句“退出时将清除 N 个”按“至少 N 个”理解；白名单空着时走的是
`deleteAllCookies()` 整罐清空，不受这个限制。上游那份是后缀匹配只比 `endsWith`，会把白名单里
顶级域自己的 Cookie 也当成“不匹配”删掉，这里的规则改过并配了负向对照。

## 目录结构

```
src/app          可执行目标本体：main.cpp、TianBoard.rc、resource.h
src/settings     设置项本身：Settings 注册表 + Setting<T> 模板
src/core         应用与各类服务/控制器（Application、DisplayManager、
                 LanguageManager、PersistenceManager、DocumentManager…）
src/model        文档域：DocumentProxy、DocumentContainer、DocumentTreeModel、
                 SortFilterProxyModel、Resources（工具光标与自定义字体清单）；文档模式的
                 界面装配 DocumentController 在 src/ui
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
tests/           Qt Test 用例（16 个）
docs/            架构说明、编码规范、疑点台账
packaging/       Inno Setup 脚本与打包脚本
scripts/         build.ps1 / format.ps1 / verify.ps1
```

每个模块的 `CMakeLists.txt` 显式列出全部源文件（不用 GLOB），新增文件不改清单就不进构建。
`src/utils`（出边为零）、`src/settings` 与 `src/platform`（都只依赖 utils）、`src/recording`、
`src/desktop` 与 `src/web`（都没有任何模块反向 include 它们）已经从 `OBJECT` 收紧成真
`STATIC` 库；其余模块因为还有 5 组双向 include 而暂时保持 `OBJECT`，拆环顺序见
[docs/architecture.md](docs/architecture.md) 第四、五节。

代码风格由根目录 `.clang-format` 兜住：全仓（`src/` + `tests/`，387 个文件）已用
clang-format 22.1.8 排过一次，日常提交前跑 `.\scripts\format.ps1 -Check`（0 才是干净）。
`third/` 与 `src/app/resource.h` 永久排除（后者是 ANSI 代码页给 `rc.exe` 读的符号表，
重排或转码会让 `.res` 静默出错），格式化结果绑定 clang-format 版本，升版本要单独一次全量提交。

注释规范见 [docs/coding-style.md](docs/coding-style.md) 第六节：`.h` 与没有对应头文件的 `.cpp`
开头是 `@file` + `@brief`，类型与接口用 Doxygen（中文描述、英文命令），实现文件不重复头文件
的文档、只写关键步骤；被注释掉的旧代码一律不留。
