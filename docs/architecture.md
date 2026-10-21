# TianBoard 架构说明

配套文档：[编码规范](coding-style.md) · [疑点台账](code-suspicions.md) · [根 README](../README.md)

本文说明三件事：源码分成哪些模块、构建为什么是现在这个样子、以及**已知的依赖环**
（按什么顺序拆）。模块划分是 2026-10 的结构调整结果，只做目录搬迁与 include 前缀
改写，没有改任何行为。

---

## 一、源码结构

```text
src/
  ├─ app/          可执行目标本体：main.cpp、TianBoard.rc、resource.h、启动装配
  ├─ settings/     设置项本身：Settings（全部 Setting 的注册表）+ Setting<T> 模板
  │                （只依赖 utils，与 utils 一样是真 STATIC）
  ├─ core/         应用与各类服务/控制器（Application、ApplicationController、
  │                DisplayManager/VirtualDesktop、LanguageManager、PersistenceManager、
  │                DocumentManager、SceneCache、DownloadManager、NetworkAccessManager +
  │                CookieJar（可枚举的 Cookie 镜像罐）、
  │                ShortcutManager（快捷键注册表与偏好设置那一页的模型）…）
  ├─ model/        纯文档域：DocumentProxy、DocumentContainer、DocumentTreeModel
  │                （树节点 + 目录树模型 + 内部拖放载荷）、SortFilterProxyModel，
  │                出边只剩 core/settings/utils（原 document/）
  ├─ whiteboard/   画板：BoardView/BoardController/DrawingController + 图形场景与全部
  │                图形项 + 绘图工具（原 board/ + domain/ + tools/）+ PDF 页图形项与
  │                渲染器（PDFRenderer/QPdfRenderer/PdfPageItem，从原 pdf/ 搬来）
  ├─ content/      文档格式读写与导入导出：.tbz/.tbx 与 IWB 子集（IwbDocumentAdaptor /
  │                IwbSubsetAdaptor / ExportIwb / ImportIwb）、SVG/RDF 元数据、缩略图数据、
  │                PDF 导入导出（原 adaptors/；pdf/ 的图形项三家已归 whiteboard/）
  ├─ api/          小部件与文档对外 API 门面（WebWidgetBoardAPI/WebWidgetMessageAPI）
  ├─ web/          内嵌浏览器与网页控制器（原 web/ + web/browser/）
  ├─ desktop/      屏幕批注（平台无关部分）
  ├─ recording/    录屏：画面与声音采集、录制调色板、Windows Media 编码器
  ├─ ui/           全部界面控件与调色板（原 ui/ + widgets/ + network/ProxyDialog，
  │                加上从 platform/ 搬来的 KeyboardPalette* 与 CustomCaptureWindow*，
  │                以及从 model/ 搬来的 DocumentController——文档模式的界面装配）
  ├─ platform/     平台相关实现（不带界面的部分）：PlatformUtils*、WindowCapture*、
  │                KeyboardSender*、CredentialStore*（出边只有 utils）
  └─ utils/        纯工具：StringUtils、GeometryUtils、Version、FileSystemUtils（含
                   applicationResourcesDirectory）、Snapping、CookieRules、Globals、Enums.h
                   （出边为零，最底一层）
third/             内联第三方：quazip+zlib、qtsingleapplication
res/               images / etc / library / customizations / style / i18n(.ts) / qrc
tests/             Qt Test 用例（15 个）
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
代码里存在 8 组双向 include（下一节全表 + 台账）。STATIC 库遇到环需要 `--start-group`
之类的手段，等于把问题藏进链接参数里；OBJECT 库把所有 .obj 并进同一个可执行，环在
链接期不成问题，于是本次调整可以只拿到两样确定性的东西——**每模块的显式文件清单**
和**每模块的 include 白名单**——把拆环留到后续阶段。

已经做到的三个：`tb_utils`（出边为零）、`tb_settings`（出边只有 utils）与
`tb_platform`（出边只有 utils）。越界的 `#include` 在编译期就失败，而不是等到链接期靠
符号缺失才发现——这个配方对其它模块同样成立，`settings` 与 `platform` 都是照着
`utils` 拆出来的。

