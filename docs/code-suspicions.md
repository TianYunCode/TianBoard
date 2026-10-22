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
- **位置**：`src/whiteboard/GraphicsWidgetItem.cpp:1025/1047/1068`（`freezedWidgetWrapper.html`、
  按 `npapi-wrapper*` 前缀挑包装页、读 `etc/npapi-wrapper.config.xml`）与
  `src/content/IwbSubsetAdaptor.cpp:1016`（`createNPAPIWrapperInDir(flashPath, ..., "application/x-shockwave-flash", ...)`）
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
  `setWebChannel` 调用）、`src/web/TabWidget.cpp:394/400`（等着 `pixmapCaptured`/
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
- **位置**：`src/content/IwbSubsetAdaptor.cpp:1278/1288/1312/1422` 四条"incorrect … namespace,
  incorrect document"
- **现象**：命名空间对不上照样往下导，每个节点各打一条日志。代码里原来紧跟的
  `//return false;` 已经在注释清理那轮删掉（删的是注释，行为没变），各处补了一句中文
  说明“这里刻意放行”。这既可能是"故意容忍旧文档"，也可能是"当年为了导入某个坏文件
  临时关掉校验忘了恢复"，属于行为问题，别在日志清理的提交里顺手改。

### 7. 代码里剩下的 `TODO`（全仓 12 处，2026-10-22 重新数过；`WebWidgetBoardAPI` 那两处见第一节，不重复列）
- `src/core/PersistenceManager.cpp:718`（复制页面时磁盘对象只能靠“再存一遍”顺带搬走，缺一套
  正式的引用复制机制）：上游遗留。今天动过它一半——那段按图元类型搬包内文件的循环已经搬进
  `content/SvgSubsetAdaptor::duplicateItemResources`，但“靠再存一遍搬”这个机制本身没变。
  （原先这里还记着另外两条：双击打开文档包的等待光标绕法与一处临时 if，改造中已被别的工作消掉。）
- `src/settings/Settings.cpp:15`：`maxCrossSize` 是否开放给用户自行设置。
- `src/ui/DocumentNavigator.cpp:127`：文档的页数与容器里的页数各数一遍、缺页就地补——
  临时绕法暴露的是结构问题。
- `src/whiteboard/FeaturesController.cpp:64`（`favoritePath` 该由控制器下发还是公开静态成员）、
  `:278`（库条目权限机制要重新设计）。
- `src/whiteboard/GraphicsTextItem.cpp:23`（控件本该按需创建却在构造期建）、
  `GraphicsStrokesGroup.cpp:36`（组缺统一管理机制）、`GraphicsDelegateFrame.cpp:916`
  （三个变换可合并成一次）、`GraphicsScene.cpp:729`（压感插值不平滑，起收笔有台阶）、
  `GraphicsStroke.cpp:184`（简化算法只会删近共线点，弯弧段简不掉）：绘制热路径上的算法欠账。

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

- `src/ui/MainWindow.cpp:1247`：导出时文件对话框的 `// HACK`（注释说明了为什么不能改成
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

- **上游的 `Web/PrivateBrowsing` 有意没搬**。它要做的是"整个应用用无痕档"：新建一个
  `QWebEngineProfile`（不带存储名的构造=off-the-record）并让每一个 `QWebEngineView` 都用它。
  我们的网页层现在统一吃 `QWebEngineProfile::defaultProfile()`（Application 构造里就在给它配
  `QWebEngineSettings`），要接无痕就得把 profile 一路穿过 `WebView`/`BrowserWindow`/
  `TabWidget`/镜像与捕获那几处——是一次成体系的改动，而且和这轮已经做好的"退出清 Cookie +
  第三方 Cookie 过滤"重叠大半，先不做。真要做时注意：切档必须重建页面，运行中改这个开关
  不可能无损生效。

## 三、命名与形态遗留

- **`src/app/resource.h` 是全仓唯一保留 `#ifndef` 守卫的头**（`src/` 下 199 个头文件里
  198 个已经是 `#pragma once`）：`rc.exe` 不认识 `#pragma once`，把它改掉会让图标与
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
- **一批「发出来但没人听」的信号留着没删**（本轮只删了既不发也没人连的那些声明；名单 2026-10-22
  逐条重测过——`grep connect` 为空才算"没人听"）：
  `PersistenceManager::proxyListChanged`/`documentCreated`/`documentMetadataChanged`、
  `DownloadManager::cancelAllDownloads`、`GraphicsCurtainItem::removed`（它的注释还写着"供场景在删除前
  统一通知观察者"，可没有任何观察者）、`KeyboardPalette::localeChanged`。
  原先列在这里的两条已经不再成立，别再照旧名单动手：`allDownloadsFinished` 现在有人听着
  （`ui/DockPalette.cpp:112`），`DocumentManager::documentUpdated` 这个信号早已随一轮清理消失。
  删它们要连 `emit` 一起删，属于「要么补上接收方、要么删掉」的判断，不适合和别的清理混
  在一批里做；`WebTrapWebView::objectCaptured`/`webElementCaptured` 则刻意留着——它们
  是「捕获网页」重写时的目标接口（见第一节第 4 条）。
- **`core/IdleTimer` 已删**（2026-10-22）：它想做的事写在注释里很完整——“画板有焦点且 2 秒没有任何
  输入就把鼠标指针藏掉”，实现也自洽（`eventFilter` 记时间、`timerEvent` 判断后
  `setOverrideCursor(BlankCursor)`），但全仓除了它自己和 `src/core/CMakeLists.txt` 的源文件清单
  一处都不认识它：没有构造点，也没人把它装成事件过滤器，所以这条功能从来没生效过。画板上真正
  看得见的藏指针是另一条路（`whiteboard/BoardView.cpp:1133`、`whiteboard/GraphicsScene.cpp:862`
  直接对 viewport `setCursor`）。它一删，`core` 对 `whiteboard` 少 2 条边。留这条记录是为了口径：
  判死类要看**有没有构造/安装点**，不是看“某个方法没人调用”——前者才是运行时不存在。
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

**删掉一个 Qt 信号 / 改动带 `Q_OBJECT` 类的元数据之后，增量构建可以产出一个"看起来全新、
其实启动就崩"的二进制**（2026-10-22 实测）：把 `DocumentManager` 的 `documentUpdated` 信号
连同它唯一的 `signals:` 段删掉后，`build/` 增量链接出的 exe 在 `main` 之前就崩
（栈顶是 `__scrt_initialize_type_info` → `pre_c_initialization`），8 个拉起应用的用例全红；
`rm -rf build` 重新 configure + 构建之后同一份源码全绿。门禁查不出来，因为新鲜度判据比的是
"源码不比产物旧"，而这个产物确实比源码新。**规矩：动过 `Q_OBJECT` 类的信号/槽或虚表组成之后，
先用干净的构建树跑一遍再判断是不是自己的代码错了**（`scripts/warnings.ps1 -Clean` 或删 `build/`）。

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
- **判一个素材是不是“没人用”，先看路径是怎么拼出来的**。全仓 grep `:/images/moveUp.svg`
  一定查不到引用，因为 `ThumbnailWidget` 是 `addIcon("moveUp")` 拼的
  （`:images/` + 名字 + `.svg`），`KeyboardPalette` 拼得更深
  （`:/images/virtual.keyboard/` + 行高 + `/` + `capslock` + `.png`）。这一轮按“文件名有没有
  出现在任何非 res 文件里”扫，20 个“死资源”里 17 个是这种拼接误报，真死的只有
  `InteractivesCategory.svg`、`tab_mask.png`、`toolbar/updates.png` 三个。
