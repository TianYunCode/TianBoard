# 代码疑点台账

改造过程中发现、但**刻意没有动手**的问题集中记在这里。

行号会随改动漂移，认不出某一行时按后面的函数名找，别只信编号。

为什么不当场修：一次提交里既改结构又改行为，出了问题分不清是哪一半引起的，
回滚也只能整批回滚。所以凡是“看着像缺陷、但修它会改变运行期行为”或者“需要产品
决策”的，先记账。依赖结构的问题在 [架构说明](architecture.md) 的“依赖债”一节，
不在这里重复。已经修掉的不再列在这里，去 git log 看对应的提交说明。

---

## 一、空实现与半截实现（有调用点，用户点了没反应）

### 1. `WebWidgetBoardAPI` 两个方法收了参数不用
- **位置**：`src/api/WebWidgetBoardAPI.cpp:371`（`usedMethods`）、`377`（`response`）
- **现象**：`usedMethods(QStringList methods)` 里 `foreach` 循环体是空的；
  `response(bool correct)` 立刻 `Q_UNUSED(correct)`，注释都是 `// TODO: Implement this method`。
  小部件回报“我用过哪些方法 / 调用成功与否”这两条通道目前没有下游。
- **建议**：确认这是废弃协议还是待实现；废弃就删掉声明与调用点，待实现就补测试。

### 2. Flash / NPAPI 小部件包装还在链路里，运行时却放不出东西
- **位置**：`src/whiteboard/GraphicsWidgetItem.cpp:862/880/899`（`freezedWidgetWrapper.html`、
  按 `npapi-wrapper*` 前缀挑包装页、读 `etc/npapi-wrapper.config.xml`）与
  `src/content/IwbSubsetAdaptor.cpp:891`（`createNPAPIWrapperInDir(flashPath, ..., "application/x-shockwave-flash", ...)`）
- **现象**：Qt6 WebEngine 没有 NPAPI/Flash 插件接口，这套包装页只能显示一张“无法播放”的
  静态图；但代码路径是活的，旧文档里的 flash 条目会走到这里。
- **建议**：要么把 flash 条目明确降级成“不支持的媒体”并提示用户，要么连同
  `res/etc/npapi-wrapper.*`、`freezedWidgetWrapper.html` 一起删。属于产品可见的取舍。

### 3. 小部件的 JS 桥与加载回调在 Qt6 里没有接回来
- **位置**：`src/whiteboard/GraphicsWidgetItem.cpp`（`mInitialLoadDone`、`loadMainHtml()`、
  `paint()` 的“加载中”分支）与 `src/ui/ToolWidget.cpp::initialize()`（只 `load()`，页面里拿不到任何 API 对象）
- **现象**：Qt5 时代这些回调挂在 `page()->mainFrame()` 的
  `initialLayoutCompleted`/`geometryChangeRequested`/`linkClicked` 等信号上，Qt6 WebEngine
  没有对应信号（要换 `QWebEnginePage::webChannel` + 页面侧 `qwebchannel.js`，小部件自己的
  脚本不认）。所以 `mInitialLoadDone` 没有任何路径置真，`paint()` 永远走“未加载”分支，
  每个小部件图元左上角都挂着一个空的气泡底。小部件仍能显示
  （`ToolWidget` 的预览视图是真的），但脚本调不到画板能力。
- **这一轮删掉的**：上面那四个从没被连接过的槽（`mainFrameLoadFinished`、
  `initialLayoutCompleted`、`geometryChangeRequested`、`onLinkClicked`）连同
  `hasLoadedSuccessfully()`、`removeScript()`、`sendJSEnterEvent()`/`sendJSLeaveEvent()`
  ——它们的函数体要么整块被注释掉，要么只把入参丢给 `Q_UNUSED`，调用点拿到的永远是空效果。
  桥要恢复时从 git 历史里取回即可，本条记录说明该接哪些信号。
- **上一轮删掉的死半截**：`javaScriptWindowObjectCleared()`（基类与
  `GraphicsWebWidgetItem` 两处）、`injectInlineJavaScript()` 连同它读的
  `widget-inline-js` 目录扫描（`ApplicationController::widgetInlineJavaScripts()`，
  部署里根本没有这个目录）、只被那条死链 new 出来的 `WebWidgetAPI`/
  `WebWidgetPreferenceAPI` 两个门面，以及 `FeaturesWebWidget` 里从不使用的 `boardApi`。
