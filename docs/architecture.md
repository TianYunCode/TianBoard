# TianBoard 架构说明

配套文档：[编码规范](coding-style.md) · [疑点台账](code-suspicions.md) · [根 README](../README.md)

本文说明三件事：源码分成哪些模块、构建为什么是现在这个样子、以及**已知的依赖环**
（按什么顺序拆）。模块划分是 2026-10 的结构调整结果，只做目录搬迁与 include 前缀
改写，没有改任何行为。

---

## 一、源码结构

```text
src/
  ├─ app/          可执行目标本体：main.cpp、TianBoard.rc、resource.h、启动装配、
  │                十三个 *SelfTest（外加 SelfTestAssets 那份共用合成素材），以及
  │                Application.cpp / ApplicationController.cpp
  │                ——这两个装配角色的实现（头文件在 core/，见第四节）
  ├─ settings/     设置项本身：Settings（全部 Setting 的注册表）+ Setting<T> 模板
  │                （只依赖 utils，与 utils 一样是真 STATIC）
  ├─ core/         各类服务/控制器（DisplayManager、LanguageManager、PersistenceManager、
  │                DocumentManager、SceneCache、DownloadManager、NetworkAccessManager +
  │                CookieJar（可枚举的 Cookie 镜像罐）、
  │                ShortcutManager（快捷键注册表与偏好设置那一页的模型）…）；
  │                Application 与 ApplicationController 只有头在这里，对 ui 的出边已归零
  ├─ model/        文档域：DocumentProxy、DocumentContainer、DocumentTreeModel
  │                （树节点 + 目录树模型 + 内部拖放载荷）、SortFilterProxyModel、
  │                ForeignObjectsHandler（文档包内被引用对象的搬运，从 core/ 搬来）、
  │                IDocumentStore（仓库契约：实现是 core 的 PersistenceManager）、
  │                MimeData（拖放用的"哪份文档第几页"）、MimeTypes.hpp、
  │                PackageLayout.hpp（.tbz 包内的目录名与 folders.xml 词表）、
  │                Resources（工具光标 + 自定义字体清单，从 ui/ 搬来）。
  │                出边只剩 settings/utils，2026-10-22 起是真叶子、真 STATIC（原 document/）
  ├─ whiteboard/   画板：BoardView/BoardController/DrawingController + 图形场景与全部
  │                图形项 + 绘图工具（原 board/ + domain/ + tools/）+ PDF 页图形项与
  │                渲染器（PDFRenderer/QPdfRenderer/PdfPageItem，从原 pdf/ 搬来）+ ToolWidget 与
  │                Magnifier（从 ui/ 搬来：它们只认画板的类型，画板又要造它们）+ MimeDataGraphicsItem
  │                （携带图形项副本的剪贴板载荷；它要能释放副本，所以不能在 model）
  ├─ content/      文档格式读写与导入导出：.tbz/.tbx 与 IWB 子集（IwbDocumentAdaptor /
  │                IwbSubsetAdaptor / ExportIwb / ImportIwb）、SVG/RDF 元数据、缩略图数据、
  │                PDF 导入导出（原 adaptors/；pdf/ 的图形项三家已归 whiteboard/）。
  │                对 core 零出边（走 model/IDocumentStore），真 STATIC
  ├─ api/          小部件与文档对外 API 门面（WebWidgetBoardAPI/WebWidgetMessageAPI）
  ├─ web/          内嵌浏览器与网页控制器（原 web/ + web/browser/）
  ├─ desktop/      屏幕批注（平台无关部分）
  ├─ recording/    录屏：画面与声音采集、录制调色板、Windows Media 编码器（没有任何模块
  │                反向依赖它，真 STATIC）
  ├─ ui/           全部界面控件与调色板（原 ui/ + widgets/ + network/ProxyDialog，
  │                加上从 platform/ 搬来的 KeyboardPalette* 与 CustomCaptureWindow*，
  │                以及从 model/ 搬来的 DocumentController——文档模式的界面装配；Resources
  │                搬去了 model/，ToolWidget 与 Magnifier 搬去了 whiteboard/）
  ├─ platform/     平台相关实现（不带界面的部分）：PlatformUtils*、WindowCapture*、
  │                KeyboardSender*、CredentialStore*、VirtualDesktop（出边只有 utils）
  └─ utils/        纯工具：StringUtils、GeometryUtils、Version、FileSystemUtils（含
                   applicationResourcesDirectory）、Snapping、CookieRules、TextTools、
                   AutoSaver、Globals、UserNotices、Enums.h（出边为零，最底一层；TextTools、
                   AutoSaver 与 UserNotices 2026-10-21/22 从 core/ 搬来，搬完出边仍然是零）
third/             内联第三方：quazip+zlib、qtsingleapplication
res/               images / etc / library / customizations / style / i18n(.ts) / qrc
tests/             Qt Test 用例（25 个）
packaging/         Inno Setup 脚本与打包脚本
scripts/           build.ps1 / format.ps1 / verify.ps1
docs/              本文与编码规范、疑点台账
```

搬迁用 `git mv`，历史可追：`git log --follow -- src/ui/MainWindow.cpp`。

三个目录的归属需要说明（原布局里没有对应物）：

| 原目录 | 去了哪里 | 理由 |
|---|---|---|
| `network/` | AutoSaver / HttpGet / NetworkAccessManager → `core/`；ProxyDialog → `ui/` | 前三个是服务，最后一个是对话框；没有单列 network 模块 |
| `pdf/` | `content/` | PDFRenderer/QPdfRenderer 是文档格式读取，PdfPageItem 只在导出与 PDF 图形项里出现 |
| `tools/` | `whiteboard/` | 尺规、量角器、窗帘都是画板工具，与 domain/ 的图形项同属画板热路径 |

`core/WB.h` → `utils/Enums.h`（纯全局枚举与 typedef，和 Globals.h 同类）；
`core/resource.h` → `app/`（只被 TianBoard.rc 用）；
`utils/CoreGraphicsScene.*` → `whiteboard/`（它是 `GraphicsScene` 的基类，本属画板）。

`core/Settings.h`、`core/Setting.h` → `settings/`（2026-10 的第二次拆分）。原来每个模块
为了读一个开关都要 include `core/Settings.h`，而 core 是装配点，这一发 include 顺带把
`core/Application.h` 的整棵依赖拖进调用方。拆出来之后它只剩两条向上的边，也都断掉了：
`applicationResourcesDirectory()` 是部署约定而不是平台能力，下沉到
`utils/FileSystemUtils`（platform 里那份一并删掉，全仓调用点跟着改名）；路径设置里的
`{UserLoginName}` 改由 `core/Application` 在启动时 `Settings::setUserLoginName()` 推进来。
于是 settings 的出边只有 utils，成为第二个真 STATIC 模块，13 个模块里有 11 个可以只依赖
它而不碰 core（不依赖它的只有 utils 和 settings 本身）。

## 二、运行结构

单进程单可执行，**没有 DLL 插件、没有运行时插件扫描**。

```text
TianBoard.exe
  ├─ main.cpp：消息处理器、单实例转发（qtsingleapplication）、Q_INIT_RESOURCE
  ├─ Application        QApplication 子类，持有主窗口
  ├─ ApplicationController  模式切换（白板 / 网页 / 文档 / 桌面 / 录音）与各控制器装配
  └─ 四大界面：BoardView(画板) · WebController(内嵌浏览器) · DocumentController(文档库) ·
              DesktopAnnotationController(桌面批注)
```