换成 STATIC 的前置条件：该模块的出边全部落在比它低的层（下表），且没有对上的
include。

## 五、依赖债：现存的双向环

下面是 `src/` 全量扫描出来的直接互相 include（数字 = 各方向的 include 处数，
含头文件里的；`app/` 作为装配点不计）。

| 环 | 边数 | 典型出处 |
|---|---|---|
| `core` ↔ `whiteboard` | 46 / 67 | `core/ApplicationController.cpp` 认 BoardController；`whiteboard/*.cpp` 认 Application/DocumentController |
| `core` ↔ `ui` | 13 / 54 | 五个服务伸手拿界面：`DocumentManager`、`PersistenceManager`（认 DocumentController/MainWindow）、`DisplayManager`（BlackoutWidget）、`DownloadManager`（MainWindow）、`NetworkAccessManager`（ProxyDialog），加上 `Application` 自己；`ui/*.cpp` 反过来认 Application |
| `content` ↔ `core` | 44 / 19 | `content/ExportAdaptor.cpp` 认 core；`core/PersistenceManager.cpp` 认 content 的读写器 |
| `ui` ↔ `whiteboard` | 43 / 50 | 调色板与画板互相认 |
| `core` ↔ `model` | 8 / 9 | `core/DocumentManager`/`PersistenceManager` ↔ `model/DocumentProxy`；model 反过来认 PersistenceManager/Application/DocumentManager |
| `core` ↔ `web` | 3 / 19 | `core/Application.cpp` 认 WebController |
| `core` ↔ `desktop` | 2 / 7 | `core/ApplicationController.cpp` 认 DesktopPalette |
| `core` ↔ `recording` | 1 / 5 | ApplicationController/PreferencesController 认 RecordingController |

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
  还认 `core/Application` 与 `core/VirtualDesktop`。平台层只剩下不带界面的那件事：
  一次按键怎么交给系统（`KeyboardSender`，`sendUnicodeKey`/`sendVirtualKey` 两个函数，
  Win32 `SendInput` 的实现）。搬完 `platform` 出边只剩 utils，`tb_platform` 因此也收紧成
  STATIC，`platform` ↔ `core` 与 `platform` ↔ `ui` 两组环一起消失。
  顺带删掉三个只在 `KeyboardPalette_win.cpp` 里空实现的平台钩子
  （`checkLayout`/`onActivated`/`onLocaleChanged`）、从没定义过的 `sendKeyEvent` 声明，
  以及两处 `#ifndef Q_OS_WIN` 里连接空槽的 `onDeactivated`——本工程只有 Win32 平台实现。

`whiteboard` 现在对 `desktop`、`recording`、`api`、`content` 与 `web` 都是**零出边**（这五组环因此消失）：

- 白板工具栏的“录制”按钮不再由 `BoardPaletteManager` 转发。这条 connect 挪到装配层
  （`core/Application.cpp`），转发的槽换成 `ApplicationController::toggleRecordingPalette`
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
  `app/main.cpp` 在启动时装上（`installBoardDropHandlerFactory`）。放在 `app/` 而不是
  `core/Application.cpp`，是因为 api 本来就认 core，装上就会新造一组 `core` ↔ `api` 环；
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

按下面这个顺序拆，每一步都能把某个模块的出边降到“只指向更低的层”，然后就该模块
从 `OBJECT` 改成 `STATIC`。

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
   搬完 `model` 的出边只有 core:9 / settings:4 / utils:4，两组环（`model↔ui`、
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
   装配层的转发——录制按钮、批注宿主、`openInternetUrl`，见上一节）