- **已顺手修掉的故障**：拖放落到小部件上时门面是空指针——现在经 `whiteboard/IBoardDropHandler`
  接口按需创建（实现由 `app/main.cpp` 装上，见 docs/architecture.md 第五节）；
  `ToolWidget::javaScriptWindowObjectCleared()` 每次切活动场景 `new` 一个
  `WebWidgetBoardAPI`（父对象给场景，无人接收）并重复连接
  `DownloadManager::downloadFinished` —— 泄漏 + 重复回调，已连同该槽一起删除。
- **建议**：真要恢复脚本桥，需要一次独立改造：定下 QWebChannel 的对象名与 JS 侧包装，
  并把 `WebWidget*API` 剩下的门面的 `Q_PROPERTY`/`SCRIPTABLE` 换成 `Q_INVOKABLE`；
  同时把 `paint()` 那个常驻气泡改成“确实没有内容时才提示”。

### 4. 「捕获网页」(Web Trap) 在 Qt6 下整条命中测试链是空的
- **位置**：`src/web/WebTrapWebView.cpp`（`trapElementAtPos()` 现在是空实现，函数里用中文
  注明了为什么；`highliteElementAtPos()` 依赖 `page()->webChannel()`，而全仓没有任何
  `setWebChannel` 调用）、`src/web/TabWidget.cpp:328/333`（等着 `pixmapCaptured`/
  `embedCodeCaptured`，永远等不到）
- **现象**：Qt5 那套靠 `QWebHitTestResult` 取"页面上这块图/这个嵌入代码"的能力，Qt6
  WebEngine 没有对应 API（要自己发 JS 到页面里取元素信息）。按钮点亮后只是切换一个内部
  标志位，什么都不捕获。已把 `actionWebTrap` 在 `WebController::setupPalettes()` 里
  `setEnabled(false)`，避免用户以为"没反应是我操作不对"。
- **建议**：恢复就在 `WebEnginePage` 里跑一段脚本取命中元素再回传（截图/内嵌代码两条输出），
  或者干脆连同工具栏动作一起删掉。

### 5. `FeaturesWidget` / `StartupHintsPalette` 里的 `boardApi` 是拿空场景造的死人对象（已清）
- **位置**：`src/ui/FeaturesWidget.cpp`（原来是 `new WebWidgetBoardAPI(Application::boardController->activeScene())`）、
  `src/ui/StartupHintsPalette.cpp`（原来是 `new WebWidgetBoardAPI(0)`）
- **为什么恒空**：构造链在 `Application::exec()` 里走到 `boardController->init()` →
  `setupViews()` → `BoardPaletteManager` → `FeaturesWidget`，而 `setActiveDocumentScene()`
  要到 `init()` 的后半段才调，所以 `activeScene()` 此刻还是 0。于是这些小部件 API 对象的
  `if (activeScene() != mScene) return;` 全部提前返回（`pageCount()` 恒 -1），
  而构造函数照样给全局的 `DownloadManager::downloadFinished` 挂了一条永久接收者。
- **处理**：两处都已删。`StartupHintsPalette` 整类连构造点都被注释，属于死类，在删死类那批
  一起删掉；`FeaturesWebWidget::boardApi` 从头到尾只被一个空 body 的
  `if (ok && NULL != d->boardApi) {}` 读过，连同那个槽一起删——少一条没人接收、
  每次下载完成都空转的全局连接。
- **剩下的规矩**：桥真要恢复，这些小部件门面必须像 `GraphicsWidgetItem::processDropEvent()`
  那样**按需创建**，不要在构造期拿“当时的活动场景”（见第三节）。

### 6. `.iwb` 的命名空间校验只打日志、不拒绝
- **位置**：`src/content/IwbSubsetAdaptor.cpp:1097/1105/1124/1202` 四条"incorrect … namespace,
  incorrect document"