运行期目录约定：`FileSystemUtils::applicationResourcesDirectory()` 返回可执行文件所在
目录，因此 `etc/`、`library/`、`customizations/`、`Language/` 必须与 exe 同级，由
`src/app/CMakeLists.txt` 的 POST_BUILD 步骤铺好；`tst_smoke` 会显式校验这一点。

代理的用户名/密码不进 INI，走 Windows 凭据管理器（`src/platform/CredentialStore_win.cpp`，
generic 凭据，target 名 `TianBoard/HttpProxy`）。以前它们是 `Vault/*` 两段密文，密钥和
salt 写死在源码里——任何拿到二进制的人都能解，等于没加密，还为此在仓库里养了 72MB 的
OpenSSL。换掉之后工程不再依赖 OpenSSL，`etc/TianBoard.config` 里也没有代码认得的键之外
的残留（脚本核对过每一条 `Section/Key`）。

## 三、构建形态

* **每个模块一个 `src/<module>/CMakeLists.txt`，逐个文件显式列出**。没有
  `file(GLOB_RECURSE)`：往树上丢一个 .cpp 而它没进构建，是这类遗留工程最难查的故障；
  现在新增文件必须改 CMakeLists，配置期就会暴露漏列。
* **模块 = `add_library(tb_<module> OBJECT ...)`**，`src/app/CMakeLists.txt` 把它们全部
  `target_link_libraries(TianBoard PRIVATE ...)` 并进一个可执行。
* **include 规矩**：跨模块一律 `"模块/文件.h"`（`#include "core/Application.h"`），
  同目录也这么写（`#include "MainWindow.h"` 不允许）。每条 `target_include_directories`
  只给 `${CMAKE_SOURCE_DIR}/src`；`CMAKE_INCLUDE_CURRENT_DIR` 显式关闭，所以裸写
  直接编译失败，规矩由编译器兜住而不是靠人自觉。唯一的额外一条：`tb_sdk` 的接口上带
  仓库根，因为 `core/Application.h` 引用 `"third/singleapp/qtsingleapplication.h"`，而
  `Application.h` 被 core 之外的 10 个模块直接 include；内联第三方按它原本的仓库根
  相对路径引用，
  `third/` 不动（见根 `CMakeLists.txt` 里的注释），`src/` 的前缀规矩不受影响——仓库根下
  已经没有模块目录了。
* **产物按 binary 目录 + 配置分开放**：`${CMAKE_BINARY_DIR}/bin/<Config>/`。
  一处定义（根 `CMakeLists.txt` 的 `TIANBOARD_BIN_DIR`），POST_BUILD 铺资源、windeployqt 的
  `deploy` 目标、`TIANBOARD_EXE`（tst_smoke 定位应用）与打包脚本全部经
  `$<TARGET_FILE_DIR:TianBoard>` 跟着走。
  为什么不钉死到单一目录：静态库是 ARCHIVE 输出，多配置生成器下 Debug 与 Release 的
  `tb_quazip.lib` 同名，钉死就会互相覆盖 —— 实测踩过：先构建 Release 再构建 Debug，
  Debug 链接到 Release 的 `.obj`，报 LNK1319 / LNK2038 RuntimeLibrary 不匹配。
  同理，路径必须相对 `CMAKE_BINARY_DIR` 而不是 `CMAKE_SOURCE_DIR`，否则
  `build-debug/` 与 `build-release/` 两个隔离目录会共用同一处互相污染。
* 单配置默认 `RelWithDebInfo`；MSVC 的纯 Release 也补 `/Zi` + `/DEBUG`，现场崩溃要能
  对着 pdb 看栈。`/utf-8`（源码里有中文注释）与 `/MP` 全局给。
* `CMakePresets.json`：`vs2022-debug` / `vs2022-release`（VS 生成器，中间产物 `build/`，
  IDE 与现有工具链用这对）+ `debug` / `release`（Ninja，中间产物 `build-debug` /
  `build-release`，需要 VsDevCmd 环境，用 `scripts/build.ps1`）。两对都已实测跑通：
  VS 那对是日常构建路径，Ninja 那对要在开发者命令提示符里跑（cl/link 不在普通 shell 的
  PATH 上，`scripts/build.ps1` 内部就是先 `call VsDevCmd.bat` 再 `cmake --preset`）。
  产物按生成器各归各的二进制目录：`build/bin/<配置>/` 与 `build-debug/bin/Debug/`。

## 四、为什么现在仍是 OBJECT 库而不是 STATIC 库

目标是每个模块一个真正的静态库、依赖无环、链接顺序明确。**这一步整体还做不到**：
代码里存在 2 组双向 include（下一节全表 + 台账）。STATIC 库遇到环需要 `--start-group`
之类的手段，等于把问题藏进链接参数里；OBJECT 库把所有 .obj 并进同一个可执行，环在
链接期不成问题，于是本次调整可以只拿到两样确定性的东西——**每模块的显式文件清单**
和**每模块的 include 白名单**——把拆环留到后续阶段。

已经做到的九个：`tb_utils`（出边为零）、`tb_settings`（出边只有 utils）、`tb_platform`
（出边只有 utils）、`tb_model`（出边只有 settings 与 utils，2026-10-22 断掉对 core 的 9 条
上边之后成为真叶子）与 `tb_recording`、`tb_desktop`、`tb_web`、`tb_api`、`tb_content`
（没有任何模块反向 include 它们）。越界的 `#include` 在编译期就失败，而不是等到链接期靠符号缺失
才发现——这个配方对其它模块同样成立，`settings` 与 `platform` 都是照着 `utils` 拆出来的。

换成 STATIC 的前置条件有两种，强度不同：

* **叶子模块**要求出边全部落在比它低的层（下表），且没有对上的 include——这是终态。
* **非叶子模块**只要"没有任何上层反向 include 它"（也就是不在任何环里）就可以先归档：
  它向上那几条边的符号在最终可执行里照样解得开，因为上层此刻还是 OBJECT、.obj 直接进
  exe；`tb_recording`、`tb_desktop`、`tb_web`、`tb_api`、`tb_content` 就是这样先走了一步，
  它们仍认 core/ui/whiteboard。等那些模块也
  逐个 STATIC 时，再给它们补 `target_link_libraries` 声明，MSVC 的链接器对静态库顺序不
  敏感（反复扫描直到符号解完），不会变成一门链接参数的玄学。

## 五、依赖债：现存的双向环

下面是 `src/` 全量扫描出来的直接互相 include（数字 = 各方向的 include 处数，
含头文件里的；`app/` 作为装配点不计）。2026-10-21 又删掉 107 行确认无害的跨模块
include：候选 286 处，其余 179 处被编译器证明仍在替调用方供给类型——判死只能靠编译，
grep 筛出来的多数是误报（`utils/Enums.h` 被非限定枚举名用、`Application::mainWindow->ui()`
这类用法里类型名一次都不出现）。同一轮把 `VirtualDesktop`（虚拟桌面枚举与窗口归属，
只 include 自己的头加 Win32/Qt）从 core/ 搬去 platform/，19 处跨模块引用因此从 →core
挪到 →platform——这些模块对 platform 的边本来就存在，一条新环都没造出来。

