# 代码疑点台账

改造过程中发现、但**刻意没有动手**的问题集中记在这里。

为什么不当场修：一次提交里既改结构又改行为，出了问题分不清是哪一半引起的，
回滚也只能整批回滚。所以凡是“看着像缺陷、但修它会改变运行期行为”或者“需要产品
决策”的，先记账。依赖结构的问题在 [架构说明](architecture.md) 的“依赖债”一节，
不在这里重复。已经修掉的不再列在这里，去 git log 看对应的提交说明。

---

## 一、空实现与半截实现（有调用点，用户点了没反应）

### 1. 文档标签页的 剪切/复制/粘贴 是三个空函数
- **位置**：`src/model/DocumentController.cpp:3125/3131/3137`（`cut()`/`copy()`/`paste()`
  三处，每处一句 `// TODO - implemented me`）
- **调用点是活的**：`src/core/ApplicationController.cpp:622/626/630`（cut）、
  `642/646/650`（copy）、`662/666/670`（paste）按当前模式分派，文档模式走的就是这三个空函数。
- **现象**：在文档界面按 Ctrl+X/C/V 或菜单项，没有任何效果、也没有提示。
- **建议**：要么实现（缩略图选中项的删除/复制/新建），要么让动作在文档模式下不可用
  （`setEnabled(false)`），别留“看着能点”的死动作。

### 2. `WebWidgetBoardAPI` 两个方法收了参数不用
- **位置**：`src/api/WebWidgetBoardAPI.cpp:427`（`usedMethods`）、`436`（`response`）
- **现象**：`usedMethods(QStringList methods)` 里 `foreach` 循环体是空的；
  `response(bool correct)` 立刻 `Q_UNUSED(correct)`，注释都是 `// TODO: Implement this method`。
  小部件回报“我用过哪些方法 / 调用成功与否”这两条通道目前没有下游。
- **建议**：确认这是废弃协议还是待实现；废弃就删掉声明与调用点，待实现就补测试。

### 3. Flash / NPAPI 小部件包装还在链路里，运行时却放不出东西
- **位置**：`src/whiteboard/GraphicsWidgetItem.cpp:1102/1133/1152`（`freezedWidgetWrapper.html`、
  按 `npapi-wrapper*` 前缀挑包装页、读 `etc/npapi-wrapper.config.xml`）与
  `src/content/IwbSubsetAdaptor.cpp:904`（`createNPAPIWrapperInDir(..., "application/x-shockwave-flash", ...)`）
- **现象**：Qt6 WebEngine 没有 NPAPI/Flash 插件接口，这套包装页只能显示一张“无法播放”的
  静态图；但代码路径是活的，旧文档里的 flash 条目会走到这里。
- **建议**：要么把 flash 条目明确降级成“不支持的媒体”并提示用户，要么连同
  `res/etc/npapi-wrapper.*`、`freezedWidgetWrapper.html` 一起删。属于产品可见的取舍。

### 4. 其它 `TODO`（全仓 26 处里的其余部分，按模块列，不再逐条展开）
- `src/content/IwbSubsetAdaptor.cpp:101`、`1140`：子集读写有两段没实现
  （“从 parseIwbElement() 的结果建组”）。
- `src/content/ImportIwb.cpp:71/86/215`：导入 .iwb 时的临时目录与内容转换留 TODO ——
  与“旧文档不必保持可读”的既定决策一致，属于可以不填的坑。
- `src/core/PersistenceManager.cpp:177`（双击打开文档包）与 `:238` 附近（落盘对象处理）：
  上游遗留的临时分支；`src/core/Settings.cpp` 里还有一处 `//TODO: user-settable?`。
- `src/whiteboard/GraphicsItemGroupUndoCommand.cpp:34`：`//Work around. TODO determine why
  does Qt call the redo function on pushing to undo` —— undo 栈语义可疑，动它要写回归。
- `src/whiteboard/GraphicsTextItem.cpp:26`、`GraphicsStrokesGroup.cpp:39`、
  `GraphicsDelegateFrame.cpp:809`、`GraphicsScene.cpp:691`（笔迹平滑“TODO LATER”）、
  `GraphicsStroke.cpp:181`：绘制热路径上的算法欠账。
- `src/whiteboard/FeaturesController.h:156`：`//TODO make less complicated for betteer maintainence`
  （原文如此），`FeaturesController.cpp`：`favoritePath` 的取法待定。
- `src/desktop/DesktopAnnotationController.cpp`（窗口管理器探测没做）、`src/desktop/DesktopPalette.h:29`：
  一行被注释掉的 `#ifdef Q_OS_LINUX`（“在 Linux 上放开就会报错”）。
