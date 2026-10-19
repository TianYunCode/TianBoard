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
  ├─ core/         应用与各类服务/控制器（Application、ApplicationController、Settings、
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
  └─ utils/        纯工具：StringUtils、GeometryUtils、Version、FileSystemUtils、
                   Globals、CoreGraphicsScene、CryptoUtils、Enums.h
third/             内联第三方：quazip+zlib、openssl 静态库、qtsingleapplication
res/               images / etc / library / customizations / style / i18n(.ts) / qrc
tests/             Qt Test 用例（7 个）
packaging/         Inno Setup 脚本与打包脚本
scripts/           build.ps1 / format.ps1
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
`core/resource.h` → `app/`（只被 TianBoard.rc 用）。

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

运行期目录约定：`PlatformUtils::applicationResourcesDirectory()` 返回可执行文件所在
目录，因此 `etc/`、`library/`、`customizations/`、`Language/` 必须与 exe 同级，由
`src/app/CMakeLists.txt` 的 POST_BUILD 步骤铺好；`tst_smoke` 会显式校验这一点。

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
  `Application.h` 被 core 之外的全部 11 个模块直接 include；内联第三方按它原本的仓库根
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

## 四、为什么现在是 OBJECT 库而不是 STATIC 库

目标是每个模块一个真正的静态库、依赖无环、链接顺序明确。**这一步做不到**：代码里
存在 23 组双向 include（下一节全表 + 台账）。STATIC 库遇到环需要 `--start-group`
之类的手段，等于把问题藏进链接参数里；OBJECT 库把所有 .obj 并进同一个可执行，环在
链接期不成问题，于是本次调整可以只拿到两样确定性的东西——**每模块的显式文件清单**
和**每模块的 include 白名单**——把拆环留到后续阶段。

换成 STATIC 的前置条件：该模块的出边全部落在比它低的层（下表），且没有对上的
include。

## 五、依赖债：现存的双向环

下面是 `src/` 全量扫描出来的直接互相 include（数字 = 各方向的 include 处数，
含头文件里的；`app/` 作为装配点不计）。

| 环 | 边数 | 典型出处 |
|---|---|---|
| `core` ↔ `whiteboard` | 34 / 86 | `core/ApplicationController.cpp` 认 BoardController；`whiteboard/*.cpp` 认 Settings/Application |
| `core` ↔ `ui` | 11 / 57 | `core/Application.cpp`、`PersistenceManager.cpp`、`DownloadManager.cpp` 直接认 `ui/MainWindow.h`；`ui/*.cpp` 反过来认 Application/Settings |
| `content` ↔ `core` | 56 / 15 | `content/ExportAdaptor.cpp` 认 core；`core/PersistenceManager.cpp` 认 content 的读写器 |
| `content` ↔ `whiteboard` | 50 / 3 | `whiteboard/GraphicsWidgetItem.*` 反向认 content |
| `ui` ↔ `whiteboard` | 42 / 47 | 调色板与画板互相认 |
| `core` ↔ `model` | 10 / 17 | `core/DocumentManager` ↔ `model/DocumentController` |
| `model` ↔ `ui` | 8 / 11 | `model/DocumentController.cpp` 直接建 ThumbnailView/MainWindow |
| `model` ↔ `whiteboard` | 6 / 9 | 文档页与图形场景互认 |
| `model` ↔ `content` | 22 / 5 | `model/DocumentContainer.cpp` 直接调 ThumbnailAdaptor |
| `core` ↔ `web` | 2 / 24 | `core/Application.cpp` 认 WebController |
| `core` ↔ `desktop` | 2 / 10 | `core/ApplicationController.cpp` 认 DesktopPalette |
| `core` ↔ `recording` | 2 / 10 | ApplicationController/PreferencesController 认 RecordingController |
| `core` ↔ `platform` | 6 / 4 | `platform/KeyboardPalette.cpp` 认 Settings/Application |
| `core` ↔ `utils` | 11 / 2 | `utils/CryptoUtils.h`、`utils/FileSystemUtils.cpp` 反向认 `core/Application.h` |
| `utils` ↔ `whiteboard` | 2 / 44 | `utils/CoreGraphicsScene.cpp` 认两个图形项 |
| `api` ↔ `whiteboard` | 8 / 2 | `whiteboard/GraphicsWidgetItem.cpp` 反向认 api 门面 |
| `content` ↔ `ui` | 2 / 4 | `content/ExportAdaptor.cpp` 弹 MessagesDialog/MainWindow |
| `platform` ↔ `ui` | 4 / 6 | `platform/KeyboardPalette.h` 认 `ui/ActionPalette.h` |
| `platform` ↔ `whiteboard` | 1 / 8 | `platform/WindowCapture_win.cpp` 认 BoardView |
| `platform` ↔ `desktop` | 5 / 2 | WindowCapture_win 认 DesktopPalette/AnnotationController |
| `desktop` ↔ `whiteboard` | 9 / 2 | BoardView 反向认 DesktopAnnotationController |
| `recording` ↔ `whiteboard` | 4 / 2 | BoardController 反向认 RecordingController |
| `web` ↔ `whiteboard` | 6 / 7 | 互相认控制器与图形项 |