| 环 | 边数 | 典型出处 |
|---|---|---|
| `core` ↔ `whiteboard` | 2 / 22 | `core/PersistenceManager`/`DocumentManager`/`SceneCache` 认图形项与画板场景；`whiteboard/*.cpp` 认 Application 剩下的全局指针。挂在 `Application` 上的两个全局画板物件已经还回去了：`boardController`→`BoardController::instance()`（一刀削掉 21 条上边）、`undoStack`→`BoardController::undoStack()`（再削 3 条）；core→whiteboard 这侧 16→14 删掉没人实例化的 `core/IdleTimer`（第 13 步），14→10 把 `DisplayManager`/`DownloadManager` 改成"只报事实"（第 14 步），10→7 是 `DocumentManager` 里三条"其实不存在"的 include（第 15 步），7→3 把页复制时的图元资源搬移搬回 content 的写出器（第 16 步），3→2 把"追加完请画板补缩略图"换成装配点装上的处理器（第 17 步）。剩下的 2 条都是 `SceneCache`（h/cpp 各一条：缓存并造出 GraphicsScene、存取它的视图状态）；`whiteboard` 那侧的主因仍是 `mainWindow` 与 `documentController` |
| `ui` ↔ `whiteboard` | 29 / 30 | 调色板与画板互相认（ToolWidget/Magnifier 搬去画板、Resources 搬去 model，又删掉一批真正没人用的跨模块 include 之后） |

`content` ↔ `core` 与 `core` ↔ `model` 两组环已于 2026-10-22 拆掉，手法都是同一个形状：
**契约住在被依赖的那一层，实现由高层提供，装配点（或实现自己的构造函数）把它装上**。
前者抽成 `model/IDocumentStore`（建文档、取场景、搬对象、页面操作、树模型），后者让模型也
改走同一份契约，另外把两个本来就不该挂在 `Application` 上的东西搬走：`.tbz` 目录名与 MIME
类型名进 `model`，静态对象宿主进 `utils/StaticMemory`。`MimeData` 那个文件里其实混着两层
载荷，按载荷类型拆到 `model` 与 `whiteboard` 各一半。

`.tbz` 的磁盘布局常量 2026-10-22 从 `PersistenceManager` 的静态成员搬进
`model/PackageLayout.hpp`（见第一节 model 那一行）。这一刀把**向上**的边削掉四条——三个画板
图形项文件与 `content/ImportPDF.cpp` 不再为了一个目录名 include 持久化服务（whiteboard → core
51→48、content → core 17→16），代价是**向下**多了几条合法的边（whiteboard → model 13→18、
content → model 21→24、core → model 6→8）。环的数量不变，但每一组环里"够不着的那一边"更短了。
新增的 `tst_packagelayout` 钉住这些字面量：往返自检存读用的是同一份常量，改名字对它自洽，
只有钉在字面量上才会变红（实测：把 `images` 改成 `image`，`tst_format` 全绿、`tst_packagelayout` 红）。

`core` ↔ `ui` 已于 2026-10-21 归零（见下面第 9 步的收尾）：`Application.cpp` 与
`ApplicationController.cpp` 两个实现搬到 `app/`，头文件仍留在 `core/`（每个模块都要从那里
拿入口指针），于是 `src/core/` 里没有任何文件认识 `ui/`。`ui` 因此不再是环的一员，但它仍被
`desktop`/`recording`/`web`/`whiteboard` 反向 include，还不能归档。

`utils` 的两条上边（`CryptoUtils.h`/`FileSystemUtils.cpp` → `core/Application.h`，以及
`CoreGraphicsScene.cpp` → 两个图形项）与 `settings` 的那条（`Settings.cpp` →
`platform/PlatformUtils.h` 拿资源目录，另加 `osUserLoginName`）都已断掉，这两个模块
都不在任何环上，也已经收紧成 STATIC。

`platform` 是分两步断干净的：

- `WindowCapture_win.cpp` 里那个 `whiteboard/BoardView.h` 从来没用过；抓窗口时独占鼠标
  键盘的需求改成构造时收一个 `QWidget*`（`WindowCapture(QWidget *grabTarget, ...)`），
  于是它不再认识 `desktop/DesktopPalette` 与 `DesktopAnnotationController`。
本工程只有 Win32 一个平台：`src/` 与 `tests/` 里已经没有任何 `Q_OS_*` 条件编译，
平台差异只靠"文件名叫 `*_win.cpp` + 住在 `platform/` 模块"表达（规矩见
`docs/coding-style.md` 第二节）。
- 屏幕键盘 `KeyboardPalette*` 与框选取图的 `CustomCaptureWindow*` 搬去 `ui/`——它们本来
  就是界面：继承 `ui/ActionPalette`、要 `ui/MainWindow` 定位、要 `ui/RubberBand` 画框，
  还认 `core/Application` 与 `core/VirtualDesktop`（后者 2026-10-21 也已搬到 `platform/`）。
  平台层只剩下不带界面的那件事：
  一次按键怎么交给系统（`KeyboardSender`，`sendUnicodeKey`/`sendVirtualKey` 两个函数，
  Win32 `SendInput` 的实现）。搬完 `platform` 出边只剩 utils，`tb_platform` 因此也收紧成
  STATIC，`platform` ↔ `core` 与 `platform` ↔ `ui` 两组环一起消失。
  顺带删掉三个只在 `KeyboardPalette_win.cpp` 里空实现的平台钩子
  （`checkLayout`/`onActivated`/`onLocaleChanged`）、从没定义过的 `sendKeyEvent` 声明，
  以及两处 `#ifndef Q_OS_WIN` 里连接空槽的 `onDeactivated`——本工程只有 Win32 平台实现。

`whiteboard` 现在对 `desktop`、`recording`、`api`、`content` 与 `web` 都是**零出边**（这五组环因此消失）：

- 白板工具栏的“录制”按钮不再由 `BoardPaletteManager` 转发。这条 connect 挪到装配层
  （`app/Application.cpp` 的 `exec()`），转发的槽换成 `ApplicationController::toggleRecordingPalette`
  ——装配层本来就认识 `RecordingController`，画板模块不必为它 include `recording/`。
  顺手删掉 `BoardController::toggleRecording`：它没有发送者，而且真按下去会懒建一个
  录制控制器，把“没启用录制也创建录制管线”这个老问题留着。
- 桌面批注宿主：`BoardPaletteManager` 要把左右调色板挂到透明批注窗口上，过去它
  include `desktop/DesktopAnnotationController.h` 再 `uninotesController()->drawingView()`。
  现在装配层给两个动词——`desktopDrawingHost()`（给我一个能当父控件的窗口）和
  `desktopDrawingResized()`（宿主尺寸变了）——画板只认 `ApplicationController`。
  返回类型是 `QWidget*`，`DesktopAnnotationController` 留在 desktop 内部。
- `BoardView.cpp` 里那段 `Q_OS_OSX` 的手写板事件转发（`directTabletEvent` /
  `widgetForTabletEvent`，注释自己写着 Qt 4.8 的 workaround）连带删掉：本工程只有
  Win32 平台实现（`src/platform/*_win.cpp`），Mac 上根本链接不起来；而且那段代码 include
  的是 `core/ApplicationController.h`，头文件里 `DesktopAnnotationController` 只是前置声明，
  它调用成员函数——即便真在 Mac 上编译也是错的。