- `src/ui/MainWindow.cpp:1193`：导出时文件对话框的 `// HACK`（上游注释说明了为什么不能改成
  常规写法），别顺手“清理”。

## 二、指向不存在文件的死引用（注释里，编译不参与）

这些行指向的文件在本仓库里从来不存在；把它们改成新模块名反而更像“有效路径”，
所以要么整行删掉，要么保持原样。属于行为无关的死代码清理，单独一批做。

- `src/whiteboard/BoardPaletteManager.cpp:32`：`//#include "network/ServerXMLHttpRequest.h"`
  （`network/` 已并入 `core/`，该头文件不存在）
- `src/content/IwbSubsetAdaptor.cpp:35`、`src/content/SvgSubsetAdaptor.cpp:53`：
  `//#include "qtlogger.h"`（Qt Solutions 的日志头，仓库里没有）
- `src/content/ExportFullPDF.cpp:25-27`：`//#include <Merger.h>`、`<Exception.h>`、
  `<Transformation.h>` 与 `//using namespace merge_lib;`（某个 PDF 合并库的残骸）
- `src/web/TabWidget.cpp:4`：`//#include "ui/History.h"`（模块名是上游 Qt5 时代的，
  文件本身不存在）
- `src/web/EditTableView.cpp:2`：`//#include <QtWidgets/QKeyEvent>`
- `src/web/WebTrapWebView.cpp:5`：`//#include <QWebHitTestResult>`（Qt6 已无该类）

## 三、需要产品决策，不该由重构顺手改掉

- **导出 PDF/图片的 DPI 取的是主屏，不是文档所在屏**。落点：
  `src/core/VirtualDesktop.cpp:201-211`（`physicalDpiX/Y` 里写死
  `QGuiApplication::primaryScreen()`），调用方 `src/content/ExportPDF.cpp:65`、
  `src/content/ExportFullPDF.cpp:39`、`src/content/IwbSubsetAdaptor.cpp`。
  多屏且两屏物理 DPI 不同时代入的是错的 DPI。改它会让写进文档的 `pageDpi` 变化，
  属于会改变产物内容的决定，要由人确认后再动。
- **语言只放了中/英**。`LanguageManager` 的机制与 `tst_language` 已就位，加第三种语言
  只要补 `res/i18n/TianBoard_<lang>.ts` 并产出 `Language/TianBoard_<lang>.qm`；
  在有人翻译之前不能凭空往菜单里加语种。
- **`.clang-format` 全量重排仍未执行**（见第五节倒数第二条），理由不变。

## 四、命名与形态遗留

- **`src/app/resource.h` 是全仓唯一保留 `#ifndef` 守卫的头**（`src/` 下 172 个头文件里
  171 个已经是 `#pragma once`）：`rc.exe` 不认识 `#pragma once`，把它改掉会让图标与
  版本信息静默丢失。另外 `TianBoard.rc` 是 UTF-16，改这两个文件必须用字节级工具，
  别用文本编辑器——编码一变整个文件就是二进制乱码，而 `rc.exe` 仍可能返回 0，
  只在产出的 `.res` 体积上露馅。
- `src/recording/WindowsMediaFile.h:5` 的 `#ifndef inteface` **不是** include guard
  （守卫已经是 `#pragma once`），它包住的是 `#define interface struct` 这个 Windows SDK
  老写法。拼错的 `inteface` 目前无害，但别把它当残留守卫删掉。
- **`IwbConstants.h` 里还有 C 风格遗产**：第 3 行 `#define PI 3.1415926535`（污染宏命名
  空间，且与 `M_PI` 重复）、`FEATURE_*` 一组常量、`eAncestors`/`eCopyMode`/`eAddItemMode`
  这类 `e` 前缀 + 值前缀不一致的枚举。改成 `constexpr` 与 `enum class` 是纯改名，
  但要和 `svgIWBNS`/`tIWB*` 一组常量一起做，别混进功能提交。
  停靠面板那四组 `eWBDock*` 已经改成 `enum class`，见 git log。
- **`MimeType::AppleWidget`** 自己的注释写着 “mime type invented by us :-()”，
  却和 `GraphicsWidgetItem` 一路共享 “widget” 语义。名字里的 Apple 商标与实现无关，
  但改它要连写进 `.tbz` 的 mime 字符串一起看，属于协议层，先记。
- 类名前缀本轮没换新的：既定决策是“去掉 `WB`/`UB` 前缀且**不换新的**”，所以新代码也别加
  `TianBoard::` 这类项目命名空间（见 [coding-style.md](coding-style.md) 第二章）。
  `src/` 下已 grep 核对：没有 `UB*`/`ub*`/`WB_*`/`eWB*` 标识符残留。
