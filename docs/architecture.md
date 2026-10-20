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
  │                DocumentManager、SceneCache、DownloadManager、NetworkAccessManager…）
  ├─ model/        文档域：DocumentProxy、DocumentContainer、DocumentController、
  │                SortFilterProxyModel（原 document/）
  ├─ whiteboard/   画板：BoardView/BoardController/DrawingController + 图形场景与全部
  │                图形项 + 绘图工具（原 board/ + domain/ + tools/）
  ├─ content/      文档格式读写与导入导出：.tbz/.tbx 与 IWB 子集（IwbDocumentAdaptor /
  │                IwbSubsetAdaptor / ExportIwb / ImportIwb）、SVG/RDF 元数据、缩略图数据、
  │                PDF 渲染导出（原 adaptors/ + pdf/）
  ├─ api/          小部件与文档对外 API 门面（WebWidgetAPI/WebWidgetBoardAPI/WebWidgetMessageAPI）
  ├─ web/          内嵌浏览器与网页控制器（原 web/ + web/browser/）
  ├─ desktop/      屏幕批注（平台无关部分）
  ├─ recording/    录屏：画面与声音采集、录制调色板、Windows Media 编码器
  ├─ ui/           全部界面控件与调色板（原 ui/ + widgets/ + network/ProxyDialog）
  ├─ platform/     平台相关实现：PlatformUtils*、WindowCapture*、CustomCaptureWindow*、
  │                KeyboardPalette*
  └─ utils/        纯工具：StringUtils、GeometryUtils、Version、FileSystemUtils（含
                   applicationResourcesDirectory）、Globals、Enums.h
                   （出边为零，最底一层）
third/             内联第三方：quazip+zlib、qtsingleapplication
res/               images / etc / library / customizations / style / i18n(.ts) / qrc
tests/             Qt Test 用例（8 个）
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
代码里存在 17 组双向 include（下一节全表 + 台账）。STATIC 库遇到环需要 `--start-group`
之类的手段，等于把问题藏进链接参数里；OBJECT 库把所有 .obj 并进同一个可执行，环在
链接期不成问题，于是本次调整可以只拿到两样确定性的东西——**每模块的显式文件清单**
和**每模块的 include 白名单**——把拆环留到后续阶段。

已经做到的两个：`tb_utils` 与 `tb_settings` 是真 `STATIC`。前者出边为零；后者出边只有
utils。越界的 `#include` 在编译期就失败，而不是等到链接期靠符号缺失才发现——这个配方
对其它模块同样成立，`settings` 就是刚照着 `utils` 拆出来的。

换成 STATIC 的前置条件：该模块的出边全部落在比它低的层（下表），且没有对上的
include。

## 五、依赖债：现存的双向环

下面是 `src/` 全量扫描出来的直接互相 include（数字 = 各方向的 include 处数，
含头文件里的；`app/` 作为装配点不计）。

| 环 | 边数 | 典型出处 |
|---|---|---|
| `core` ↔ `whiteboard` | 33 / 65 | `core/ApplicationController.cpp` 认 BoardController；`whiteboard/*.cpp` 认 Application/DocumentController |
| `core` ↔ `ui` | 11 / 38 | `core/Application.cpp`、`PersistenceManager.cpp`、`DownloadManager.cpp` 直接认 `ui/MainWindow.h`；`ui/*.cpp` 反过来认 Application |
| `content` ↔ `core` | 44 / 15 | `content/ExportAdaptor.cpp` 认 core；`core/PersistenceManager.cpp` 认 content 的读写器 |
| `content` ↔ `whiteboard` | 50 / 3 | `whiteboard/GraphicsWidgetItem.*` 反向认 content |
| `ui` ↔ `whiteboard` | 40 / 46 | 调色板与画板互相认 |
| `core` ↔ `model` | 11 / 13 | `core/DocumentManager` ↔ `model/DocumentController` |
| `model` ↔ `ui` | 8 / 9 | `model/DocumentController.cpp` 直接建 ThumbnailView/MainWindow |
| `model` ↔ `whiteboard` | 6 / 9 | 文档页与图形场景互认 |
| `model` ↔ `content` | 22 / 5 | `model/DocumentContainer.cpp` 直接调 ThumbnailAdaptor |
| `core` ↔ `web` | 2 / 17 | `core/Application.cpp` 认 WebController |
| `core` ↔ `desktop` | 2 / 7 | `core/ApplicationController.cpp` 认 DesktopPalette |
| `core` ↔ `recording` | 2 / 7 | ApplicationController/PreferencesController 认 RecordingController |
| `core` ↔ `platform` | 7 / 3 | `platform/KeyboardPalette.cpp` 认 Application |
| `api` ↔ `whiteboard` | 8 / 2 | `whiteboard/GraphicsWidgetItem.cpp` 反向认 api 门面 |
| `content` ↔ `ui` | 2 / 2 | `content/ExportAdaptor.cpp` 弹 MessagesDialog/MainWindow |
| `platform` ↔ `ui` | 4 / 6 | `platform/KeyboardPalette.h` 认 `ui/ActionPalette.h` |
| `web` ↔ `whiteboard` | 5 / 7 | 互相认控制器与图形项 |