- **QtSvg 认渐变的 `xlink:href` 借色**：素材库里 6 个 `white_*` 形状用的是
  `<linearGradient xlink:href="#SVGID_2_">` 这种 Inkscape 别名写法，实测能正常画出白色主体。
  别按“Qt 只支持 SVG Tiny 1.2，遇到借色一定画成透明”去改素材。
  反过来也别加“渐变色标必须在渲染结果里出现”的像素门禁：星形、菱形这类窄形状在色标端点
  附近本来就凑不出几个像素，而 `white_rectangle.svg` 里两条空 path（`d="M77.242,54.635"`）
  引用着退化渐变（x1=x2 且 y1=y2）却不影响可见图形——实测 7 个误报，全都不是缺陷。
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
- **角度磁吸有两份实现**：`GraphicsDelegateFrame::mouseMoveEvent` 里是老的 45°/30° 容差判断，
  还带一个 `mAngleOffset` 借位机制（吸住之后把多拖的部分留着，拖回来时不丢角度）；整组旋转的
  `SelectionFrame` 现在走 `Snapping::toNearestAngle`（同一组步长与同一个 `angleTolerance`，但没有
  借位）。规则一致、实现重复。合并时注意：借位不是可有可无的修饰，去掉它会让"绕过 45° 继续拖"
  手感变粘，所以要么两边都带借位，要么两边都不带，别只改一边。

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
  `awk -v N=n 'NR==N' 文件` 打印那一行看是不是描述的事物。同理“199 个头/198 个
  `#pragma once`”“全仓 12 处 TODO”这类计数也是派生数字，改完要重测（今天实测：头文件数
  从 170 涨到 199、TODO 从 14 掉到 12，两处旧数字都在这次核对里被换掉）。
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
- **代码里写着并行，构建里根本没有 OpenMP**（2026-10-22 实测并删掉）：`GraphicsScene::eraseLineTo`
  带着三处 `#pragma omp`（一处 `parallel for`、两处 `critical`），看起来擦除是按图元并行求交的。
  全仓 CMake 搜不到任何 `openmp`（根与 `src/*/CMakeLists.txt`、`build/CMakeCache.txt`、生成的
  `.vcxproj` 都没有），exe 的字节里也找不到 `vcomp140` 或 `omp_get` 这两个名字——MSVC 在没有
  `/openmp` 时把这些 pragma 整个忽略，循环一直是串行跑的，`critical` 那两对花括号只是多余的作用域。
  已删成明写的串行循环并留一句"为什么不是并行"。**可复算的判据**：
  `grep -ri openmp build/CMakeCache.txt src/*/CMakeLists.txt CMakeLists.txt` 无匹配，加上
  `python -c "print(open('build/bin/Debug/TianBoard.exe','rb').read().find(b'vcomp140'))"` 是 -1。
  真要给擦除提速是性能工作：热点在 `items(rect)` 粗筛与 `QPainterPath::subtracted`，
  得先量大页面耗时再决定，别和这类清理混在一批。

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

- **格式往返（`tst_format` 抓到的）**：`rulerToSvg`/`compassToSvg` 写的是
  `boundingRect()`，而 `rulerFromSvg`/`compassFromSvg` 用 `setRect()` 还原——差的那一圈正是
  画笔外扩，于是尺子和圆规每“存一次再开一次”就胖一个笔宽，刻度跟着变形；量角器本来就写
  `rect()`，所以只有它两端一致。两处改成写 `rect()`（`SvgSubsetAdaptor.cpp:2888`、`:2943`）。
  这条是本轮新写的 `--format-selftest` 第一次跑就报出来的，也是它存在的理由：界面上完全看不出
  来，只有把同一页存了再冷读回来才会露。
- **IWB 互操作（同一轮里加的第二腿）**：把自检文档导出 `.iwb` 再导回来，一次抓到四处：
  ① `GraphicsItemDelegate::lock()` 无条件 `mFrame->positionHandles()`，而 `mFrame` 只在
  `createControls()` 里建；导入过程中刚建出来的图元从没被选中过，于是 **导入 .iwb 直接崩**
  （0xC0000005，`delegated+0xf: mov rcx,[rax+28h]`，rax=0）。已判空（`GraphicsItemDelegate.cpp:608`）。
  ② `parseSvgImage` 把图片落到 `boardController->selectedDocument()` 的目录而不是正在导入的那份
  （`addPixmap` 默认参数），落完又把图元 UUID 换成新造的（`IwbSubsetAdaptor.cpp:970`），
  页文件于是引用 `images/<新UUID>.png` 而盘上是 `<旧UUID>.png` ⇒ **导入回来的图片全是空白**。
  改成用代理目录 + 保留 `addPixmap` 分配的 UUID。把 ② 临时退回，`tst_format` 立刻报
  “IWB 导入的图片是空位图”，恢复即绿——这条断言不是摆设。
  ③ 导出器对 polyline 无条件 `stroke := fill`，而本页 polyline 的颜色本来就在 `stroke` 上、
  `fill` 恒为 `none` ⇒ 笔迹颜色导出去变成空串。改成只有本来没有 stroke 时才回落（`IwbDocumentAdaptor.cpp:2073`）。
  ④ 图片引用是带命名空间的 `xlink:href`，而本文档按 namespace processing 解析，
  `attribute("href")` 取不到 ⇒ 走不进“文件不存在”那条告警、静默给一张空位图。已按 `attributeNS` 取。
  **仍然会丢的（未修，别再当往返无损）**：IWB 没有工具图元的概念，尺子/量角器/圆规导出去就没了；
  一笔 polyline 回来是一个覆盖整条包围盒的 polygon，`originalWidth()` 是 -1，逐段结构与笔宽不还原。

- **从未接线的缩略图卡片（已删）**：`ThumbnailProxyWidget` / `DraggableThumbnail` /
  `DraggableThumbnailPixmap` / `DraggableThumbnailView` 四个类连同只被它们用的
  `ui/ThumbnailView` 在全仓没有一个 `new`（`DocumentController` 里那句
  `#include "ui/ThumbnailView.h"` 是唯一的活引用，也只是 include）。文档模式真正在用的
  可操作缩略图是 `SceneThumbnailPixmap` 那一族：它自己画悬浮图标、自己判命中，不走
  `QGraphicsProxyWidget` 那条路。删掉约 490 行，两个只属于死类的 `Page %0` 原文也随
  `lupdate -no-obsolete` 退出翻译目录（悬浮图标 `close/duplicate/moveUp/moveDown` 仍被
  活类调用，资源一个没动）。

- **快捷键当文案（做快捷键自定义之前先算的账）**：24 条
  `setShortcut(QCoreApplication::translate("MainWindow", "Ctrl+T", nullptr))` 写在
  `MainWindowUi::retranslateUi` 里，有两处后果：① 21 条按键名因此进了翻译目录，谁把它们译了
  就改掉按键绑定的解析（补中文时只能整批标成“刻意不译”）；② `retranslateUi` 每次切换语言都
  重跑，于是“用户自己改的快捷键”会被下一次换语言冲掉——现在还没有这个功能，所以它只是个暗坑。
  本轮把它补成真功能（`core/ShortcutManager` + `ui/ShortcutSettingsWidget`）时一并搬正：默认值
  以数据形式待在 `setupUi`（只跑一次），用户覆盖存在 `Shortcut/<objectName>`，注册发生在主窗口
  构造的最后一步。`Application::exec()` 里那三行事后 `setShortcuts(QKeySequence::Copy/Cut/Paste)`
  也并入 `setupUi`：它们跑在主窗口构造之后，正是“后写的一刀盖掉先读回来的覆盖”，留着它，
  改绑复制/剪切/粘贴必然失效。`DocumentController` 里两处
  `actionDelete->setShortcut(QKeySequence(Qt::Key_Delete))` 同罪（与 `setupUi` 的默认值本来就重复），
  一并删掉；现在全仓只有 `setupUi` 写默认值、只有 `ShortcutManager` 写覆盖，装配后期不再有人
  补刀。`tst_shortcuts` 会抽查这几条默认值仍在原位，就是为了钉住这个顺序。

- **`QModelIndex::sibling()` 被当成"取第 i 个孩子"用了两处（2026-10-21，补导入重名自检时撞出来的
  栈溢出）**：`QAbstractItemModel::sibling(row, column, index)` 的语义是"**和 index 同一个父节点**
  的第 row 行"，实现里先 `parent(index)` 再 `index(row, column, 那个父)`。`DocumentTreeModel::removeRows`
  清空子树那段写的是 `parent.sibling(i, 0)`，拿到的却是 `parent` 自己的兄弟——在"我的文档"下删一行时
  `i` 为 0 会指回 `parent` 本身，于是 `while (rowCount(curChildIndex)) removeRows(0, 1, curChildIndex)`
  每轮都在删同一个父节点的第一行、行数永远不减，直接递归到栈溢出（`c00000fd`，Qt 的代理模型在
  `create_mapping_recursive` 里先把栈吃光）；`copyIndexToNewParent` 递归复制目录时同一处误用会让
  复制一个分组却搬来它邻居的子树。两处都改成 `index(i, 0, parent)`。判据与现场在
  `tests/tst_import.cpp`（`--import-selftest` 的第 4 个案子：同名替换）。
