# 代码疑点台账

改造过程中发现、但**刻意没有动手**的问题集中记在这里。

为什么不当场修：本次提交是纯结构调整（搬目录 + include 前缀），铁律是“行为等价”。
一次提交里既改结构又改行为，出了问题分不清是哪一半引起的，回滚也只能整批回滚。
所以凡是“看着像缺陷、但修它会改变运行期行为”的，先记账，由人决定是否单独开一批修复。
依赖结构的问题在 [架构说明](architecture.md) 的“依赖债”一节，不在这里重复。

---

## 一、空实现与半截实现（有调用点，用户点了没反应）

### 1. 文档标签页的 剪切/复制/粘贴 是三个空函数
- **位置**：`src/model/DocumentController.cpp:3143-3160`（`cut()`/`copy()`/`paste()`
  三处，每处一句 `// TODO - implemented me`）
- **调用点是活的**：`src/core/ApplicationController.cpp:622/626/630`（cut）、
  `642/646/650`（copy）、`662/666/670`（paste）按当前模式分派，文档模式走的就是这三个空函数。
- **现象**：在文档界面按 Ctrl+X/C/V 或菜单项，没有任何效果、也没有提示。
- **建议**：要么实现（缩略图选中项的删除/复制/新建），要么让动作在文档模式下不可用
  （`setEnabled(false)`），别留“看着能点”的死动作。

### 2. `WebWidgetBoardAPI` 两个方法收了参数不用
- **位置**：`src/api/WebWidgetBoardAPI.cpp:427-441`
- **现象**：`usedMethods(QStringList methods)` 里 `foreach` 循环体是空的；
  `response(bool correct)` 立刻 `Q_UNUSED(correct)`，注释都是 `// TODO: Implement this method`。
  小部件回报“我用过哪些方法 / 调用成功与否”这两条通道目前没有下游。
- **建议**：确认这是废弃协议还是待实现；废弃就删掉声明与调用点，待实现就补测试。

### 3. 网页抓取（YouTube 按钮修复）停在半路
- **位置**：`src/web/WebController.cpp:140`、`359`（`// TODO : Uncomment the next line to
  continue the youtube button bugfix`）、`582`（`// TODO : Implement this`）、
  `170`/`587`（`// TODO: check the hidden iFrame`）、`595`（询问用户抓取哪个内容的对话框没做）
- **现象**：抓取路径上有多处“被注释掉的正确写法”，等于当前行为是绕过问题的版本。
- **建议**：这些注释是当年绕 bug 的现场记录，动它要拿真实页面回归，单独排期。

### 4. 其它 `TODO`（30 处里的其余部分，按模块列，不再逐条展开）
- `src/content/CFFSubsetAdaptor.cpp:101`、`1140`：CFF 子集读写有两段没实现
  （“从 parseIwbElement() 的结果建组”）。
- `src/content/ImportCFF.cpp:71/86/215`：导入 .iwb 时的临时目录与内容转换留 TODO ——
  与“旧文档不必保持可读”的既定决策一致，属于可以不填的坑。
- `src/core/PersistenceManager.cpp:178/238/684`：三处 `claudio remove this hack`
  的双击打开文件/落盘对象处理，是上游遗留的临时分支。
- `src/whiteboard/GraphicsItemGroupUndoCommand.cpp:34`：`//Work around. TODO determine why
  does Qt call the redo function on pushing to undo` —— undo 栈语义可疑，动它要写回归。
- `src/whiteboard/GraphicsTextItem.cpp:26`、`GraphicsStrokesGroup.cpp:39`、
  `GraphicsDelegateFrame.cpp:809`、`GraphicsScene.cpp:691`（笔迹平滑“TODO LATER”）、
  `GraphicsStroke.cpp:181`：绘制热路径上的算法欠账。
- `src/whiteboard/FeaturesController.h:157`：`//TODO make less complicated for betteer maintainence`
  （原文如此），`FeaturesController.cpp:67`：`favoritePath` 的取法待定。
- `src/desktop/DesktopAnnotationController.cpp:869`、`src/desktop/DesktopPalette.h:30`：
  窗口管理器探测没做；后者是一行被注释掉的 `#ifdef Q_OS_LINUX`（“在 Linux 上会报错，先注掉”）。
- `src/ui/MainWindow.cpp:1252`：导出时文件对话框的 `// HACK`（上游注释说明为什么不能改成
  常规写法），别顺手“清理”。

## 二、指向不存在文件的死引用（注释里，编译不参与）

搬迁时**没有**改这些行的模块前缀，因为它们指向的文件在这个仓库里从来不存在，
改成新模块名反而更像“有效路径”。要清就直接删注释，属于行为无关的死代码清理，单独一批做。

- `src/web/WebController.cpp:17` 与 `src/whiteboard/BoardPaletteManager.cpp:32`：
  `//#include "network/ServerXMLHttpRequest.h"`（`network/` 已并入 `core/`，该头文件不存在）
- `src/web/BrowserWindow.cpp:21`：`//#include "pdf/UBWebPluginPDFWidget.h"`
  （Qt6 已无 NPAPI 插件接口，上一轮清理已删掉 `Web_.h` 里同名的一段）
- `src/content/CFFSubsetAdaptor.cpp:35`、`src/content/SvgSubsetAdaptor.cpp:53`：
  `//#include "qtlogger.h"`（Qt Solutions 的日志头，仓库里没有）