8. **`ui/` ↔ `whiteboard/`（最大的一组，40/48）**：拆成“画板拥有视图，调色板经
   接口回调解”。这组动作量大，且都在交互热路径上，必须单独排期、单独回归。
9. **`core/`**（进行中）：偏好设置对话框（`PreferencesDialog` + `PreferencesController` +
   `BrushPropertiesFrame`）已搬去 `ui/`——一个 QDialog 不该住在服务层。搬的时候顺手收掉两条
   向上够的边：`recording/RecordingController.h` 是从没用过的死 include（删），Cookie 那一组
   控件不再自己去问 `WebController`，改成发 `cookieSettingsChanged()`、由 `core/Application`
   重装过滤器并把"退出会清掉几个"回推给 `setCookiesMarkedForDeletion()`（ui 对 web/recording
   因此归零，否则会新造两组环）。
   **剩下 8 组环里 7 组都过 `core`**，主体是 `Application`/`ApplicationController` 这两个装配
   角色，另有上面点名的五处"服务伸手拿界面"。收尾手法与前几刀同一路：服务只发信号或只收
   `QWidget*`（`Application::dialogHost()` 已是这个形状），装配动作留在装配层。已按这条路
   拆掉两处服务漏点：`NetworkAccessManager` 不再 include `ui/ProxyDialog.h`——站点要凭据时
   改问装配期装上的钩子 `installCredentialPrompt`（实现是 `ProxyDialog::prompt`，接在
   `app/main.cpp`）；`DownloadManager` 不再命令主窗口显示/隐藏下载窗口，只广播事实
   （`hasModalDownload()` + 原有 `downloadModalFinished`），`ui/DownloadWidget` 自己决定可见性，
   `MainWindow` 那两个各只有一次调用者的转发函数删掉，顺手补上那个无父顶层窗体的释放。四个
   `*SelfTest` 与它们的分发表也已搬去 `app/`（2026-10-21，同一轮）：core 只剩一个通用钩子
   `Application::setSelfTestRun(延时, 入口)`，`app/SelfTests.cpp` 按命令行决定跑哪个、已有实例
   在跑就退 75。搬完 core 对 whiteboard 从 46 降到 30、对 content 19→16、对 ui 17→15。

`docs/code-suspicions.md` 记的是另一类问题（可疑行为），与这里的依赖债分开。

## 六、模块依赖边全表（现状态，供拆环时核对）

出边（本模块 include 别的模块；`x → y : n` 表示 x 里有 n 处 `#include "y/…"`，
由 `src/` 全量扫描得到）：

```text
api         -> content:1, core:4, model:1, settings:1, utils:2, whiteboard:7
app         -> api:1, content:4, core:10, model:4, platform:1, settings:5, ui:2, utils:4,
               web:1, whiteboard:17
content     -> core:44, model:23, platform:3, settings:12, utils:22, whiteboard:57
core        -> content:16, desktop:2, model:5, platform:6, recording:1, settings:14, ui:13,
               utils:6, web:2, whiteboard:30
desktop     -> core:7, platform:3, settings:3, ui:11, web:1, whiteboard:8
model       -> core:9, settings:4, utils:4
platform    -> utils:1
recording   -> core:5, platform:1, settings:3, ui:4, utils:3, web:2, whiteboard:4
settings    -> utils:3
ui          -> core:54, model:10, platform:11, settings:20, utils:12, whiteboard:43
utils       -> （无出边）
web         -> core:19, platform:1, settings:5, ui:5, utils:3, whiteboard:5
whiteboard  -> core:67, model:7, platform:5, settings:20, ui:50, utils:47
```

`core` 还另有 1 处 `#include "third/singleapp/…"`（见第三节），那是内联第三方而不是模块。
重跑这张表不需要脚本，一条 grep 就够：

```text
grep -rhoE '#include[[:space:]]+"[a-z]+/' src/<模块> | sed -E 's/.*"([a-z]+)\/.*/\1/' \
  | sort | uniq -c
```