- **当前文档那一行被删掉后 `mCurrentNode` 没人清**：`DocumentTreeNode::~DocumentTreeNode` 会
  `delete mProxy`，而 `mCurrentNode` 是裸指针；`currentIndex()` 恰好是从根重新搜节点而不是走父链，
  所以不会当场崩，但 `data()` 里 `mCurrentNode == dataNode` 这种比对拿的是已释放的地址——地址被
  新节点复用时"当前文档"的高亮图标会落到别人身上。`removeRows` 现在在删之前把自己（或自己子树里）
  那份清成 `nullptr`，`tst_import` 第 7 个案子直接断言 `currentNode()` 归零。
- **一条被我写错又改回来的“缺陷”**：曾记下 `createDocumentProxiesStructure` 遇到分组不存在时 `return`
  会"把整批导入静默断掉"。实测不成立——`DocumentTreeModel::goTo()` 在找不到某一级时**就地补建**那个
  分组（`tst_import` 第 8 个案子钉住这一点：删掉分组再导入，分组连同里面的文档都被补回来，且不会掉进
  "我的文档"根）。所以那条 `return` 实际不可达；记下这条是为了别再拿它当缺陷去"修"。
- **重名导入的三处漏点（同一次改动顺手补）**：① `createDocumentProxiesStructure(..., interactive=true)`
  把 `processInteractiveReplacementDialog` 的返回值丢掉，用户点"取消"后那份刚解压出来的文档包
  既不登记也不释放——内存里漏一个 `DocumentProxy`，磁盘上留下一包没人认领的目录，下次启动扫描时
  它还会自己冒出来；现在取消即整包退回磁盘外，与另两个调用点的行为一致。② 对话框的确认按钮只看
  `mLineEdit->hasAcceptableInput()`，而 `mValidator` 从来没被创建过（`setRegexp` 一旦调用就是空指针解
  引用，好在零调用点），所以清空输入框也能点确认：旧名字被写成空串后 `addDocument()` 直接 return，
  代理又漏一次。现在解析器返回空名字一律按取消处理，`setRegexp`/`mValidator`/`setFileNameAndList`
  和两个零接收者的信号（`createNewFolder`、`closeDialog`）一起删掉。③ 替换发生时先 `deleteDocument`
  旧的、后 `addDocument` 新的，中间界面被叫去"选中那个还不在树里的新文档"，`setCurrentDocument`
  因此静默落空、当前节点继续抱着即将删除的旧节点；现在先挂新的再换旧的。


### 2026-10-22：`src/ui` 的 N–Z 段补审（D–M 那一段见下一节）

同样先核实再动手，八条里修了七条：

- **越界读写（最严重）**：`PreferencesController` 的两个"网格透明度"槽拿
  `ColorPicker::selectedColorIndex()` 当下标，而该索引来自 `list.indexOf(当前交叉色)`——
  配置里的交叉色不在那组网格色中时是 **-1**，于是 `colors.at(-1)` 与 `colors.replace(-1, …)`。
  release 下 QList 不查边界，这是越界读加越界**写**。现在两支都先验索引再动手。
- **恒真条件**：`ThumbnailWidget::mouseReleaseEvent` 判 `elapsed() < STARTDRAGTIME`，
  而 `STARTDRAGTIME` 是 `1000000`——`elapsed()` 的单位是毫秒，等于 16.6 分钟，条件永远成立。
  查过接收方才决定怎么处理：`DocumentController::pageClicked` 把 item 与 index 都 `Q_UNUSED`，
  只调 `pageSelectionChanged()` 刷新动作可用性，而框选结束正需要这一次刷新——所以既不照字面
  补一个时间阈值、也不改成按位移判断（那都会把松手后的刷新丢掉），而是删掉恒真判断直接发，
  行为与现在完全一致，但不再谎称有防抖。顺带删掉只 start 从不读的 `mClickTime` 与那个宏。
- **累积框选丢失**：同一个函数里先 `mPreviouslyIncrementalSelectedItemsX = 本轮集合`、
  再 `toSet += 本轮 + mPreviously...`，两个操作数已是同一个集合，"往外拉框时累加之前选中的"
  整个失效。改成先算并集再覆盖。X/Y 两支同错。
- **`QSet::contains(整个集合)`**：那层守卫写的是"本轮集合是否包含上一轮整个集合"，
  QSet 没有这种重载，且紧接着的差集本来就覆盖了这种情况；删掉。
- **Ctrl 增量框选漏最后一项**：`for (i = 0; i < count() - 1; ++i)`，且每轮重建一次
  `values()` 临时表。改成取一次、遍历全部。
- **`ScreenMirror` 坐标系**：`mapToGlobal()` 要的是部件内部坐标，传的却是
  `geometry().topLeft()`（父容器坐标系）→ 记下的抓屏矩形整体偏移。平时看不出来是因为
  `ApplicationController` 随后会 `setSourceRect()` 覆盖一次；源部件析构后回退矩形抓屏用的
  就是这一份错的。改用 `mapToGlobal(QPoint(0,0))` / `mapToGlobal(rect().bottomRight())`。
- **`ZoomPalette::hide()` 不是重写**：`QWidget::hide()` 非虚，只有经 `ZoomPalette*` 的直接
  调用才走派生逻辑。真虚的是 `QWidget::setVisible(bool)`，而 `hide()/show()` 内部都走它——
  于是改成覆写 `setVisible()`，任何入口的判断都一致了。
- **偏好设置里两个色板重复**：`setupUi` 已经建了 `crossColor{Light,Dark}BackgroundColorPicker`
  并 addWidget，`wire()` 又 new 一对真的接了线的塞进同一布局 → Grid 页出现两个色块，
  前面那个没有颜色列表也没有连接。现在统一用表单里那两个，多余的两个成员删掉。

**没修的一条（需要人工验证，先记着）**：笔与荧光笔两个面板共用同一份
`BrushPropertiesFrame::Ui`，切语言时以前只有对话框自己重翻译，面板文案停在旧语言。已经让
`BrushPropertiesFrame` 自带 `retranslateUi()` 并在 `languageChanged` 上刷两个面板，但
"切语言后面板确实变过来了"这件事只有真点一遍才算数——无头自检目前只覆盖主窗口与设置对话框
的取词（`tst_language`）。

另记一条测量事实：`tst_ui_modes` 与 `tst_recording` 在**同一时刻还有全量构建在跑**时
出现过一次超时失败；单独重跑 12 次全绿。这台机器的负载会直接影响这两个用例，判红先确认
当时是不是在并发构建（`tst_ui_modes` 现在对"慢"重试三次，见 §五 相应提交）。

（上一轮这个模块因轮次上限没跑完）

只读代理报回 7 条，逐条自己核实后修掉，其中三条是"写好了却从没被接上"的：

- **`DocumentController::closing()` 连定义都没有**，`mIsClosing` 只在构造里置 false，
  于是 `pageSelectionChanged()` 与 `updateActions()` 里那两道"关闭时别再动界面"的守卫
  **永远不生效**。而 `Application::closing()` 第一件事就是清回收站——删文档、动选择树，
  信号正好打回这两处。现在补上定义并在清回收站之前调用它（与 `webController->closing()`
  同一个套路）。
- **`DocumentNavigator` 从没 `setAcceptDrops(true)`**：`dragEnterEvent`/`dragMoveEvent`/
  `dropEvent` 三个处理器一直是写好的，但 `QGraphicsView` 默认不收拖放，所以"长按缩略图
  拖动换页序"这条功能整体是死的。开起来的同时补了三道守卫——原来 `dragEnterEvent` 对
  外部拖进来的东西走 `acceptProposedAction()`，而 `dragMoveEvent`/`dropEvent` 直接解引用
  `mDropSource`，一旦真收了外来拖放就是空指针崩溃；现在只接自己发起的那一趟。
  **注意**：这条改动把一段从没运行过的交互代码接上了，无头环境模拟不了 `QDrag::exec`，
  所以换序本身还没被人真按过一次，只是不会再崩。
- **`MousePressFilter`（"长按转双击"）全仓零实例化**，只剩 `DocumentController.cpp` 一句
  include；而且它内部三处都坏：把 `QMouseEvent` 静态转成 `QTabletEvent` 读 `globalPosition()`
  （成员偏移不同）、`mObject` 是裸指针且 1 秒后可能已析构、用 `SLOT()` 字符串连接（违反本仓
  函数指针规矩）。半截功能按惯例删掉，要接回从 git 历史取。

另外四条是真缺陷：