### 拆环顺序（从叶子往根拆）

按下面这个顺序拆，每一步都能把某个模块的出边降到“只指向更低的层”，然后就该模块
从 `OBJECT` 改成 `STATIC`。

1. **`utils/`**（被 10 个模块依赖，它一脏全线脏；只有 2 条向上的边）
   * `utils/CryptoUtils.h`、`utils/FileSystemUtils.cpp` 里 `core/Application.h` 是
     “顺手拿全局单例要设置/路径”——把需要的值作为参数传进来。
   * `utils/CoreGraphicsScene.cpp` 认 `whiteboard/GraphicsMediaItem.h`、
     `GraphicsGroupContainerItem.h`——这个类本来是画板侧的场景基类，长期归属应是
     `whiteboard/`，搬走即可（搬迁属于行为不变的第二步）。
2. **`api/`**：断开 `whiteboard/GraphicsWidgetItem.cpp` → api 的反向边（小部件经
   QObject 消息桥调用画板能力，改为画板侧注册回调/接口，api 只认下层的 whiteboard）。
3. **`model/`**：断开 `model` → `ui`（DocumentController 直接实例化缩略图视图）与
   `model` → `content`（DocumentContainer 直接调 ThumbnailAdaptor）。两处都要抽接口：
   `IThumbnailProvider`、`IDocumentStore`，由装配层（core/app）注入。
4. **`platform/`**：`KeyboardPalette` 认 `ui/MainWindow`、`ui/ActionPalette`；
   `WindowCapture_win` 认 `desktop/*` 与 `whiteboard/BoardView`。平台层应当只暴露
   “捕获一块画面”“弹一个键盘”，由调用方实现宿主接口。
5. **`content/`**：主要脏边是 `content/ExportAdaptor.cpp` → `ui/MainWindow`、
   `ui/MessagesDialog`（导出进度与错误框）。导出器改为回报状态，界面自己决定怎么显示。
6. **`desktop/` + `recording/` + `web/`**：把 `BoardView`/`BoardController` 对这三者的
   直接调用换成控制器之间的事件转发（白板只发“进入桌面批注”“开始录制”这类请求）。
7. **`ui/` ↔ `whiteboard/`（最大的一组，42/47）**：拆成“画板拥有视图，调色板经
   接口回调解”。这组动作量大，且都在交互热路径上，必须单独排期、单独回归。
8. **`core/`**（最后）：core 是装配点，`core` → `ui`/`whiteboard` 的边要换成接口 +
   构造期注入，或者把 `ApplicationController` 上提到 `app/`，让 core 退化成纯服务集合。

`docs/code-suspicions.md` 记的是另一类问题（可疑行为），与这里的依赖债分开。

## 六、模块依赖边全表（现状态，供拆环时核对）

出边（本模块 include 别的模块）：

```text
api         -> content, core, model, utils, web, whiteboard
app         -> core, platform, utils
content     -> core, model, platform, ui, utils, whiteboard
core        -> content, desktop, model, platform, recording, ui, utils, web, whiteboard
desktop     -> core, platform, ui, web, whiteboard
model       -> content, core, platform, ui, utils, whiteboard
platform    -> core, desktop, ui, utils, whiteboard
recording   -> core, platform, ui, utils, web, whiteboard
ui          -> api, content, core, model, platform, utils, whiteboard
utils       -> core, whiteboard
web         -> core, platform, ui, utils, whiteboard
whiteboard  -> api, content, core, desktop, model, platform, recording, ui, utils, web
```