- 拖放门面反向边：`GraphicsWidgetItem::processDropEvent` 过去 `new WebWidgetBoardAPI(...)`
  再调它，画板因此 include `api/`。现在画板只认自己这层的 `whiteboard/IBoardDropHandler`
  接口（一个纯虚函数 + 一个工厂指针），`WebWidgetBoardAPI` 实现它，实现由装配点
  `app/main.cpp` 在启动时装上（`installBoardDropHandlerFactory`）。放在装配点而不是
  `Application` 的实现里，是因为 api 本来就认 core，装在 core 就会新造一组 `core` ↔ `api` 环；
  `app/` 是唯一允许认识所有模块的那一层。没装上也不会崩，只是不接拖放。
  同一次改动里删掉了 `WebWidgetAPI`/`WebWidgetPreferenceAPI` 两个门面与整条
  `javaScriptWindowObjectCleared` → `injectInlineJavaScript` 的死链：Qt6 没有
  `addToJavaScriptWindowObject`，注册它们的 connect 在移植时就断了线，`widget-inline-js`
  目录也不在部署里——只剩"被一个从没接上的槽 new 出来"这一处引用。
- PDF 那三家搬家：`GraphicsPDFItem` 的基类是 `content/PdfPageItem`，画板因此反向认
  content；另有 `BoardController.cpp` 的两条适配器 include 其实从没被使用，只是碰巧供给了
  `GeometryUtils::inchSize` 这类符号——现在改成直接 include `utils/GeometryUtils.h`。
  `PDFRenderer`/`QPdfRenderer`/`PdfPageItem` 三个类没有任何跨模块依赖，用它们的人全在
  content（而 content 本来就认画板 50+ 处），所以整簇搬进 `src/whiteboard/`：它做的是
  “一页 PDF 怎么变成画布上的图形”，不是文档格式读写。搬完 `whiteboard` 对 content 零出边，
  `tst_pdfrenderer` 的源路径与 include 一并改到 `whiteboard/`。
- 浏览器跳转：`BoardPaletteManager::linkClicked` / `purchaseLinkActivated` 和
  `GraphicsItemDelegate::gotoContentSource` 三处都是同一个两步——`showInternet()` 再
  `Application::webController->loadUrl(...)`。合成装配层的一个
  `ApplicationController::openInternetUrl(QUrl)`，画板只认 `core/ApplicationController.h`；
  另外四条 `web/*` include（BrowserWindow、TabWidget、WebView、WebKitUtils）本就没人用，删掉。

`content` ↔ `ui` 也断掉了，两个方向各两处：

- 下去的一条：`ExportAdaptor::showErrorsList()` 自己 `new MessagesDialog(tr(...), Application::mainWindow)`
  弹导出警告。改成 `reportErrorsList()` 只 `emit exportWarnings(title, messages)`，
  由注册导出器的 `core/DocumentManager` 连上弹窗——导出层不管界面怎么显示。
  同一个文件里两处 `QFileDialog` 拿 `Application::mainWindow` 当父窗口，为此在 core 加了
  `Application::dialogHost()`（返回 `QWidget*`）：下层要的是“对话框挂在谁身上”，
  不是 MainWindow 这个类型。
- 上来的一条：`ui/DocumentNavigator.cpp` 那两条 `content/ThumbnailAdaptor.h` +
  `SvgSubsetAdaptor.h` 从没被使用（跟 `BoardController` 那次一样，只是历史上供给过符号）。

### 拆环顺序（从叶子往根拆）

按下面这个顺序拆，每一步都能把某个模块的出边降到“只指向更低的层”（或者至少让没有
任何模块反向依赖它），然后就该模块从 `OBJECT` 改成 `STATIC`。

1. ~~**`utils/`**~~（已完成：出边为零，已收紧成 STATIC）
2. ~~**`settings/`**~~（已完成：资源目录下沉到 `utils/FileSystemUtils`，登录名由
   `core/Application` 启动时注入；出边只有 utils，已收紧成 STATIC）
3. ~~**`api/`**~~（已完成：见上一节最后一条，`whiteboard` 对 api 零出边。本模块仍认
   whiteboard/core/model/content/utils，是画板之上的一层门面）
4. ~~**`model/`**~~（已完成，2026-10-21）：两步。先 `model` → `content`
   （元数据与缩略图抽成 `model/IDocumentFormatServices`，见上面第 4 步前半那段），再把
   `model/DocumentController.{h,cpp}` 整个搬到 `ui/`——它是界面装配代码：建文档库控件、
   建导出菜单、连主窗口十几个动作、弹 MessageWindow、装 MousePressFilter。搬之前先把
   两处"下层回头问界面"的依赖反转掉：`DocumentTreeModel` 从控制器里拆成
   `model/DocumentTreeModel.{h,cpp}`（原来声明寄在 `core/PersistenceManager.h`、实现写在
   控制器里），`ExportDocumentSetAdaptor` 改由调用方把选中的树节点当参数传进来
   （`persist(proxy, selectedTreeIndex)`），导出器不再 include 控制器。
   搬完 `model` 的出边只有 core:9 / settings:4 / utils:4（2026-10-21 又收进 Resources，
   现为 core:10 / settings:5 / utils:5），两组环（`model↔ui`、
   `model↔whiteboard`）一起消失。**还收紧不了 STATIC**：`model↔core` 仍在，
   要等第 9 步把 `PersistenceManager`/`DocumentManager` 从 core 拆出来。

5. ~~**`platform/`**~~（已完成：`WindowCapture` 只收一个 `QWidget*` 作捕获目标，不再认
   `desktop/*` 与 `whiteboard/BoardView`；`KeyboardPalette*` 与 `CustomCaptureWindow*`
   这两个界面组件上移到 `ui/`，平台层只留 `KeyboardSender` 的 `SendInput`；出边只剩
   utils，已收紧成 STATIC）
6. ~~**`content/` ↔ `ui/`**~~（已完成：导出器改成 `emit exportWarnings(...)`，弹窗交给
   `core/DocumentManager`；对话框父窗口经 `Application::dialogHost()` 拿 `QWidget*`；
   `ui/DocumentNavigator` 的两条死 include 删除。`content` 剩下的脏边都在
   `content` ↔ `core`（44/15）与 `content` ↔ `model`（22/5）这两组上）