- **`currentIndexMoved` 每拖一次漏一个 `DocumentProxy`**，而且漏的那个还留在容器上：它
  `new DocumentProxy(*newProxy)` 交给 `pureSetDocument`，最后只把**画板**改回 `newProxy`，
  文档库控制器却从此认着一个不在树里的幽灵副本。注释说"模型随后会销毁 newProxy"不成立——
  这个信号只在 `MoveAction` 时发、`newIndex` 指向刚克隆到新位置的那个节点， doomed 的是源
  节点那一份。`pureSetDocument` 也只是赋值，不删任何东西。改成两处都直接接 `newProxy`。
  **复核订正（2026-10-22）：这条泄漏在当前装配下跑不到。** `currentIndexMoved` 全仓只有一个
  emit 点——`DocumentTreeModel::dropMimeData` 的 `Qt::MoveAction` 分支，而绑定这棵树模型的唯一
  视图 `DocumentTreeView::dropEvent` 开头就把 `dropAction` 设成 `IgnoreAction`，`dropMimeData`
  第一行据此 return false；真正在用的换组路径是它自己调的 `docModel->moveIndexes()`（移动节点、
  不改代理身份，因此不需要重指）。于是 `dropMimeData` 的文档树分支＋`currentIndexMoved`
  信号/槽/连接是一簇不可达代码。**没顺手删的原因**：拖放换组是用户可见功能，无头环境模拟不了
  `QDrag::exec()`，删错了没人看得见；先记着，等有手动验证条件再一起处理。
- **`deleteEmptyFolders` 先把整层子节点快照成普通 `QModelIndex` 再逐个删**：`deleteIndexAndAssociatedData`
  会 `removeRow` 掉正在处理那一行，同父节点下靠后的行号全体前移一位，于是
  `containsDocuments(child)` 拿过期索引判断"这个目录是空的吗"，判错就可能删掉还有文档的目录。
  改成**倒序遍历、每轮现取索引**。（复核订正：同一批改动里的 `deleteDocumentsInFolderOlderThan`
  原本被记成"正向遍历会漏检过期文档"，**不成立**——`deleteDocument` 不碰模型行，代理走
  `deleteLater`、行由 `deleteEmptyFolders` 那一路摘，循环期间 `rowCount` 恒定，正倒序等价；
  那里保留倒序只为别扫到循环中新插入的行。另外两版的"删掉自己这一层"分支都进不去：
  唯一调用点先用 `hasChildren(trashIndex)` 把关，而 `childCount` 在入口取。）
- **`generateThumbnails` 用 `scene->items()` 的下标当页号**恢复选中态：每页在场景里占两个
  条目（缩略图 + 标签），`mDropBar` 还排在最前，所以每次重建后高亮落到别的页或直接丢失。
  改成按对象身份先查出页号。
- **`DockDownloadWidget` 的左右箭头图标与 `PageNavigationWidget` 正好相反**：`DockPalette`
  按"收起态取 iconToRight（朝外＝可展开）、展开态取 iconToLeft（朝内＝可收起）"选图，
  下载面板因此把两个状态画反。

补审后仍留在台账里的低置信度项见下一段。

仍未处理的审计发现（低置信度或产品决策）：`PreferencesController` 对
`selectedColorIndex() == -1` 未判空（只有配置里的颜色不在调色板时触发）、
`PersistenceManager` 垃圾箱重命名用源文档页数（Windows 上退化为无操作）。

### 2026-10-22：从 `--format-selftest` 日志里抓出的资源落盘簇

跑自检时日志里出现 `Cannot open file '<文档目录>/images/{uuid}.svg'`，顺着这条线查出三处同族缺陷，
都在"图元的资源文件"与"页文件里的引用"之间的关系上：

- **`SvgSubsetAdaptor` 的两个写出器只写引用、不写字节**：`pixmapItemToLinkedImage` 与
  `svgItemToLinkedSvg` 往页文件里落 `xlink:href="images/{uuid}.png|svg"`，但都不保证那份文件
  真在盘上（前者的 `path` 局部变量算出来根本没用，是这条证据）。同文件的 `pdfItemToLinkedPDF`
  一直是"没有就写"，所以这是不一致而不是设计。**后果实测**：保存再打开，图元读回来是一个
  0.9×0.9 的空东西，内容指纹正是空串的 MD5（`d41d8cd9…`）——不崩、不报错，只是东西没了。
  触发条件是"这一页上有没经过 `GraphicsScene::addPixmap`/`addSvg` 进场景的图元"，也就是
  粘贴/复制（`deepCopy`）与跨文档导入那两条路。现在两个写出器都改成缺文件就补写。
- **`GraphicsScene::addSvg` 把资源写进"当前选中的文档"而不是这一页所属的文档**：它取
  `BoardController::instance()->selectedDocument()->persistencePath()`，而 `addPixmap` 早就有
  `useProxyForDocumentPath` 走 `this->document()`（`ImportImage`、`IwbSubsetAdaptor`、自检都传 true）。
  IWB 导入用 `mCurrentScene->addSvg(...)` 导入到另一份文档时，字节落进了正在显示的那份文档，
  这一页的引用成了空头——上面那条 `Cannot open file` 就是它。改成优先按场景自己的 `mDocument` 落，
  没有归属文档时记一条告警（图元仍留在内存里，只是保存后这一页会丢它）。
- **`deepCopy` 让副本沿用源图元的 UUID**：`GraphicsPixmapItem` 与 `GraphicsSvgItem` 的构造函数
  本来就发了新 UUID，`deepCopy` 又 `setUuid(this->uuid())` 改回去，两份于是共用同一个按 UUID 命名的
  文件；而 `CoreGraphicsScene::deleteItem` 会调 `clearSource()` 按 UUID 删文件——删掉任一份，
  另一份引用的文件就没了。现在保留构造函数发的新 UUID，副本自己的文件由写出器补齐。
  `clearSource()` 这一族今天又量深了一层，两件事记清楚：
  ① 它按"画板当前选中的文档"算路径（图元此刻已从场景移除，拿不到归属文档），删非当前文档里的图元时
     会删空、留下一个孤儿文件；文件随文档目录整体删除，暂不处理。要真修，该让
     `CoreGraphicsScene::deleteItem` 把**自己那份文档**传进去（`clearSource(DocumentProxy *)`）——
     调用方手上有场景，归属本来就该由它给。
  ② 更靠前的一层：`BoardController::ClearUndoStack()` 只对 `mActiveScene->deleteItem(item)` 动手，
     而图元的"待删登记"在它自己那个场景里；跨文档时这一步返回 false，于是直接 `delete item`，
     `clearSource()` 根本没被调到——文件同样留下。两处是同一个根：**撤销栈与释放点都按"当前"算，
     而图元的归属是"它自己那一份"**。
  今天只把三处 `BoardController::instance()->selectedDocument()->persistencePath()` 这条不判空的链
  加了守卫（退出期 `instance()` 已析构或当前没有文档时记一条告警再返回，不再空指针解引用），
  归属机制本身一字未动。

验证：`tst_format` 的 `buildScene` 现在多了一个内嵌 SVG 与它的 `deepCopy` 副本，`sceneFacts` 按
SVG 原始字节做指纹；把写出器那条"缺文件就补写"临时短路成 `false` 重新构建，自检立刻报
`读回后缺少: svg|770.0,500.0,…` 与 `读回后多出: svg|769.1,499.1,0.9,0.9|d41d8cd9…`，恢复后 18/18 全绿。

同批把日志串里带上游错字与粗话的 22 处换成中文（`an error ocured`、`for wrighting`、
`starting converion`、`embeded`、`cannot make sens of`、`Who the hell are you ?` 等）；
其余约 200 条英文日志没有错字，只是与新增代码的中文日志不一致，要不要整体统一是单独的决定。

### 2026-10-22：第 19 条自检（剪贴板端到端）又抓出两处

`--clipboard-selftest` 把"复制→粘贴→删原件→存读往返"整趟跑了一遍（这条路以前只过编译），
过程中查出两个真缺陷：