- **现象**：命名空间对不上照样往下导，每个节点各打一条日志。代码里原来紧跟的
  `//return false;` 已经在注释清理那轮删掉（删的是注释，行为没变），各处补了一句中文
  说明“这里刻意放行”。这既可能是"故意容忍旧文档"，也可能是"当年为了导入某个坏文件
  临时关掉校验忘了恢复"，属于行为问题，别在日志清理的提交里顺手改。

### 7. 代码里剩下的 `TODO`（全仓 14 处，`WebWidgetBoardAPI` 那两处见第一节，不重复列）
- `src/core/PersistenceManager.cpp:163`（双击打开文档包时的等待光标绕法）、`:217`（临时 if）
  与 `:612`（复制页面时磁盘对象只能靠“再存一遍”顺带搬走）：上游遗留的临时分支。
- `src/settings/Settings.cpp:17`：`maxCrossSize` 是否开放给用户自行设置。
- `src/ui/DocumentNavigator.cpp:112`：文档页数与容器页数各数一遍，缺页就地补——
  临时绕法暴露的是结构问题。
- `src/whiteboard/FeaturesController.cpp:64`（`favoritePath` 该由控制器下发还是公开静态成员）、
  `:242`（库条目权限机制要重新设计）。
- `src/whiteboard/GraphicsTextItem.cpp:26`（控件本该按需创建却在构造期建）、
  `GraphicsStrokesGroup.cpp:34`（组缺统一管理机制）、`GraphicsDelegateFrame.cpp:777`
  （三个变换可合并成一次）、`GraphicsScene.cpp:653`（压感插值不平滑，起收笔有台阶）、
  `GraphicsStroke.cpp:152`（简化算法只会删近共线点，弯弧段简不掉）：绘制热路径上的算法欠账。

注释清理那一轮删掉了三处只写着“这里还没实现”的 `//TODO` 行，坑本身还在，记在这里免得被忘掉：

- `src/content/IwbSubsetAdaptor.cpp`（原 `:98`、`:1125`）：子集读写有两段没实现
  （“从 `parseIwbElement()` 的结果建组”）。
- `src/content/ImportIwb.cpp`（原 `:64/:78/:204`）：导入 .iwb 的临时目录与内容转换——
  与“旧文档不必保持可读”的既定决策一致，属于可以不填的坑。
- `src/whiteboard/GraphicsItemGroupUndoCommand.cpp`（原 `:28`）：
  `//Work around. TODO determine why does Qt call the redo function on pushing to undo`
  ——这个“为什么”已经查实并写成正向说明（入栈时 `QUndoStack` 会自动 `redo()` 一次，
  组合在创建命令时已生效，所以首次执行要跳过）。undo 栈语义可疑，动它仍要配回归。
- `src/whiteboard/FeaturesController.h`（原 `:154`）：
  `//TODO make less complicated for betteer maintainence`（原文如此）——一句没有指向的
  自我批评，删掉不损失信息；`Features` 那套模型/视图确实纠缠，要动先出方案。

另有两处不是 TODO 但同样别顺手改：

- `src/ui/MainWindow.cpp:1285`：导出时文件对话框的 `// HACK`（注释说明了为什么不能改成
  常规写法）。
- 文档格式相关的常量与旧键名，见下面第三节。

## 二、需要产品决策，不该由重构顺手改掉

- **每次保存都重写一遍 `metadata.rdf`**：`DocumentProxy::mIsModified` 只置真、没有清零路径
  （见 `model/DocumentProxy.h` 的 `isModified()` 注释），于是 `persistDocumentScene()` 里
  `if (pDocumentProxy->isModified())` 恒真——切一次主模式、自动备份一次、退出一次，元数据就
  各写一遍。日志里能直接看到同一份 `metadata.rdf` 连着写两次。
  别顺手"修"成保存后清零：`updated-at` 现在被写成"最后一次保存的时刻"，清零后它就变成
  "最后一次改元数据的时刻"，文档库里的"更新时间"列会跟着冻在创建时间上。
  要动它先定清楚产品要哪个语义，再决定是加清零、还是把 `updated-at` 改成只在内容真变时刷新。