- `src/platform/WindowCapture.h`：有 `Q_OBJECT` 但没有同名 `.cpp`（实现是
  `WindowCapture_win.cpp`）。它**必须**继续显式列在 `src/platform/CMakeLists.txt` 里，
  删掉那一行 = moc 不参与 = 捕获窗口信号槽静默失效。同类风险见
  [coding-style.md](coding-style.md) 第四章。

## 五、构建与仓库层面的坑（记下来免得踩回去）

- **产物目录必须是 `${CMAKE_BINARY_DIR}/bin/<Config>/`，不能钉死到单一 `build/bin/`**。
  静态库属于 ARCHIVE 输出，多配置生成器下 Debug 与 Release 的 `wbz_quazip.lib` 同名，
  钉死就互相覆盖：实测先构建 Release 再构建 Debug，Debug 会链到 Release 的 `.obj`，
  报 LNK1319 + 一串 LNK2038 RuntimeLibrary/`_ITERATOR_DEBUG_LEVEL` 不匹配。
  另外路径要相对 `CMAKE_BINARY_DIR`，否则两个隔离构建目录共用同一处照样串。
  `tst_smoke` 通过 `TIANBOARD_EXE`（`$<TARGET_FILE:TianBoard>`）拿路径，
  POST_BUILD 铺的 `etc/ library/ customizations/ Language/` 经 `$<TARGET_FILE_DIR>` 走，
  所以分目录不需要改别处。
- **改了源文件清单必须先重新 configure 再构建**。MSBuild 生成器不会因为清单变化自动重跑
  配置，`AutogenInfo.json` 里留着已删除文件的条目时报错只有一句
  “Info error in info file”，看不出根因。
- **QtSvg 对单条 path 的坐标量有硬上限**，超限只是打印
  `Invalid path data; path truncated.` 然后画出半个图形（不报错、不中止）。
  随包的 `res/library/pictures/Worldmap_wdb_combined.svg` 原来就踩了这个坑，
  现在由 `tst_assets::svgAssetsPaintWithoutTruncation` 兜住：接管消息处理器，把
  `library/` 与 `images/` 下每个 SVG 真渲染一遍，既查告警也查“有没有画出像素”。
  注意告警是在 `QSvgRenderer` **构造期**发出的，handler 只包住 `render()` 会全部漏掉。
- **模块 CMakeLists 是显式文件清单**（没有 `file(GLOB_RECURSE)`）：新文件不登记就完全不进
  构建，这是刻意的（资源确定性），但也意味着“加了文件没加 CMake”不会再有隐蔽的
  静默失败——配置期或链接期就会报，别为了省事把 GLOB 加回来。
- **`CMAKE_INCLUDE_CURRENT_DIR` 是关的**：裸写 `#include "MainWindow.h"` 直接编译失败。
  历史写法已全部改成 `模块/文件.h` 形式（已 grep 核对）。
- **`.gitattributes` 里 `* -text`**：仓库里 CRLF/LF 混用，禁止自动换行转换；任何“顺手
  统一换行符”的操作都会产出几千行的伪 diff。批量文本替换要逐字节往返（`<:raw`/`>:raw`），
  而且**替换串里不能出现 `\r?\n` 这种写法**——`\r` 会被当成字节的 CR 插进文件，把一次
  改名变成整行损坏（本轮踩过：多余 CR 加上 `$1` 没展开，编译器只报“未声明的标识符”）。
  同理，`s{...}{...}` 的定界符会和替换串里的花括号打架，替换里带 lambda 时要换定界符。
- **Qt 对象名与样式表是一对**：`res/etc/TianBoard.css` 用 `QToolButton#buttonGroupLeft`
  这类 `#id` 选择器，改 `setObjectName()` 必须同步改 CSS，否则样式静默脱落，
  `tst_assets` 管不到这一层。
- **`.clang-format` 与 `scripts/format.ps1` 已就位，但既有文件一个都没格式化**。
  全量重排要单独排期、按模块分批，不与功能改动混批。
- `packaging/make-installer.ps1` 找不到 `ISCC.exe` 时**明确失败**（不是跳过），
  并对 stage 里的必需文件做硬校验；`windeployqt` 的模块参数按 Qt6 的名字写
  （`-webenginewidgets`，Qt6 没有 `-webengine` 这个选项）。脚本尚未在装了 Inno Setup
  的机器上实跑过，第一次跑要人工看一遍 stage 清单。