- **`ClearUndoStack` 用来源 URL 当"还在剪贴板上"的替身，而空 URL 也算匹配**：`sourceURLs`
  只在剪贴板装着 `boardPageItem` 时才填，填进去的是各图元的 `sourceUrl()`；画板里大半图元
  根本没有来源地址（贴上去的位图、内嵌 SVG、文本、笔迹都是），于是 `sourceURLs.contains(空)`
  对**每一个**无来源的已删图元都成立——它们全被当成"还在剪贴板上"，一个都不释放。已经移出
  场景、又不在撤销命令之外任何地方，切几次页就在内存里攒下一批。改成空 URL 不算匹配
  （`!ubi->sourceUrl().isEmpty() && …`）；这条改动安全的前提正是上一节那个写出器补写：
  剪下来又还没粘回去的图元现在会真被释放，连它按 UUID 命名的资源文件一起清掉，
  而那份文件的原主（副本）自己在下次存页时把字节重新落回盘上。
- **`ui/DocumentNavigator` 两处裸解引用缩略图**：`updateSpecificThumbnail` 直接
  `*BoardController::instance()->pageAt(iPage)`，而 `pageAt()` 越界返回 `nullptr`（Release 下
  `QList::at()` 也不查边界，紧接着那句 `d->mThumbsWithLabels.at(iPage)` 同样裸用）；
  重建那趟循环里写着 `Q_ASSERT(!pix->isNull())`——缩略图还没生成出来时（盘上那张 PNG 缺失或
  读不出，`documentPageThumbnail` 给的是一张空图）Debug 直接 abort，实测 control A 就撞在
  这里：`ASSERT: "!pix->isNull()" in src/ui/DocumentNavigator.cpp`。数据缺失不该让应用没命，
  改成记一条告警、这一格按空图摆着等下一轮刷新，越界那两处判过再走。

反向对照三条都过（各自短路掉一处修复，`--clipboard-selftest` 立刻变红、恢复即绿）：
副本沿用源 UUID → 2 条不一致；写出器不补字节 → 2 条；空 URL 也算匹配 → 2 条（盘上留下
本该随原件删掉的 2 份文件，最后 images/ 是 4 个而不是 2 个）。

### 2026-10-22：第 20 条自检（导出端到端）撞出一处崩溃，并钉住一条改文档的读页路

`--export-selftest` 把一份两页文档真导成 PDF 与 `.tbz` 再读回来对账（导出器唯一的非交互入口是
`persistsDocument()`，过去界面点"导出"只看得到"没报错"）。第一次跑就是段错误。

- **已修：`DocumentTreeModel::mCurrentNode` 从来没被初始化**。构造函数只管建根节点，从没碰这个
  成员；`setCurrentNode()` 全仓只有一个调用点（`goTo()` 里）。于是任何一次在任何一次 `goTo()`
  之前的 `removeRows` 都会走进 `if (mCurrentNode && (curChildNode == mCurrentNode ||
  curChildNode->findNode(mCurrentNode)))`——Debug 下它的值是调试堆的 `0xCD…` 填充（探针实测
  `current 0xcdcdcdcdcdcdcdcd`）恒"非空"，紧接着拿它去走父链就是野指针解引用；Release 下取值
  取决于堆上正好残留了什么。崩溃栈：`ExportSelfTest::run → ImportDocument::importFile →
  PersistenceManager::createDocumentFromDir → registerDocumentProxy → removeRow →
  DocumentTreeModel::removeRows → DocumentTreeNode::findNode → parentNode`，触发条件是"导入一个
  与现有文档同名的包并答替换"——界面上就是重新导入自己刚导出的那份文档。改成声明处 `= nullptr`。
- **已修：`PersistenceManager::loadDocumentScene` 在某一页读不出来时会改文档**。旧写法
  `createDocumentSceneAt(proxy, 0)` 插一页、再读第 0 页，然后把这页
  `mSceneCache.insert(proxy, sceneIndex, scene)` 挂到**请求的那个页号**上：既让 `pageCount` +1
  （页号全体后移），又把第 1 页的内容当成了"第 N 页"。实测证据：一份多出一页没有页文件的文档
  连导两趟 PDF，`pageCount` 从 3 涨到 4，第二次导出的 PDF 多一页——导出本该是只读的。
  现在只有"这份文档一页都还没有"才替它建第 0 页，其余情况记一条告警并返回空；四个产品调用点
  （`ApplicationController`、`DocumentController`、`BoardController` 两处）本来就判了空，
  `ExportPDF` 那条没判，补上"这页读不到就跳过、并且明确告诉用户这份文档没有可读的页"。
  自检这一腿用"声称两页、盘上一页都没有"的文档（这样才不会被场景缓存挡掉），断言导出前后
  `pageCount` 不变、不许补出页文件、也不许一声不吭；把 `loadDocumentScene` 改回旧写法，
  这条立刻 3 处变红（反向对照实测）。
- **API 脚枪：`createDocument(group, name, withEmptyPage, directory, pageCount)` 的两个页数参数是
  叠加的**——`pageCount` 只是先 `setPageCount()`，`withEmptyPage=true` 再插一页并 `incPageCount()`，
  所以 `pageCount=1 + withEmptyPage=true` 得到 2 页（两个自检都因此多出一页空页，已改成传 0 并在
  调用处注明）。产品路径 `createDocument(group)` 用默认 `pageCount=0`，没有这个问题。

顺带钉住几条现状（以后动导出就得对得上）：PDF 每页尺寸 = 场景像素 × 72 /
`VirtualDesktop::physicalDpi()`；导出会临时把深色底改成白底、画完再恢复（恢复没做就是屏幕上白一下）；
`ExportFullPDF` 的 `*_overlay.pdf` 中间产物必须被改名掉、不许残留；同名替换之后"我的文档"根下该名字
只剩一份。反向对照：把 `pdfWriter.newPage()` 短路，自检报页数不对（2 条）。

同一轮把导入那一侧也接上了产物断言（`--import-selftest` 第 9、10 两条用例）：用 `QPdfWriter`
现场造一份两页 PDF、用 `QPainter` 现场画一张 PNG，走界面上同一个入口 `DocumentManager::importFile`，
断"PDF 导入后 2 页、每页恰好一个 `GraphicsPDFItem`、文档目录自包含、冷读回来图元还在"与
"图片导入后 1 页、回来的像素与源文件指纹一致、`images/` 里有那张 PNG、冷读回来仍然一致"。
两个反向对照：把 `ImportImage` 放进场景的位图换成空图 → 3 条红；把 `DocumentManager` 里导入时那次
`folderToCopy` 短路 → **仍然全绿**，因为每存一页时 `pdfItemToLinkedPDF` 会把缺的 PDF 重新落盘
（位图与 SVG 从这一轮起也是同样写法）。也就是说那条"文档目录自包含"的断言钉的是结果而不是手段，
真正承重的是写出器；这一点在测试代码里也写明了，免得后人误读。

### 2026-10-22：第 21 条自检（撤销/重做全回路）——这条路量下来是干净的

`--undoredo-selftest` 在画板真实的可撤销场景里跑一遍：换文档清栈、加图元压栈、`undo()` 移出场景、`redo()` 放回**同一个对象**（按 UUID 找回、像素指纹一致）、`Item::remove` 之后撤销能连内容一起带回、再重做又走掉、文本同一条回路、`ClearUndoStack()` 之后栈空而场景不变、换到另一份文档时栈跟着空。
**六步全绿**：撤销机制本身没有暗病，这也是 `BoardController::undoStack()` 归属改动之后第一次有人把整条回路跑完。反向对照：把 `GraphicsScene::addPixmap` 里那次 `push` 短路，3 条断言立刻变红。

### 2026-10-22：第 22 条自检（自动保存）：修掉"关掉之后还在写盘"，并纠正一个我自己的错判

`--autosave-selftest` 量四件事：画板装配之后 `BoardController` 名下存在且只存在一个正在计时的
`QTimer`，间隔等于设置里的分钟 × 60000；**改设置当场**间隔就得跟着变（不许借一次模式回调）；
设成 0 之后不许还有活动定时器；恢复原值后定时器重新跑起来；再量一遍主模式那一维（切出画板模式要停、
切回来要跑）。最后调一次公开的 `saveData()`，要求当前页的页文件真的变长。

- **已修**：`appMainModeChanged()` 遇到间隔为 0（也就是"关掉自动保存"）时直接 `return`，正在跑的那个
   定时器不会被停下来。用户在偏好设置里关掉之后，画板其实还在按旧间隔往盘上写。改成先 `stop()` 再 return。
   反向对照：把这段改回旧写法，第三条断言变红（实测 1 条不一致）。