`utils` 的两条上边（`CryptoUtils.h`/`FileSystemUtils.cpp` → `core/Application.h`，以及
`CoreGraphicsScene.cpp` → 两个图形项）与 `settings` 的那条（`Settings.cpp` →
`platform/PlatformUtils.h` 拿资源目录，另加 `osUserLoginName`）都已断掉，这两个模块
都不在任何环上，也已经收紧成 STATIC。`platform` 自己少了两条上边：
`WindowCapture_win.cpp` 里那个 `whiteboard/BoardView.h` 从来没用过；抓窗口时独占鼠标
键盘的需求改成构造时收一个 `QWidget*`（`WindowCapture(QWidget *grabTarget, ...)`），
于是它不再认识 `desktop/DesktopPalette` 与 `DesktopAnnotationController`。

`whiteboard` 现在对 `desktop` 与 `recording` 都是**零出边**（这两组环因此消失）：

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

### 拆环顺序（从叶子往根拆）

按下面这个顺序拆，每一步都能把某个模块的出边降到“只指向更低的层”，然后就该模块
从 `OBJECT` 改成 `STATIC`。

1. ~~**`utils/`**~~（已完成：出边为零，已收紧成 STATIC）
2. ~~**`settings/`**~~（已完成：资源目录下沉到 `utils/FileSystemUtils`，登录名由
   `core/Application` 启动时注入；出边只有 utils，已收紧成 STATIC）
3. **`api/`**：断开 `whiteboard/GraphicsWidgetItem.cpp` → api 的反向边（小部件经
   QObject 消息桥调用画板能力，改为画板侧注册回调/接口，api 只认下层的 whiteboard）。
4. **`model/`**：断开 `model` → `ui`（DocumentController 直接实例化缩略图视图）与
   `model` → `content`（DocumentContainer 直接调 ThumbnailAdaptor）。两处都要抽接口：
   `IThumbnailProvider`、`IDocumentStore`，由装配层（core/app）注入。
5. **`platform/`**（部分完成）：`WindowCapture` 不再认 `desktop/*` 与 `whiteboard/BoardView`
   （抓窗口时只收一个 `QWidget*` 作为鼠标/键盘的捕获目标）。剩下的脏边是
   `KeyboardPalette` 认 `ui/MainWindow`、`ui/ActionPalette`，以及
   `CustomCaptureWindow` 认 `ui/RubberBand`——后者是三个模块共用的可选边框控件，
   放 `ui/` 是对的，要断它得把“框选交互”整体上移出平台层。平台层应当只暴露
   “捕获一块画面”“弹一个键盘”，由调用方实现宿主接口。
6. **`content/`**：主要脏边是 `content/ExportAdaptor.cpp` → `ui/MainWindow`、
   `ui/MessagesDialog`（导出进度与错误框）。导出器改为回报状态，界面自己决定怎么显示。
7. **`desktop/` + `recording/` + `web/`**（部分完成）：画板侧对 `desktop` 与 `recording`
   的直接调用已全部换成装配层的转发（见上一节）；剩下 `web` ↔ `whiteboard` 一组，
   要断的是 `whiteboard` 反向认 `web` 的 7 处（网页内容小部件需要 WebController）。
8. **`ui/` ↔ `whiteboard/`（最大的一组，40/46）**：拆成“画板拥有视图，调色板经
   接口回调解”。这组动作量大，且都在交互热路径上，必须单独排期、单独回归。
9. **`core/`**（最后）：core 是装配点，`core` → `ui`/`whiteboard` 的边要换成接口 +
   构造期注入，或者把 `ApplicationController` 上提到 `app/`，让 core 退化成纯服务集合。

`docs/code-suspicions.md` 记的是另一类问题（可疑行为），与这里的依赖债分开。

## 六、模块依赖边全表（现状态，供拆环时核对）

出边（本模块 include 别的模块；`x → y : n` 表示 x 里有 n 处 `#include "y/…"`，
由 `src/` 全量扫描得到）：

```text
api         -> content:1, core:6, model:1, settings:1, utils:2, web:1, whiteboard:8
app         -> core:1, platform:1, settings:1, utils:1
content     -> core:44, model:22, platform:3, settings:12, ui:2, utils:22, whiteboard:50
core        -> content:15, desktop:2, model:11, platform:7, recording:2, settings:15,
               ui:11, utils:7, web:2, whiteboard:33
desktop     -> core:7, platform:5, settings:3, ui:10, web:1, whiteboard:9
model       -> content:5, core:13, platform:1, settings:4, ui:8, utils:5, whiteboard:6
platform    -> core:3, settings:1, ui:4, utils:1
recording   -> core:7, platform:1, settings:3, ui:4, utils:3, web:2, whiteboard:4
settings    -> utils:3
ui          -> api:1, content:2, core:38, model:9, platform:6, settings:15, utils:8,
               whiteboard:40
utils       -> （无出边）
web         -> core:17, platform:3, settings:5, ui:4, utils:2, whiteboard:5
whiteboard  -> api:2, content:3, core:65, model:9, platform:7, settings:20, ui:46,
               utils:42, web:7
```

`core` 还另有 1 处 `#include "third/singleapp/…"`（见第三节），那是内联第三方而不是模块。
重跑这张表不需要脚本，一条 grep 就够：

```text
grep -rhoE '#include[[:space:]]+"[a-z]+/' src/<模块> | sed -E 's/.*"([a-z]+)\/.*/\1/' \
  | sort | uniq -c
```