- **导出用的 DPI 已经收敛成一个入口，但“取哪个屏”仍未定**：原来 11 处各自写
  `(physicalDpiX()+physicalDpiY())/2` 或只取 `Y`，同一份文档的导出与导入可能按两套比例折算；
  现在全部走 `VirtualDesktop::physicalDpi()`（固定取主屏）。要改成“画板所在屏”并不安全：
  `BoardView::resizeEvent` 按“最后 resize 的那个视图”决定 `sceneRect`，多屏下并没有唯一的
  “画板所在屏”，先要定清楚场景尺寸该跟哪块屏。
- **语言只放了中/英**。`LanguageManager` 的机制与 `tst_language` 已就位，加第三种语言
  只要补 `res/i18n/lang_<代码>.ts` 并产出 `Language/TianBoard_<lang>.qm`；
  在有人翻译之前不能凭空往菜单里加语种。
- **`Settings` 里 200 多个 `Setting*` 裸指针成员刻意不改**。看着像"该换成
  `unique_ptr`/注册表"，但那些对象全部以 `this` 为 parent 交给 Qt 的对象树托管，
  换成智能指针就是双重所有权（`deleteLater`、`removeSetting()` 与 QtAutomation 都会
  打架）；改成"按域分组的子注册表"则要动 62 个读方，收益只有观感。真要动它，先想清楚
  解决的是哪个具体毛病，而不是"指针太多"。

## 三、命名与形态遗留

- **`src/app/resource.h` 是全仓唯一保留 `#ifndef` 守卫的头**（`src/` 下 170 个头文件里
  169 个已经是 `#pragma once`）：`rc.exe` 不认识 `#pragma once`，把它改掉会让图标与
  版本信息静默丢失。另外 `TianBoard.rc` 是 UTF-16，改这两个文件必须用字节级工具，
  别用文本编辑器——编码一变整个文件就是二进制乱码，而 `rc.exe` 仍可能返回 0，
  只在产出的 `.res` 体积上露馅。
- `src/recording/WindowsMediaFile.h:10` 包住 `#define interface struct`（Windows SDK 的
  老写法）的那个 `#ifndef` **不是** include guard——守卫已经是 `#pragma once`。原先它拼成了
  `inteface`，等于永远不生效，于是每个含此头文件的编译单元都报一次 `C4005 “interface” 宏重定义`；
  `/W4` 扫描时已把拼写改对，现在是真正的"SDK 已定义就不重复定义"。别再改回 `inteface`。
- **`IwbConstants.h` 里还有 C 风格遗产**：第 12 行 `#define PI 3.1415926535`（污染宏命名
  空间，且与 `M_PI` 重复）、`FEATURE_*` 一组常量、`eAncestors`/`eCopyMode`/`eAddItemMode`
  这类 `e` 前缀 + 值前缀不一致的枚举。改成 `constexpr` 与 `enum class` 是纯改名，
  但要和 `svgIWBNS`/`tIWB*` 一组常量一起做，别混进功能提交。
  停靠面板那四组 `eWBDock*` 已经改成 `enum class`，见 git log。
- **`MimeType::AppleWidget`** 自己的注释写着 “mime type invented by us :-()”，
  却和 `GraphicsWidgetItem` 一路共享 “widget” 语义。名字里的 Apple 商标与实现无关，
  但改它要连写进 `.tbz` 的 mime 字符串一起看，属于协议层，先记。
- **一批「发出来但没人听」的信号留着没删**（本轮只删了既不发也没人连的那些声明）：
  `PersistenceManager::proxyListChanged`/`documentCreated`/`documentMetadataChanged`、
  `DocumentManager::documentUpdated`、`DownloadManager::cancelAllDownloads`/
  `allDownloadsFinished`、`GraphicsCurtainItem::removed`、`KeyboardPalette::localeChanged`。
  删它们要连 `emit` 一起删，属于「要么补上接收方、要么删掉」的判断，不适合和别的清理混
  在一批里做；`WebTrapWebView::objectCaptured`/`webElementCaptured` 则刻意留着——它们
  是「捕获网页」重写时的目标接口（见第一节第 4 条）。