- `src/content/ExportFullPDF.cpp:25-27`：`//#include <Merger.h>`、`<Exception.h>`、
  `<Transformation.h>` 与 `//using namespace merge_lib;`（某个 PDF 合并库的残骸）
- `src/web/TabWidget.cpp:4`：`//#include "ui/History.h"`（指向上游 Qt5 时代的模块名，已按
  本次规矩统一成 `模块/文件.h` 写法，文件本身仍不存在）

## 三、命名与形态遗留（下一阶段处理，与结构调整分开）

- 仍带旧品牌前缀的宏：`src/utils/WB.h` 的 `WB_MAX_ZOOM`、`src/whiteboard/GraphicsItemDelegate.h`
  的 `WB_FREE_CONTROL(Object, Scene)`。
- 类名前缀本轮没换新的：既定决策是“去掉 `WB`/`UB` 前缀且**不换新的**”，所以新代码也别加
  `TianBoard::` 这类项目命名空间（见 [coding-style.md](coding-style.md) 第二章）。
- 名字与真实语义不符、但**故意没改**的：`CFF*`（.tbz 内部格式，改到一半）、
  `DemoLibrary`。改名会牵动字符串、文件名与 moc，必须单独一批。
  已经改完的：`Recording*`（类名、动作名、设置项与 ini 节名都是录屏语义）、
  `WebWidget*`（原 `W3CWidget*`/`Widget*API` 门面与 `GraphicsW3CWidgetItem` 图形项）。
- `src/platform/WindowCapture.h`：有 `Q_OBJECT` 但没有同名 `.cpp`（实现是
  `WindowCapture_win.cpp`）。它**必须**继续显式列在 `src/platform/CMakeLists.txt` 里，
  删掉那一行 = moc 不参与 = 捕获窗口信号槽静默失效。同类风险见
  [coding-style.md](coding-style.md) 第四章。
- `src/recording/WindowsMediaFile.h:1` 的 include guard 是 `#ifndef inteface`（拼错的
  “interface”），既不合 `TIANBOARD_*_H` 的形，也撞不上任何真实宏 —— 目前无害，
  但它是全仓 173 个规整 guard 之外的孤例，改它要单独提交。
- `src/app/resource.h` 不被任何 C++ 引用，只被 `TianBoard.rc` include；
  `.rc` 是 UTF-16 且 `core/resource.h` 已随文件搬到 `src/app/`，改这个文件要用工具，
  别用文本编辑器（编码一变整个文件就是二进制乱码）。

## 四、构建与仓库层面的坑（记下来免得踩回去）

- **产物目录必须是 `${CMAKE_BINARY_DIR}/bin/<Config>/`，不能钉死到单一 `build/bin/`**。
  静态库属于 ARCHIVE 输出，多配置生成器下 Debug 与 Release 的 `wbz_quazip.lib` 同名，
  钉死就互相覆盖：实测先构建 Release 再构建 Debug，Debug 会链到 Release 的 `.obj`，
  报 LNK1319 + 一串 LNK2038 RuntimeLibrary/`_ITERATOR_DEBUG_LEVEL` 不匹配。
  另外路径要相对 `CMAKE_BINARY_DIR`，否则两个隔离构建目录共用同一处照样串。
  `tst_smoke` 通过 `TIANBOARD_EXE`（`$<TARGET_FILE:TianBoard>`）拿路径，
  POST_BUILD 铺的 `etc/ library/ customizations/ Language/` 经 `$<TARGET_FILE_DIR>` 走，
  所以分目录不需要改别处。
- **模块 CMakeLists 是显式文件清单**（没有 `file(GLOB_RECURSE)`）：新文件不登记就完全不进
  构建，这是刻意的（资源确定性），但也意味着“加了文件没加 CMake”不会再有隐蔽的
  静默失败——配置期或链接期就会报，别为了省事把 GLOB 加回来。
- **`CMAKE_INCLUDE_CURRENT_DIR` 是关的**：裸写 `#include "MainWindow.h"` 直接编译失败。
  已有 2 处历史写法这次一并改成规范形式（`#include <core/WB.h>` → `#include "utils/WB.h"`
  共 2 行；`#include "windowsmedia/X.h"` 共 2 行），除此之外没有第三种写法残留（已 grep 核对）。
- **`.gitattributes` 里 `* -text`**：仓库里 CRLF/LF 混用，禁止自动换行转换；任何“顺手
  统一换行符”的操作都会产出几千行的伪 diff。批量文本替换用逐字节往返的方式（本次用
  Python，非 UTF-8 的字节按 latin-1 原样往返，只有 `src/app/resource.h` 一个文件命中）。
- **`.clang-format` 与 `scripts/format.ps1` 已就位，但既有文件一个都没格式化**。
  全量重排要单独排期、按模块分批，不与功能改动混批。
- `packaging/make-installer.ps1` 找不到 `ISCC.exe` 时**明确失败**（不是跳过），
  并对 stage 里的必需文件做硬校验；`windeployqt` 的模块参数按 Qt6 的名字写
  （`-webenginewidgets`，Qt6 没有 `-webengine` 这个选项）。脚本尚未在装了 Inno Setup
  的机器上实跑过，第一次跑要人工看一遍 stage 清单。