7. ~~**`desktop/` + `recording/` + `web/`**~~（已完成：画板侧对这三者的直接调用全部换成
   装配层的转发——录制按钮、批注宿主、`openInternetUrl`，见上一节）。2026-10-21 又补一刀：
   `core` 对 `recording` 的最后一条边（`ApplicationController::toggleRecordingPalette` 里那句
   `RecordingController::instance()`）改成装配点装上的 `installRecordingToggle` 钩子，于是没有任何
   模块反向 include recording，`tb_recording` 收紧成 STATIC、`core` ↔ `recording` 那组环消失（双向环
   8 → 7）。剩下 `core` → `desktop`(2) 与 `core` → `web`(2) 是同一条路，拆完这两个模块也能跟进
   STATIC。desktop 这一条随后断掉：core 的总控器改经 `core/IDesktopAnnotation.h`
   这份契约用批注层（drawingView/showWindow/hideWindow/closeWindow/relayout/updateShowHideState
   + `imageCaptured` 信号 + `screenLayoutChanged` 槽），实现类由装配点装的工厂创建并以总控器为
   parent，所以所有权与生命周期跟拆环前一模一样；`tb_desktop` 因此也收紧成 STATIC，双向环
   7 → 6。顺手删掉两处死代码：`desktop/DesktopPalette.h` 是总控器里从没用过的 include，
   `restoreBoard` 是一个从没被 emit 过的信号（桌面工具栏的"显示 TianBoard"走的是
   `goToBoard()` → `showBoard()`），以及零调用者的 `uninotesController()`。工厂没装上时构造
   直接 `qFatal`——批注层是四个主模式之一，不是可选装饰；`--ui-smoke` 现在会进一次桌面批注
   再撤出来（tst_ui_modes 多出这一腿，把工厂装回 nullptr 立刻判红）。
   web 这一条同路断掉，而且断得更彻底：`Application::webController` 这个全局指针的类型从
   `WebController*` 换成 `core/IWebController.h` 的契约（show/loadUrl/adaptToolBar/showTabAtTop/
   cut/copy/paste/controlView/closing/installCookiePolicy/cookiesMarkedForDeletion +
   imageCaptured 与 activeWebPageChanged 两个信号 + screenLayoutChanged 槽），建它改走
   `Application::installWebControllerFactory`。契约里 `activeWebPageChanged` 带的是 `QWidget*`
   而不是 `WebView*`——发的一端隐式向上转型即可，收的一端（recording 选捕获源、ui 选镜像源）
   本来就只当窗口用；于是 recording 与 desktop 对 web 也归零，`tb_web` 收紧成 STATIC，双向环
   6 → 5。`--ui-smoke` 走网页模式、`--cookie-selftest` 直接量 `cookiesMarkedForDeletion()`，
   两条都覆盖这套装配；把 install 调用注释掉，应用构造期 qFatal、冒烟用例 exit 3 立刻判红。
8. **`ui/` ↔ `whiteboard/`（最大的一组，36/33：`Resources` 搬去 `model/`、`ToolWidget` 与 `Magnifier` 搬去
   `whiteboard/` 之后从 43/50 一路降下来）**：拆成“画板拥有视图，调色板经
   接口回调解”。这组动作量大，且都在交互热路径上，必须单独排期、单独回归。
9. **`core/`**（进行中）：偏好设置对话框（`PreferencesDialog` + `PreferencesController` +
   `BrushPropertiesFrame`）已搬去 `ui/`——一个 QDialog 不该住在服务层。搬的时候顺手收掉两条
   向上够的边：`recording/RecordingController.h` 是从没用过的死 include（删），Cookie 那一组
   控件不再自己去问 `WebController`，改成发 `cookieSettingsChanged()`、由 `core/Application`
   重装过滤器并把"退出会清掉几个"回推给 `setCookiesMarkedForDeletion()`（ui 对 web/recording
   因此归零，否则会新造两组环）。
   **当时剩下 8 组环里 7 组都过 `core`**，主体是 `Application`/`ApplicationController` 这两个装配
   角色，另有上面点名的五处"服务伸手拿界面"。收尾手法与前几刀同一路：服务只发信号或只收
   `QWidget*`（`Application::dialogHost()` 已是这个形状），装配动作留在装配层。已按这条路
   拆掉两处服务漏点：`NetworkAccessManager` 不再 include `ui/ProxyDialog.h`——站点要凭据时
   改问装配期装上的钩子 `installCredentialPrompt`（实现是 `ProxyDialog::prompt`，接在
   `app/main.cpp`）；`DownloadManager` 不再命令主窗口显示/隐藏下载窗口，只广播事实
   （`hasModalDownload()` + 原有 `downloadModalFinished`），`ui/DownloadWidget` 自己决定可见性，
   `MainWindow` 那两个各只有一次调用者的转发函数删掉，顺手补上那个无父顶层窗体的释放。
   同一轮又拆三处：导出告警与仓库丢失提示统一走 `UserNotices`（装配期装上的 presenter，
   实现是 `ui/MessagesDialog` 与主窗口的提示框，没装就退化成日志；这个门面 2026-10-22 从
   core/ 下沉到 utils/，理由见第四节）；
   `DisplayManager` 不再认识 `ui/BlackoutWidget`，改成 `installBlackoutSurfaceFactory`
   返回一个 `QWidget*` 并把"用户活动"作为回调传回来（不需要为拆环发明带信号的接口类）；
   `DocumentManager` 那两条 `ui/MainWindow.h`/`ui/DocumentController.h` 确认是死 include 删掉
   ——**判死之后仍要编译一次**，本次就有一个"看着是死引用其实在替调用方供给类型"的例子。
   core 对 ui 因此只剩 8 条，全在 `Application`(4)/`ApplicationController`(3) 这两个装配角色
   与 `PersistenceManager::processInteractiveReplacementDialog`(1)。最后这一条已于 2026-10-21
   拆掉：`PersistenceManager` 遇到重名不再自己弹对话框、也不再命令控制器换文档，改问装配期装上
   的 `installDuplicateNameResolver`（实现是 `ui/DocumentReplaceDialog::ask`，接在 `app/main.cpp`），
   顶掉当前文档时只发一个事实 `currentDocumentReplaced(incoming, outgoing)`；`copyDocumentScene`
   也不再拿 `Application::documentController` 往缩略图条里插一张图，改成复用既有契约
   `documentSceneCreated`（`documentSceneChanged` 本来就按"屏幕上是不是这份文档"决定要不要重建）。
   于是 `src/core/` 里除那两个装配角色没有一个文件认识 `ui/`：7 条 = Application 4 +
   ApplicationController 3，剩下的都是"装配层本来就认识所有模块"的形状。
   拆之前先补了 `app/ImportSelfTest.{h,cpp}` + `tests/tst_import.cpp`：重名裁决的取消/改名/替换
   三条路径原先必须弹窗才走得到，人工点不全，桩裁决器把它们变成七个可自动执行的案子（不撞名
   不征询、取消整包退回磁盘外、改名两份共存、同名替换且画板跟到新文档、.tbx 整批那一腿、没装
   钩子时不登记、删掉当前文档那一行）。补测试的过程里抓到两处真缺陷，都记在
   `docs/code-suspicions.md`：`QModelIndex::sibling()` 被当成"取这个节点的第 i 个孩子"用了两处，
   以及 `mCurrentNode` 没人负责清。
   五个 `*SelfTest` 与它们的分发表也已搬去 `app/`（2026-10-21，同一轮）：core 只剩一个通用钩子
   `Application::setSelfTestRun(延时, 入口)`，`app/SelfTests.cpp` 按命令行决定跑哪个、已有实例
   在跑就退 75。搬完 core 对 whiteboard 从 46 降到 30、对 content 从 19 降到 16；对 ui 经过
   这几轮从 17 降到现在的 6（`Resources` 搬去 `model/` 又去掉一条）。
   **最后这 6 条用"搬实现"收尾，没有新抽象。** `Application.cpp` 与 `ApplicationController.cpp`
   整体 `git mv` 到 `app/`，两个头文件仍列在 `tb_core` 里。理由不是"core 太大"，而是这两个类
   的函数体本来干的就是装配：建主窗口、造四个控制器、把 `QAction` 连到模式切换；`core` 里其余
   的服务一个都不碰界面。把它们留在 `core` 会让"服务层不认识界面"这条永远不成立，而把它俩
   的声明也一起搬去 `app` 更糟——200 多个调用点要从 `app/` 反向够进来，`app` 本来不计入环表，
   那等于把耦合藏到指标后面。搬完 `core` → `ui` 归零，`core` ↔ `ui` 那组环消失（5 → 4 组），
   顺带 `core` 对 whiteboard 17、model 6、platform 4、settings 7、utils 4 各降一截，这些边
   全都挪进了 `app` 那一行。AUTOMOC 仍按头文件走：`moc_Application*.cpp` 在 `tb_core`，成员函数
   在最终 exe 的目标里，同一次链接解得开（`tst_smoke` 与 `tst_ui_modes` 真把应用拉起来走完四个
   模式，装配漏了会在构造期 `qFatal`，两条都还判绿）。