- 类名前缀本轮没换新的：既定决策是「去掉 `WB`/`UB` 前缀且**不换新的**」，所以新代码也别加
  `TianBoard::` 这类项目命名空间（见 [coding-style.md](coding-style.md) 第二章）。
  `src/` 下已 grep 核对：没有 `UB*`/`ub*`/`WB_*`/`eWB*` 标识符残留；后来补扫出另外几族
  历史前缀并一并改名：`wbz*`/`ubz*`（文档包缩写留在标识符里）与临时目录名 `sank*`。
  这三族现在都不在 `src/` 里，改的时候注意只动标识符，别动写进 `.tbz` 的字符串值。
- `src/platform/WindowCapture.h`：有 `Q_OBJECT` 但没有同名 `.cpp`（实现是
  `WindowCapture_win.cpp`）。它**必须**继续显式列在 `src/platform/CMakeLists.txt` 里，
  删掉那一行 = moc 不参与 = 捕获窗口信号槽静默失效。同类风险见
  [coding-style.md](coding-style.md) 第四章。

## 四、构建与仓库层面的坑（记下来免得踩回去）

- **产物目录必须是 `${CMAKE_BINARY_DIR}/bin/<Config>/`，不能钉死到单一 `build/bin/`**。
  静态库属于 ARCHIVE 输出，多配置生成器下 Debug 与 Release 的 `tb_quazip.lib` 同名，
  钉死就互相覆盖：实测先构建 Release 再构建 Debug，Debug 会链到 Release 的 `.obj`，
  报 LNK1319 + 一串 LNK2038 RuntimeLibrary/`_ITERATOR_DEBUG_LEVEL` 不匹配。
  另外路径要相对 `CMAKE_BINARY_DIR`，否则两个隔离构建目录共用同一处照样串。
  `tst_smoke` 通过 `TIANBOARD_EXE`（`$<TARGET_FILE:TianBoard>`）拿路径，
  POST_BUILD 铺的 `etc/ library/ customizations/ Language/` 经 `$<TARGET_FILE_DIR>` 走，
  所以分目录不需要改别处。
- **`cmake --preset` 之前从来没跑成功过**：`CMakePresets.json` 用了 `"comment"`（合法字段是
  `"description"`）和 buildPresets 里的 `"config"`（合法字段是 `"configuration"`），CMake
  直接报 `Invalid extra field`。`scripts/build.ps1` 也带着两条过期假设：产物路径写死
  `build\bin\TianBoard.exe`（实际是 `<二进制目录>\bin\<配置>\`），以及 `--build ... --target test`
  —— 根 CMakeLists 只有 `enable_testing()` 没有 `include(CTest)`，根本没有 `test` 目标，
  要直接调 `ctest --test-dir`。**教训：只要没被真正执行过的构建入口，就要当作坏的。**
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
- **`.clang-format` 读不进去时是"整份配置作废"，不是"忽略那一条"**：里面原先写了个
  clang-format 不认识的 `CppNamespaceIndentation`，于是它对每个文件都报
  `invalid argument` 后什么都不做，而 `format.ps1 -Check` 照样把全仓文件全报成
  "待格式化"——看起来像"还有好多没排"，实际是"一个都没排成功"。改完配置后要用
  **单文件 dry run**（`clang-format --style=file:<文件> x.h | diff x.h -`）确认真的读懂了规则，
  别只看 -Check 的退出码。
- **文档里引用源码行号的地方，改完代码必须重新 grep 核对**：本台账有 20 来条
  `文件:行号`，一次 clang-format 全量重排就让它们**全部指偏**（多数偏 10–300 行，
  个别落到完全无关的代码上）。核对不用肉眼：
  `grep -oE "src/[a-z/]+\.cpp:[0-9]+" docs/code-suspicions.md` 列出来，再
  `awk -v N=n 'NR==N' 文件` 打印那一行看是不是描述的事物。同理“170 个头/169 个
  `#pragma once`”“全仓 21 处 TODO”这类计数也是派生数字，改完要重测。
- **构建报错不会让 ctest 变红**：MSBuild `/m` 下某个工程编译失败时其它工程继续，
  `TianBoard.exe` 沿用上一次链接好的旧文件，`ctest` 拿旧产物照样全绿。本轮
  实证踩过一次（删掉一个成员函数、漏改类内两处非限定调用，坏的提交已经进了历史）。
  所以“验证通过”的前提是**构建日志里 `error C`/`LNK` 计数为 0 且 exe 的 mtime 推进了**，
  然后才谈测试；也别用 `...; true` 这种收尾，后台任务的退出码会变成那句 `true` 的 0。