- **我先前判错的一条，记下来免得再犯**：我假设"定时器只在 `appMainModeChanged()` 里创建，所以开机就是
   画板模式、一整会话不换模式的用户根本没有自动保存"，并顺手在 `init()` 末尾补了一次启动调用。
   反向对照把那次调用注释掉，自检**仍然全绿**——因为 `Application::exec` 结尾本来就会调
   `applicationController->showBoard()`，而每个 `show*()` 都 `emit mainModeChanged(...)`。
   那条调用是多余的，已经删掉。教训同"先查谁反向 include 了谁"：动手前先拿一条会红的断言证明缺陷存在。
- **同日补掉的体验缺口**：原先改自动保存间隔只有在下一次换主模式时才生效——重排只写在
   `appMainModeChanged()` 里，没人连 `autoSaveInterval` 的 `changed`。现在把那段重排抽成
   `rearmAutosaveTimer(mode)`，`appMainModeChanged()` 与新增的 `autoSaveIntervalChanged()` 都调它，
   后者连在 `Settings::settings()->autoSaveInterval` 的 `changed` 上（这种连接全仓已有五处在用）。
   间隔改成 0（关闭）同样当场生效——这与上面"关掉之后还在写盘"是同一族问题：**设置改了要立刻落到
   运行中的物件上，别等下一个事件顺路把它带进来**。反向对照：去掉那条 `connect`，自检立刻 2 处红
   （"设置改成 2 分钟之后定时器还是 180000 毫秒（改完没立刻重排）"、"间隔设成 0 之后还有 1 个定时器在跑"）；
   接回去 25/25 全绿。

### 2026-10-22：第 23 条自检（页面操作）：四步做完，每一页内存与盘上都得是同一套内容

`--pages-selftest` 在画板真实的文档上做"新增两页 → 复制第 1 页 → 把第 1 页移到第 3 位 → 删掉第 2 页"，
每一步之后把每一页摊成内容清单（文本按原文、位图按像素指纹）比较两份：一份从场景缓存直接拿（内存里的这一页），
一份绕开缓存读页文件重建（盘上的这一页）。页操作同时牵着 `pageNNN.svg` 改名、场景缓存搬移与资源复制，
错一位都不崩不报错，只让某一页悄悄变成另一页。

- 四步做完**全绿**：页数对、副本内容等于原件、移动就是"取出再插到目标位"、删除不动别的页。
- 反向对照（把 `duplicateDocumentScene` 的改名循环上界少挪一格，制造经典的 off-by-one）：立刻 6 处红，
   其中一条正是"复制之后第 3 页内存里是 PAGE-TWO、盘上是 PAGE-THREE"——这种状态下一次自动保存就会把错误的
   内容写进那一页，属于最难发现的一类数据腐蚀。
- 我一开始的判法太严，报了一条假红：应用本来就是"离开这一页/换文档/自动保存"时才写当前页，
   所以刚改过的当前页在盘上还是旧的。对账之前必须先 `persistCurrentScene(false, true)` 把当前页落盘，
   这才是"用户会看到的状态"。教训与自动保存那条同一类：断言要对着**契约**写，不是对着我以为的时机写。

### 2026-10-22：第 24 条自检（音视频插入）：拖进来的本地媒体不再留在文档外面

`--media-selftest` 现场合成一个合法的小 WAV（8bit 单声道 0.1 秒）放在文档目录之外，分两条分支插进画板：
"接口拿到了字节"与"只给了文件路径"，然后要求媒体成为文档的一部分（图元指向的路径必须在文档目录里且文件存在）、
页文件里的引用指得动、存盘再冷读回来的图元数与内容都对。

- **已修**：`BoardController::addVideo/addAudio` 的 `bUseSource` 参数——它跳过"把媒体复制进文档"这一步，
  而 `SvgSubsetAdaptor` 的音视频写出器永远按 `audios/<文件名>`、`videos/<文件名>` 这种**文档内相对路径**写引用。
  唯一的 `bUseSource=true` 调用点正是 `downloadFinished` 里"没拿到字节"那一支，也就是拖本地文件进来那条路：
  保存后文档带着一个指向外部文件的悬空引用，换机器或原文件被删，这份文档就放不出来了。参数一并删掉
  （只有一种正确行为时不该留开关）。反向对照：把复制改回可跳过 → 2 处红（"没有被复制进文档"+"冷读回来变少"）。
- **顺带发现的第二个症状其实是同一个原因**：`GraphicsMediaItem::createMediaItem` 只看路径里有没有
  `videos`/`audios` 这两个词，于是"拖一个桌面的 tone.wav 进来"曾经直接返回空、界面上什么也没发生也不报错。
  我先按扩展名/MIME 加了判断，然后反向对照（把扩展名分支短路掉）发现**结果不变**——媒体无条件进文档之后，
  路径里必然带 `audios`/`videos`，那条判断是死代码，已经回退。留在这里是为了记住："看起来像第二个缺陷的，
  常常是第一个缺陷的下游症状。"

### 2026-10-22：第 25 条自检（本地下载与应答连线）：服务层不再命令画板，顺带修掉两处不判空的全局解引用

`--download-selftest` 把一个放在文档之外的 WAV 交给下载队列，量的是**产物与界面状态**：入队时右侧面板那个
下载标签页出现、文件按字节落进这份文档的 `audios/`、排空后标签页收回、源文件仍在，再数一遍
"有下载入队/队列排空/黑屏已解除"各报了几次。

- **已修（隐患，不是当下会复现的故障）**：`core/DownloadManager.cpp` 里两处
  `BoardController::instance()->…` 完全不判空——`AsyncLocalFileDownloader::prepare()` 拿
  `selectedDocument()` 算复制目标路径，`addFileToDownload()`/`finishDownloads()` 拿
  `paletteManager()` 开关下载页签。`BoardController::instance()` 是个 `QPointer`，画板还没装配（或已经在
  退出期析构）时就是空指针解引用。现在前者改成向装配点装上的 `installTargetDocumentProvider` 要文档，
  给不出就打一条告警、这次下载按失败收尾；后者改成两条事实信号（`downloadsQueued`/`downloadsDrained`），
  由画板自己把调色板的应答连上。同一趟 `DisplayManager::unBlackout()` 也不再直接
  `BoardController::instance()->freezeWebWidgets(false)`，只 `emit blackoutLifted()`。
- **为什么这些改动必须有测试盯着**：把"服务直接命令画板"换成"服务报事实 + 装配点连线"之后，
  连线丢了既不崩也不报错——界面上只是"下载页签不出来""黑屏解除了小部件还冻着"。三条反向对照各自
  只打红该红的那一条：不装提供者钩子 4 处红（文件无处可去）；摘掉画板那两条应答连线，只有
  "入队后标签页没出现"1 处红、下载本身照旧成功；去掉 `emit blackoutLifted()`，只有报了几次那 1 处红。
  这组红/不红的分布本身就是证据：**产物对账钉的是结果，连线状态钉的是应答**，两件事必须分开钉。
- **一处判据写错差点混过去**：下载失败时整条链路是同步走完的（`prepare()` 失败 → `onDownloadError` →
  收尾），所以 `addFileToDownload()` 返回时"排空"已经报过、页签也已经收回，"入队后页签该出现"这条会跟着红。
  正常路径下复制在工作线程里，这条才量得到页签的出没。判据要对着**时序契约**写，否则一次反向对照就会
  报出比预期更多的红，看着像"改坏了两处"。

### 2026-10-22：翻页条会自己把缺的缩略图补回来，"手工插入是不是冗余"必须按路径各自量

`--pages-selftest` 加了"缩略图列表条数 == 页数、每页都有一只图、副本的图与原件一样"这三条之后，
做的第一条反向对照是**把 `BoardController::duplicateScene` 里那次 `insertThumbPage(nIndex)` 注释掉**——
预期应当红（少一条），实际**全绿**。真正兜住的是翻页条自己数一遍就地补：

- `ui/DocumentNavigator.cpp:70` 把 `DocumentContainer::documentThumbnailsUpdated` 连到
  `generateThumbnails()`，那里有一句 `if (i >= source->pageCount() || !source->pageAt(i)) source->insertThumbPage(i);`
  （`:129-131`），并且它自己就带着一条 TODO（`:127`）：“文档的页数与容器里的页数本应一致，现在却要
  各自数一遍，缺页只能就地补”。`duplicateScene` 结尾正好 `emit documentThumbnailsUpdated(this)`，
  把这条兜底走了一遍。