10. **`content` ↔ `core` 与 `core` ↔ `model`**（2026-10-22 完成，环 4 → 2）：第四种形状，
   **契约住在被依赖的那一层，实现由高层继承它，装配点（或实现自己的构造函数）装上**。
   `model/IDocumentStore` 一次拆掉两组环：content 的导入导出器与 model 的容器/树模型都改走它，
   `PersistenceManager` 继承并在构造首行 `installDocumentStore(this)`（构造扫描阶段就会用到，
   晚上装一行就是启动瞬间的 `qFatal`）、析构核对身份再摘。接口里 `GraphicsScene` 与
   `DocumentTreeModel` 一律只前置声明——否则 model 反过来 include whiteboard，新环当场造出来。
   另一半力气花在**把挂错地方的数据搬下去**：`.tbz` 目录名与 `folders.xml` 词表进
   `model/PackageLayout.hpp`、四个 MIME 类型名进 `model/MimeTypes.hpp`、静态对象宿主
   `staticMemoryCleaner` 进 `utils/StaticMemory`、状态消息与对话框父窗口进 `utils/UserNotices`、
   `core/MimeData` 按载荷类型拆成 `model/MimeData`（只装"哪份文档第几页"）与
   `whiteboard/MimeDataGraphicsItem`（装图形项副本，析构要 `qDeleteAll(Item*)`，留在画板）。
   落点不是凭感觉挑的：每个模块的 `CMakeLists.txt` 里那句"允许依赖"就是合同，搬完必须没人违约。
11. **下一刀的形状已经量好了**（2026-10-22 收尾时测的）：剩 `core` ↔ `whiteboard`(16/25) 与
   `ui` ↔ `whiteboard`(29/30)。`whiteboard` 对 `core` 的 25 条已经不多，成分是
   `Application.h` 10（`mainWindow` 3 个文件、`applicationController` 5、`app()` 6、`undoStack` 5）、
   `ApplicationController.h` 5、`PersistenceManager.h` 3，其余是各个服务各一条。
   **最便宜的一条是"再还一个归属"**：全局撤销栈 `Application::undoStack`（39 处）本来就是画板的
   撤销栈，搬成 `BoardController` 的成员即可，手法与 `BoardController::instance()` 那一刀完全相同
   （那次一刀把 whiteboard 对 core 从 46 削到 25）。`verbose` 那个日志开关同理该去 `utils`。
   真正难的是 `ui` ↔ `whiteboard`：`ui→whiteboard` 里 14 条是 `BoardController.h`（界面要问画板
   要视图/场景/缩放），`whiteboard→ui` 里是 `MainWindow.h`/`StylusPalette.h`/`FeaturesWidget.h`
   这些"画板回头去命令调色板"的方向——必须按第 8 步说的"画板拥有视图，调色板经接口回调解"来拆，
   且全在交互热路径上，要单独排期、单独回归。
12. **第 11 步那把最便宜的刀已经落下**（2026-10-22）：`Application::undoStack` 搬成
   `BoardController::undoStack()`——静态访问器读一个类内的 `QPointer<QUndoStack>`，栈本身在
   `BoardController` 的构造函数里创建，39 处调用点全在画板内部。父对象仍留
   `StaticMemory::cleaner()` 而不是 `this`：`~Application` 里文档与场景晚于界面销毁，那段窗口
   场景还会往栈里压收尾命令（`GraphicsScene` 里那条 `if (undoStack)` 守卫就是为它留的），
   让栈跟着总控器一起死会改变退出期的生命周期。顺手删掉 `core/Application.h` 的
   `<QUndoStack>` 与三处重复 include。`whiteboard` 对 `core` 25→22，环仍是 2 组。
   实测过的收获：这条改动的时序前提（画板总控器一定早于任何场景诞生）不是靠推理收下的——
   `--snap-selftest` 与 `--format-selftest` 的日志里若出现 `QObject::connect: Cannot connect`
   就说明栈晚了，两处都是零。
   第 11 步顺带提的"`verbose` 同理该去 `utils`"量过之后是**空刀**：全仓只有 `app/main.cpp` 读它
   （装详细日志处理器），`content/ExportAdaptor` 另有一个自己的 verbose，`whiteboard`/`ui`
   一处都不碰——搬它只动 `app` 那一行，两组环一条边都不少。
13. **`core` 对 `whiteboard` 少的那 2 条是删死类删出来的**（2026-10-22）：`core/IdleTimer.*` 在
   `src/`、`tests/`、`res/`、`docs/` 里全量 grep 只剩它自己和 `src/core/CMakeLists.txt` 的源文件清单，
   没有任何构造点——它想做的“画板有焦点且 2 秒无输入就把鼠标指针藏掉”从来没生效过（事件过滤器
   没装上、定时器也不存在）。画板上真正看得见的藏指针是另一条路：`BoardView.cpp:1133` 与
   `GraphicsScene.cpp:862` 直接对 viewport `setCursor(Qt::BlankCursor)`。所以删它不改变任何可观察
   行为，只削掉它 `.cpp` 里那两条最没道理的上边（`whiteboard/BoardController.h`、
   `whiteboard/BoardView.h`）。判死的规矩没变：grep 只给候选，删完六道门禁全绿、24 条 ctest 全绿、
   `/W4 -Clean -Strict` 零告警才算数。环仍 2 组，`core` ↔ `whiteboard` 由 16/22 到 14/22。