- **格式化结果绑定 clang-format 版本**（首次全量重排用的是 22.1.8）：升版本可能把
  整棵树再排一遍，所以升版本要单独一次全量提交，不与功能改动混批。长字符串会被拆成
  相邻字面量（`"…new "` + `"name…"`，值不变），带 `\` 续行的多行字面量则整串是一个
  token、不会被拆开——文档格式常量别手工改成跨行拼接的写法。
- `packaging/make-installer.ps1` 找不到 `ISCC.exe` 时**明确失败**（不是跳过），
  并对 stage 里的必需文件做硬校验；`windeployqt` 的模块参数按 Qt6 的名字写
  （`-webenginewidgets`，Qt6 没有 `-webengine` 这个选项）。脚本尚未在装了 Inno Setup
  的机器上实跑过，第一次跑要人工看一遍 stage 清单。

## 五、全量审计已修的缺陷（2026-10-20）

一次跨模块只读审计（model/content、whiteboard、core、web/desktop/recording）后集中修掉的
真实缺陷，记在这里避免以后又当成"设计如此"：

- **文档树**：多选删除按第一项的决定套全程（混选回收站可永久删除普通文档，或让
  `while (rowCount())` 死循环）；删除循环使用过期 `QModelIndex` 行号；`moveIndexes`
  的 `beginMoveRows`/`endMoveRows` 不成对；缩略图 `QPixmap` 三处泄漏。
- **IWB/SVG 读写**：`<switch>` 分支解析失败时死循环；空笔迹组 delete 后继续使用；
  `ref` 未命中时解引用空指针；空文本项 delete 后 `resize`；导出 `<g>` 无路径元素时
  `layers.at(0)` 越界；`metadata.rdf` 的版本号被后续元素覆盖回 4.1（"新版本文档"
  警告因此从未触发）。
- **画板**：置底撤销从 `end()` 解引用；复制把场景活对象放进剪贴板（源文档关闭后粘贴
  即 UAF）；`fls | GF_XXX` 当条件导致能力位恒真（连同 `sf_showProgress`）；单选到
  选择框时 Delegate 为空；SVG 按下每次泄漏一个 GraphicsPixmapItem 与一份 QMimeData；
  `addSvg` 落盘失败仍返回刚入场景的条目；Play 工具松手时 `movingItem` 可能为空；
  复制组重复 `addItem`；媒体工厂返回未初始化指针。
- **网页**：浏览器窗口 `deleteLater()` 后多处裸指针悬空；下载列表 `row == -1` 越界；
  进度标签的判断写反（正常下载不刷新）；把 `JavascriptEnabled` 当隐私浏览开关导致
  `RemovePolicy` 失效。
- **录屏**：启动失败不回收编码器/定时器（重试叠份）；视频初始化失败后麦克风一直采集；
  暂停偏移不清零使新录制首帧时间戳为负；音频时间戳在恢复时被清零；waveIn 回调线程
  直接写 `IWMWriter`（已改为拷贝 + 队列连接）。**这一段只过了编译与冒烟，没有在真机上
  录过音视频，改动后需要人工录一段验证音画同步与暂停恢复。**
- **核心**：关多屏时 `DisplayManager` 仍持有已删除的 mirror；复制文档把新 UUID 写到
  源文档；导入失败跳过 `setDisabled(false)`；本地下载 prepare 失败永久占用并发槽；
  `checkIfModalRemains` 条件写反；`-lang` 末参数越界；单实例消息无人接收（双击文档
  无反应）；`SceneCache` 从不淘汰（`pageCacheSize` 失效）；HTTP 重定向后完成信号被吞；
  退出时工具栏位置写在落盘之后；`MimeDataGraphicsItem` 不回收剪贴板副本。

仍未处理的审计发现（低置信度或产品决策）：`PreferencesController` 对
`selectedColorIndex() == -1` 未判空（只有配置里的颜色不在调色板时触发）、
`PersistenceManager` 垃圾箱重命名用源文档页数（Windows 上退化为无操作）。