- 文档库那侧是**另一份**列表：`DocumentController` 也是 `DocumentContainer` 的子类，它槽里的
  `reloadThumbnails()` 重建的是自己那份，碰不到画板的。
  **我第一版把机制记在 `documentSceneCreated → DocumentController::reloadThumbnails` 上，是错的**——
  看到一条 connect 不等于它改的是同一个对象：同一个基类的两个实例各有一份 `mDocumentThumbs`。
  教训比原来那条更硬：**读代码读出来的"机制"也要有一条会红的判据撑着**，否则连"为什么红不了"都能记错。

所以"手工插入是不是冗余"得按路径各自量，答案确实不一样：`addFilesToDocument` 那条就**不**冗余——
它不发 `documentThumbnailsUpdated`，把 `insertThumbPage` 去掉后 `--import-selftest` 第 11 案立刻红。

- **反过来这也是拆环的一条出路，但那条 `documentSceneCreated` 用不得**：一开始以为
  `core/DocumentManager` 那句 `BoardController::instance()->insertThumbPage(pageIndex)` 可以直接不发——
  `documentSceneCreated` 本来就在发，画板连上去就行。查下来不行：那是**页级**事件，
  `DocumentContainer::addPage`、`PersistenceManager::insertDocumentSceneAt`、`duplicateDocumentScene` 都在发它，
  而 `addPage` 发完自己就 `insertThumbPage`，画板再挂一条同样的应答就是**插两次**。所以真正落地的是显式
  处理器：core 只报"往这份文档追加了一页"（`DocumentManager::notifyPagesAppended`），实现由装配点装上。
  教训：**别把"某个信号已经在发"当成"接线就能复用"**，先数清谁在发、发几次、现有应答方已经做了什么。
- **顺带治掉的一处真可疑**：旧写法无条件把条目插进**画板容器**的列表，而同一个循环里
  `createDocumentSceneAt`/`persistDocumentScene` 用的是传进来的那份 `document`——从文档库往**别的文档**
  追加一页时，被挤位的正是画板当前那份文档的缩略图（翻页条下次生成缩略图时会自己补回来，
  所以界面上看不出来）。现在装上去那一版只在"这份文档就是画板当前那一份"时才动列表。
- **判据是先补的，而且是会红的**：`--import-selftest` 新增第 11 案，走界面上同一个入口
  `addFilesToDocument` 往画板当前文档追加一张现画的 PNG，断页数 +1、那一页的位图像素与源图一致、
  `images/` 里确有那份文件、画板缩略图列表跟着长一条；再往**另一份**文档追加一次，断画板那份列表
  **不许**跟着长。两条反向对照各自只打红该红的那一条：去掉那次 `insertThumbPage`（等价于不装处理器）
  →"列表 1 条 vs 页数 2"；去掉实现里"是不是当前文档"那道判断 →"往别的文档追加一页，画板那份列表
  却被改成 3 条"。
- **一条差点写错的判据**：第 11 案的 target 最初用 `createDocument(..., pageCount=1)` 造，看着是
  "有 1 页的文档"，其实**没有页文件**——`loadDocumentScene` 返回空，`setActiveDocumentScene` 就不切文档，
  于是"画板当前文档"一直是别人的那份，追加也确实没动画板的列表，红得毫无线索性。改成
  `withEmptyPage=true` 并加一条前置断言"画板必须真的切到这份文档"，量的才是这条路本身。
  **凡依赖"画板当前是哪份文档"的用例，都要显式断那句切换成功，不能假定。**

### 2026-10-22：撤销/重做自检补第 6 案——`ClearUndoStack` 末尾那次清栈是删页链路的承重墙

`--undoredo-selftest` 加了"画一笔 → 在翻页条上删掉当前这一页 → 再撤销、再画一笔"之后，做的反向对照
是把 `BoardController::ClearUndoStack()` 末尾那句 `undoStack()->clear()` 注释掉——进程**直接段错误**。
`cdb` 抓到的栈（符号未优化，函数名清楚）：

```text
Qt6Widgetsd!QGraphicsItem::parentItem
TianBoard!BoardController::findUniquesItems
TianBoard!BoardController::ClearUndoStack
TianBoard!BoardController::setActiveDocumentScene
TianBoard!UndoRedoSelfTest::run          ← 就是 board->deleteScene(...) 这一步
```

机制读清楚了，值得记：**`ClearUndoStack` 那个循环会把"既不在场景也不在剪贴板"的图元直接 delete**
（那是剪贴板/撤销占用的图元唯一的释放点），而命令对象还抱着它们；只有末尾那次 `clear()` 把命令一起
丢掉，下一轮遍历才不会读已释放内存。所以那句看起来像收尾的清栈**不是装饰，是这条链路的承重墙**，
而它原先只在换页/换文档时"顺路"被调到，从来没有用例钉过。现在第 6 案钉住了：去掉它就红（异常退出），
留着它就绿。

### 2026-10-22：第 26 条自检（偏好当场生效）：预览圈的图元不该在建页时定生死

先做了一遍全量核对，免得下次从头数：偏好设置里真能改的是 40 个设置项，逐项去数它们在 `src/` 里的读者
（去掉 `src/settings/` 与 `PreferencesController` 本身），再看那个读者是**每次动作都跑**还是**只在装配
或建页时跑一次**。口径可以复算：`grep -oP '^\s+(Setting|ColorListSetting) \*\w+' src/settings/Settings.h`
取全部注册项（`new Setting`/`new ColorListSetting` 各 104 个，与 `Settings.h` 里的成员一一对应；门禁那个
"155 个设置项"还含只走 `value/setValue` 的遗留键），与 `grep -oP 'settings->\w+' src/ui/PreferencesController.cpp` 取交集＝**40 个**
界面上真能改的，再对每一项在 `src/` 里数读者。结论：

- **绝大多数每次读，所以当场生效**：网页那一组（外部浏览器、主页、镜像屏立即显示）在每次进入网页模式
  时读；工具条位置/文字/方向、屏幕键盘键值、日期列、下载起止与解除黑屏各有显式 `changed` 连线或界面侧
  的成对连线；笔宽、颜色、压感、网格与十字色在落笔与绘制时读。
- **按设计就是下一次才算的两项**：`appStartMode` 只在 `Application::exec` 读一次（"启动模式"讲的就是
  下一次启动），`emptyTrashForOlderDocuments`/`emptyTrashDaysValue` 在 `Application::closing()` 里算
  （讲的是"退出时清多久以前的回收站"）。
- **界面上根本没有控件、只能改 INI 的一批**：`App/HideSwapDisplayScreens`（默认 `true`，直接把那个
  勾选框藏起来，于是 `DisplayManager::swapDisplayScreens` 这个槽在默认配置下没有任何发送者）、
  `ShowMarkerPreviewCircle`、`ShowEraserPreviewCircle`、`historyLimit`、`pageCacheSize`、`libIconSize`、
  `mirroringRefreshRateInFps`、`boardZoomFactor`、`angleTolerance`。其中几个确实只在建页/装配时读一次，
  但既然界面上没有入口，"改了要重启"就不构成界面上的失灵，这一轮不动它们。

真修掉的这一条是**不对称**的失灵，只靠眼睛看不出来：`GraphicsScene::createPenCircle()` 过去把
`Board/ShowPenPreviewCircle`（默认 `true`）当作"这一页要不要有那个图元"的开关，而显示与否又是
`drawPenCircle()` 每次悬笔重新算的。于是在"取消勾选"期间建好的那些页上**压根没有圈可显示**，把勾选框
重新打开只对新页生效；反过来取消勾选是"下一次悬笔不再显示"，看着像是当场生效（只有已经显示着的那只圈
会留到指针离开这一页才收）。同一条链路上两个方向的生效条件不一样，界面上的表现就是
"勾了没用，得重启，而且旧页永远不会有"。

改法是把决定权挪干净：`createPenCircle()` 不再看设置，图元跟荧光笔预览圈同口径地一直建出来（它记在
`mTools` 里、图层标成 `Control`，存页与导出都被剔除，所以多建一个隐藏图元不改变盘上格式），显示与否
只在 `drawPenCircle()` 每次算；顺带把它那一直只设光标、不收图元的 `else` 支补上一次 `hidePenCircle()`
——否则取消勾选之后圈会挂在最后一次显示的位置上，直到鼠标离开这一页。
改完专门查过一件本来会担心的事：圈挂在 `Control` 层（Z=2000，压在笔迹之上），可见时
`BoardView` 的 `scene()->itemAt(...)` 会先命中它，而 `BoardView::mousePressEvent` 是**先取 `movingItem`
再调 `inputDevicePress`**（那里才 `hidePenCircle()`）。这个面不是新引入的：`ShowPenPreviewCircle` 的默认值
本来就是 `true`，开机就建的那些页一直是这个形状。圈本身也没有任何交互标志，并且
`BoardView::itemShouldBeMoved` 是按 `item->type()` 走 switch 的，plain 椭圆不命中任何 case，落到最后的
`return false`——所以它被当成 `movingItem` 也拖不动。这一刀改变的只是"设置该不该对每一页都算数"，
没有改变圈与命中测试的关系。