14. **`DisplayManager` 与 `DownloadManager` 改成“只报事实”，core→whiteboard 14→10**（2026-10-22）：
   这两处是第 11 步列的最后两类“服务层回头命令画板”，形状与第 8 步拆 `content`→`ui` 那一刀完全相同。
   - `DisplayManager::setPreviousDisplaysWidgets(QList<BoardView *>)` 收的其实是**窗口**：它只调
     `hide()`/`setGeometry()`/全屏，所以签名换成 `QList<QWidget *>`，由装配点按视图顺序交进来；
     `whiteboard/BoardView.h` 这条 include 随之删除（顺带发现 `core/ApplicationController.h` 在这个
     .cpp 里一处都不用，也删了）。
   - `unBlackout()` 过去直接 `BoardController::instance()->freezeWebWidgets(false)`；现在只
     `emit blackoutLifted()`，应答方 `BoardController::blackoutLifted()` 由 `app/ApplicationController`
     连上——与同一处已经存在的 `adjustDisplayViewsRequired`→`adjustDisplayViews` 是同一个模式。
   - `DownloadManager` 里两处 `BoardController::instance()->paletteManager()->startDownloads()/
     stopDownloads()` 换成 `downloadsQueued()`/`downloadsDrained()` 两条事实，由画板在
     `BoardController::init()` 里把调色板的应答连上（连线两端都在画板内部，不必绕装配点）。
   - `AsyncLocalFileDownloader::prepare()` 需要“当前打开的是哪份文档”才能算出复制的目标路径，而这是
     画板的状态。这里改成第 11 步那个“再还一个归属”的变体：**被依赖的一层留口子，实现由装配点装上**
     （`DownloadManager::installTargetDocumentProvider`，仿的是同目录里已有的
     `DisplayManager::installBlackoutSurfaceFactory`）。**顺带修掉一处真隐患**：旧写法两处
     `BoardController::instance()->…` 都不判空，画板还没诞生（或已经析构）时就是空指针解引用；
     钩子给不出文档时现在打一条告警并让这次下载按失败收尾。
   第 25 条自检 `--download-selftest`（`tst_download`）盯的就是这套新接线，量的是产物与界面状态而不是
   调用关系：入队时右侧面板那个下载标签页出现、文件按字节落进这份文档的 `audios/`、排空后标签页收回、
   源文件仍在，外加事实各报几次。三条反向对照各自只打红该红的那一条——不装提供者钩子 4 处红（文件
   无处可去）、摘掉画板对下载起止的两条连线只有“标签页没出现”1 处红（下载本身照旧成功）、去掉
   `emit blackoutLifted()` 只有“报了几次”1 处红。环仍 2 组，`core` ↔ `whiteboard` 10/22。
15. **`DocumentManager` 那 4 条里有 3 条根本不存在**（2026-10-22）：`GraphicsSvgItem.h` 与
   `GraphicsPixmapItem.h` 在这个 .cpp 里一个符号都不碰，`GraphicsScene.h` 只服务两个局部指针
   （`createDocumentSceneAt` 的返回值交给 `placeImportedItemToScene`）——类型名由
   `core/PersistenceManager.h` 的前置声明供给，删掉 include 后编译照旧通过。判死的规矩与第 13 步
   相同：**编译器是唯一裁判**，grep 说“没用到”不算。顺带删掉重复的 `content/ExportWeb.h` 一行。
   10→7。剩下的 7 条是一簇，不是零散：`PersistenceManager` 4（复制页时按图元类型搬包内文件）、
   `SceneCache` 2（缓存并 `new` 出 GraphicsScene、存取它的视图状态）、`DocumentManager` 1
   （导入完请画板补一页缩略图）。要归零就得让**场景的生命周期整个搬到画板**：契约留在 core 按指针传
   （`model/IDocumentStore.h` 已经这么干了），实现住在画板——见任务表，这一刀要单独排期，因为
   `loadDocumentScene` 留在 core 是有理由的：它调 `content/SvgSubsetAdaptor`，而画板对 content
   必须保持零出边（第 6 步刚把那一组环拆掉）。
16. **图元资源复制整段搬回“认得命名规则的那一层”，core→whiteboard 7→3**（2026-10-22）：
   `PersistenceManager::duplicateDocumentScene` 里那段 70 行的循环——按 `GraphicsMediaItem`/
   `GraphicsWidgetItem`/`GraphicsPixmapItem`/`GraphicsSvgItem` 四种类型把包内文件补出一份、再给图元
   换新 UUID——搬成 `SvgSubsetAdaptor::duplicateItemResources(proxy, scene)`。**这一刀不需要新契约**：
   `core → content` 本来就是合法的向下边，而“图元 ↔ 包内文件叫什么名、摆在哪个目录”这套知识一直住在
   content（`pixmapItemToLinkedImage`/`svgItemToLinkedSvg`/`pdfItemToLinkedPDF` 与音视频那几支写出器都是它），
   复制这一侧只是同一份知识的另一个方向——留在 core 等于把知识抄一遍。搬的时候有一条必须钉住：
   **传进去的必须是场景缓存里的那一个场景**，另 `loadScene` 一份改完再落盘，缓存里那份还带着旧 UUID；
   这条写进了函数注释与头文件的 `@note`。
   顺带补的债：`--pages-selftest` 原来只比“每一页的内容清单”，而**内容清单看不出两页共用同一份文件**
   （像素一样、文本一样，只有删掉一页时另一页的图才跟着没）。现在每一步之后再加一条**资源归属**判据：
   每份 `images/<UUID>.png` 只许属于一页，且必须真在盘上。反向对照：把 `duplicateItemResources` 开头
   短路，`tst_pages` 立刻 2 处红（“复制第 1 页之后第 1 页与第 2 页共用同一份包内文件”，
   “删掉第 2 页之后”同一句）——搬过去的代码从此有人钉。
   还剩 3 条：`SceneCache.h`/`.cpp` 各一条（缓存并 `new` 出 GraphicsScene、存取 `SceneViewState`），
   以及 `DocumentManager` 那条 `insertThumbPage`。
17. **“追加完请画板补一张缩略图”换成装配点装上的处理器，core→whiteboard 3→2**（2026-10-22）：
   `DocumentManager::addFilesToDocument` 里那句 `BoardController::instance()->insertThumbPage(pageIndex)`
   是 core 最后一处“直接命令画板”的界面动作。改成 core 只报事实：`DocumentManager::notifyPagesAppended
   (document, pageIndex)`，实现由装配点装上（`DocumentManager::installPagesAppendedHandler`，与
   `installBlackoutSurfaceFactory`/`installTargetDocumentProvider` 同一个形状）。**装上去的那一版还顺带修
   了一处真可疑**：旧写法无条件把条目插进**画板容器**的列表，而同一个循环里 `createDocumentSceneAt`/
   `persistDocumentScene` 用的是传进来的那份 `document`——从文档库往**别的文档**追加一页时，挤位的是
   画板当前那份文档的缩略图。现在实现里先问一句“这份文档是不是画板当前那一份”。
   这一刀的判据是先把路走通再动的：`--import-selftest` 新增第 11 案，用 `QPainter` 现画一张 PNG 走
   界面上同一个入口 `addFilesToDocument`，断页数 +1、新的那一页有位图且像素与源图一致、`images/` 里
   确有那份文件、画板缩略图列表跟着长一条；再往**另一份**文档追加一次，断画板那份列表**不许**跟着长。
   两条反向对照各自打红该红的那一条：去掉那句 `insertThumbPage`（等于不装处理器）→“列表 1 条 vs 页数 2”；
   去掉实现里那道“是不是当前文档”的判断 →“往别的文档追加一页，画板那份列表却被改成 3 条”。
   顺带纠正一处我自己差点写错的判据：第一条案子的 target 文档原本用 `createDocument(..., pageCount=1)`
   造，看着有 1 页其实**没有页文件**，`loadDocumentScene` 返回空、`setActiveDocumentScene` 根本不切文档，
   于是“画板当前文档”一直是别的文档——补了一条“画板必须真的切过去”的前置断言才把这条路量对。
   core 对 whiteboard 只剩 `SceneCache.h`/`.cpp` 那 2 条（缓存并造出 GraphicsScene、存取 `SceneViewState`），
   要归零就得把**场景的生命周期搬到画板**：契约留在 core 按指针传、实现住在 whiteboard，且
   `loadDocumentScene` 必须留在 core（它调 `content/SvgSubsetAdaptor`，画板对 content 要维持零出边）。