判据用公开入口跑真链路：`Settings::setPenPreviewCircle` 就是勾选框连的那个槽，悬笔走
`GraphicsScene::inputDeviceMove`（工具切到钢笔、不落笔），观察量取 `tools()` 里**挂在 `Control` 层的可见
椭圆图元**个数（按层级键先排掉指针那只圈——它挂 `Tool` 层，而且 `hideTool()` 不收它；橡皮与荧光笔那两只
这条链路不会点亮）。同一份临时文档里建两页，唯一区别是建它的时候设置是什么：开着时建的那页必须看得见
圈（**这条改前也是绿的**，它的作用是证明"看不见"确实是页上没有图元，而不是判据空转），关闭时建的那页
把设置打开之后也必须看得见；再加阈值那一维（抬到笔宽之上必须归 0、调回 0 必须又出现）与取消勾选
（必须归 0）。改前 2 处红、改后 0 处；两处修法各自单独退回过一遍，各只打红自己那一条
（退回 `createPenCircle` 的无条件建 →"旧页画不出圈"；退回 `else` 支那次 `hidePenCircle()` →"取消勾选后
圈还在"），所以这两条断言是各自独立的，不是一条覆盖两处。

荧光笔与橡皮的预览圈**故意没跟着改**：它们的 `drawMarkerCircle`/`drawEraser` 只看图元在不在，不再读
设置，把创建改成无条件等于让那两个 INI 开关失效——那是行为退化，不是修 bug。

同一趟核对里记下一条待决：`Board/AutoSaveIntervalInMinutes`（`autoSaveInterval`）全仓唯一的写者是
`AutoSaveSelfTest`，偏好设置面板里没有这个控件（`res/i18n/*.ts` 里也没有任何相关文案，所以不是迁移
丢的，是上游本来就没有）。上一轮把它的 `changed` 连上之后，"改间隔当场生效"这条在界面上其实走不到，
只能改 INI。要不要补一个控件（连"关闭"这一档）是产品决策，见任务清单，不在这一批里顺手加。

### 2026-10-22：第 27 条自检（橡皮擦）：擦除这一侧一次跑通，红点全在"落笔"——每一笔的尾巴被简化整批扔掉

橡皮是画板上用得最多的工具之一，却从没被任何用例跑过（`src/app/*SelfTest.cpp` 里只有
`ShortcutSelfTest` 提到过 `Eraser`，那是快捷键注册表那一层）。`--eraser-selftest` 按界面上那条路走：
`DrawingController::setStylusTool` 切工具，`inputDevicePress`→`inputDeviceMove`→`inputDeviceRelease`
画两笔，再竖着擦过第一笔的中段。**判据是几何不是包围盒**：把所有 `GraphicsPolygonItem` 用
`sceneTransform()` 换成场景坐标，再问 `QPolygonF::containsPoint` "这个点上到底有没有笔迹"——于是
擦除带中心必须没有几何、带的两头与另一笔必须有几何，缺一边都算红。同一趟还钉：一次抬笔只许压
**一条**撤销命令（`Ctrl+Z` 一次要把整段擦除原样还回来，多边形块数也得对上）、重做又没掉、擦完存盘
再绕开场景缓存冷读回来，盘上必须还是擦过的样子、笔迹组始终是 2 个。

反向对照把 `GraphicsScene::eraseLineTo` 开头直接 `return`，**5 条断言同时变红**（"擦了个寂寞"、命令
条数、撤销后块数、重做、冷读），所以擦除这一侧的判据没有空转的。而**擦除本身第一次跑就全绿**——
切开、残片顶替、合成一条命令、撤销还原、存读一致，这条没人走过的路是干净的。

红的是"落笔"那一侧，而且是一条人人踩中的截断：`inputDeviceRelease()` 原来在**简化之前**就把
`mTempPolygon` 并进笔迹，而 `GraphicsStroke::simplify()` 是照 `mDrawnPoints` 重建多边形的——插值
（`InterpolatePenStrokes` 默认 `true`）时贝塞尔只画到相邻落点的**中点**，真正的抬笔点从没进过那张点表，
它只活在 `mTempPolygon` 里；于是 `simplifyCurrentStroke()` 那句"把当前笔迹的多边形整批扔掉、换成简化版"
把这一小段一起扔了。简化同样是默认开的（`SimplifyPenStrokes=true`），所以**每一笔的尾巴都比笔尖停下的
位置短**，缺的是最后一段步长的一半：三个采样点（x=270/400/530）那一笔的几何实测只到 465，短了 65 像素。
改法是调顺序——先把 `mTempPolygon` 复制出来并摘掉，跑简化，再把尾巴 `addPolygonItemToCurrentStroke`
并进去（顺带还更对了：半透明笔迹的自叠裁剪现在按简化**之后**的相邻段来减，原先减的是马上要被丢掉的
那批段）。

这条判据写的是契约（"抬笔点必须真有几何"），不是当时量到的行为——又验证了一次立下的规矩：**断言要
对着契约写**。还有一点值得记：给一条没人跑过的用户链路写端到端自检，抓到的缺陷常常不在那条链路上，
而在它必经的上游那一步。

### 2026-10-23：第 28 条自检（回收站）：移入与清空本身是对的，但永久删除在这棵树上留下一个已释放的节点

`--trash-selftest` 量的是文档库里那条"右键删除 → 进回收站 → 清空回收站"的路。三条判据第一次跑就全绿，
而且每一条都不是显然会成立的：

- **归属真的落盘**。`folders.xml` 只存目录树、不存文档，一份文档属于哪个分组是写在它自己的
  `metadata.rdf` 里（`DocumentTreeModel::updateIndexNameBindings` → `persistDocumentMetadata`）。
  自检读那份字节，要求里面能看到回收站的组名前缀 `Settings::trashedDocumentGroupNamePrefix`（`_Trash:`）——
  写不回盘上，重启之后"删掉的文档"就回到我的文档了。
- **移入回收站不许删包**：这一步之后盘上的目录必须还在。
- **清空回收站要删干净**：走界面上那条映射（`EmptyTrash` → `emptyFolder(index, CompleteDelete)`），
  断包从盘上消失、回收站那一层行数归 0、留在外面的那份文档一个都不许少。反向对照：把
  `PersistenceManager::deleteDocument` 里那次 `FileSystemUtils::deleteDir` 短路，只有"孤儿目录"那一条红
  （其余判据照旧绿），说明这几条各管各的。

**测出来的缺陷（已复现，根因还没定位）**：永久删除之后再走这棵树就会踩一条 Qt 断言——
`ASSERT: "len >= 0" in file .../QtCore/qstringview.h, line 135`。定位过程值得记，因为我第一版归错了地方：
先看到"第 4 步建文档时崩"，以为是 `createDocument` 的问题；在清空前后各插一条标记之后量到的是
`emptyFolder` **本身跑完了**、victim 的包已经没了、回收站行数已经是 0，断在紧接着的下一句
`docModel->indexForProxy(victim)` 上（同一趟里 `pm->createDocument()` 也会踩，同一个根因）。
`indexForProxy` 只做一件事：`findProxy` 沿 `mRootNode` 找代理、再 `indexForNode` 数它在兄弟里的位置，
所以留下的是一只**已经被释放的树节点仍挂在 children 里**。顺路的事实也对得上：
`deleteDocument` 用的是 `deleteLater()`，而 `deleteIndexAndAssociatedData` 随后就 `removeRow`。
Debug 下这是一条断言；**Release 下 Q_ASSERT 整个不参与编译，那读到的就是已经不该再用的内存**——属于
"文档库点几下之后界面自己乱掉"那一类，不是理论风险。

因此这条自检暂时**没有**第 4 步（`deleteDocumentsInFolderOlderThan` 按天数清理）与"树里不该再留着已删
文档的行"这条判据：它们都要在删除之后再走一次这棵树，会把整个用例变成一次崩溃而不是一个可比较的退出码。
测试代码里留了话说明为什么。修好根因之后这两条要一起补回来——那时它们才量得动。