18. **最后 2 条边的真实代价已经量出来了，别用"搬文件"糊过去**（2026-10-22 收尾时量的）：
   `core → whiteboard` 只剩 `SceneCache.h`/`.cpp` 两条 include，看上去一次 `git mv` 就能归零——
   `content/` 是被允许认识画板的层（`SvgSubsetAdaptor::loadScene` 本来就在那儿把页字节造成场景），
   把缓存搬过去，core 只 include `content/SceneCache.h`，那条 grep 当场就是 0。**但这是糊数字**：
   `content/SceneCache.h` 仍然 `#include "whiteboard/GraphicsScene.h"`，而 `PersistenceManager.cpp`
   正是靠这条传递包含拿到 `GraphicsScene` 的完整类型——它手里那些点全都要这个类型：
   - `createDocumentSceneAt`：`newScene->setBackground(...)`、`setBackgroundGridSize(...)`
   - `insertDocumentSceneAt`：`scene->setDocument(proxy)`
   - `persistDocumentScene`：`pScene->deselectAllItems()`、`isModified()`、`setModified(false)`
   - 搬对象那一支：`scene->relativeDependencies()`、`scene->document()`
   - 判空那一支：`theSoleScene->isEmpty()`
   外加 `SceneCache` 自己的 `new GraphicsScene(...)`、`viewState()/setViewState()`、`isActive()`。
   搬完这些一个都没少，只是不再被 grep 看见。**所以真正的最后一刀是把“场景的生命周期”整个搬到画板**：
   契约留在 core（`GraphicsScene` 只前置声明，照 `model/IDocumentStore.h` 的写法），实现住在 whiteboard，
   上面那七个动作各变成契约上的一个方法；`loadDocumentScene` 本身**必须留在 core**（它调
   `content/SvgSubsetAdaptor`，而画板对 content 要维持零出边）。调用点今天逐文件数过：
   core 之外 52 处——`app/`（六个自检加 `ApplicationController`）20、`model/` 12（其中 7 处是
   `IDocumentStore` 的声明）、`ui/` 8、`whiteboard/` 7、`content/` 5；core 内 29 处
   （`PersistenceManager.cpp` 19 + `.h` 9 + `SceneCache.cpp` 1）。是一次设计活，
   **别在长时间会话末尾做这种抽象选择**——门禁抓得住编译与测试退化，抓不住“抽象选错了”。

`docs/code-suspicions.md` 记的是另一类问题（可疑行为），与这里的依赖债分开。

## 六、模块依赖边全表（现状态，供拆环时核对）

出边（本模块 include 别的模块；`x → y : n` 表示 x 里有 n 处 `#include "y/…"`，
由 `src/` 全量扫描得到）：

```text
api         -> content:1, core:4, model:1, settings:1, utils:3, whiteboard:6
app         -> api:1, content:12, core:43, desktop:1, model:24, platform:5, recording:2,
               settings:13, ui:15, utils:11, web:1, whiteboard:59
content     -> model:30, platform:8, settings:7, utils:31, whiteboard:45
core        -> content:14, model:9, platform:4, settings:7, utils:9, whiteboard:2
desktop     -> core:7, platform:4, settings:3, ui:11, whiteboard:8
model       -> settings:5, utils:8
platform    -> utils:1
recording   -> core:5, model:1, platform:2, settings:3, ui:3, utils:6, whiteboard:4
settings    -> utils:2
ui          -> core:27, model:15, platform:10, settings:16, utils:11, whiteboard:29
utils       -> （无出边）
web         -> core:13, platform:3, settings:4, ui:5, utils:3, whiteboard:4
whiteboard  -> core:22, model:21, platform:5, settings:20, ui:30, utils:54
```

`core` 还另有 1 处 `#include "third/singleapp/…"`（见第三节），那是内联第三方而不是模块。
2026-10-22 这张表重测过七次：`Application::boardController` 那一刀把
`whiteboard` 对 `core` 从 46 削到 25、`ui` 从 33 到 27；`Application::undoStack` 那一刀再把它从
25 削到 22（`SelectionFrame.cpp` 因此不再 include `core/Application.h`，还顺手删掉两行重复 include）；
删掉没人实例化的 `core/IdleTimer` 把 `core` 对 `whiteboard` 从 16 削到 14；
`DisplayManager`/`DownloadManager` 改成只报事实再削 4 条（14→10）；
`DocumentManager` 里三条“其实不存在”的 include 再削 3 条（10→7）；
图元资源复制整段搬回 content 的写出器再削 4 条（7→3）；
“追加完请画板补缩略图”换成装配点装上的处理器再削 1 条（3→2）。
同一轮全表重测还纠正了三处旧数：`whiteboard` 对 `utils` 是 54 不是 52（`UserNotices` 下沉那一批
改的就是这条边，当时只按直觉记了增量）；`core` 对 `content` 是 14 不是 15（第 15 步删那行重复
`content/ExportWeb.h` 时就该跟着改，只按显眼的方向记了数）。
`app` 那一行本来就该最重（它是装配点，自检也从这里伸手进各层）：`whiteboard:59`、`model:24`、
`core:43`、`ui:15`、`content:12` 比旧表多出来的那几条，是这一轮新加的八个自检（录屏、剪贴板、导出、
撤销重做、自动保存、页面、媒体、本地下载）新 include 的图元、场景与界面头文件——它们都住在
`src/app/`，所以这些边只长在装配点上，没有给任何一层加新账。
2026-10-22 拆掉 `content` ↔ `core` 与 `core` ↔ `model` 两组环之后，`content` 与 `model` 都不再
有对 core 的出边（`model` 如今只剩 settings 与 utils，是真叶子）；上表里 →model 变多的那几笔
（content 30、whiteboard 21）是仓库常量、MIME 名字与拖放载荷搬进 model 的结果，方向合法。
上面 `core` 那一行是 2026-10-21 把 `Application.cpp`/`ApplicationController.cpp` 搬去 `app/`
之后重测的：core 对 ui 归零，对 whiteboard 28→17、model 8→6、platform 8→4、settings 11→7，
这些边全部转到了 `app` 那一行——`app` 是装配点，本来就该这么重。
2026-10-22 又把 `UserNotices` 从 core/ 下沉到 utils/ 并加了一条状态条通道：`content` 对
core 从 26 降到 17（9 个导入导出器不再为了一句"导出成功"认识应用对象），对 utils 从 21 升到
31——那是朝下的一条边，不是新账。`utils` 的出边仍然是零。
重跑这张表不需要脚本，一条 grep 就够：

```text
grep -rhoE '#include[[:space:]]+"[a-z]+/' src/<模块> | sed -E 's/.*"([a-z]+)\/.*/\1/' \
  | sort | uniq -c
```
