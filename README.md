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

`tests/` 下是 Qt Test 用例，一共 25 个，`ctest -C Debug` 全部通过。`tst_stringutils`、`tst_snapping`、`tst_geometryutils`、`tst_version` 只链接被测的那几个 `utils/*.cpp`，`tst_credentialstore` 只链接 `platform/CredentialStore_win.cpp`——它们都脱离整个应用栈单独构建、单独运行，这也是把模块依赖拆干净之后才有的好处。`tst_credentialstore` 验的是凭据封装里最容易写错的几处：UTF-16（含 emoji）往返、target 不存在时读回空、覆盖写、只有用户名、两边都空即删除、超过 `CRED_MAX_CREDENTIAL_BLOB_SIZE` 必须在写入前拒绝；它用自己的 PID 当 target 并在收尾删掉，不会碰到真实的代理凭据。`tst_pdfrenderer` 用 QPdfWriter 现场生成一份两页 PDF，验证 `whiteboard/QPdfRenderer` 的契约：页码从 1 起、`pageSizeF` 按 DPI 把点换算成像素、`render` 真的把页面画出来、只画 `exposedRect` 指定的局部、坏文件被判为无效而不是崩溃。`tst_assets` 检查 `res/TianBoard.qrc` 登记的每个文件确实存在、`res/images` 下每个文件都能解码、品牌图形按设计渲染（瓦片外透明、瓦片内是蓝色渐变、T 形笔画是白的），并把 `library/` 与 `images/` 下每个 SVG 真渲染一遍：接管消息处理器捕获 QtSvg 的解析告警（超限的 path 只会打印一句 `Invalid path data; path truncated.` 然后画出半个图形），同时要求画布上确实有像素。凡是拉起 TianBoard.exe 的用例（`tst_smoke`/`tst_ui_modes` 与十三个 `*selftest`）都会经 `TestProcessSandbox.h` 给子进程设 `TIANBOARD_DATA_DIR`，把文档仓库与日志整个关进 `%TEMP%/TianBoard-<用例>-<pid>` 并在收尾删掉——应用每次启动都会新建一份文档，不给它一个临时根目录的话，每跑一次 ctest 就往用户真实的文档库里塞七份没人认领的文档。代价是看自检日志要去那个临时目录的 `log/` 下。`tst_smoke` 读取 `TIANBOARD_EXE`（由 ctest 的 ENVIRONMENT 属性注入）真正拉起 TianBoard.exe，确认它跑完启动链并活过 6 秒、收到关闭请求后能自己退出（实测约 2 秒；20 秒内没走完就重试，连续 3 次都慢才算失败）、且启动日志里没有失效的 SIGNAL/SLOT 连接；应用是单实例的，若新进程把请求转交给已在跑的实例就会自己正常退出，所以它也会换一次时机重试，三次都被接管才 `QSKIP`（不是失败）；崩溃与非零退出码仍然照实变红。它同时校验 `etc/`、`library/`、`customizations/` 与 `etc/TianBoard.config` 确实和可执行文件同级。`tst_ui_modes` 同样拉起 exe，带上 `--ui-smoke` 让应用自己依次切到 文档 / 网页 / 画板 / 桌面批注 四种主模式再退出：Debug 下任何 Qt 断言都会弹对话框卡住事件循环，进程就不会按时退出——这正是文档模式代理索引错用（启动后点“文档”必现）的回归测试；桌面批注那一腿还量得到装配点装的批注层工厂有没有真装上（core 现在只认识 `IDesktopAnnotation` 契约）。`tst_format` 带上 `--format-selftest`，让应用在初始化完成后真存一页、再绕开场景缓存把盘上的字节真读回来逐项比对：一笔三段笔迹（几何、颜色、深浅底两套取色、笔宽）、一段文本、一张位图与一个内嵌 SVG（都按内容指纹比：位图按像素、SVG 按原始字节，验的是资源真进了这一份文档的对象目录而不是页上只剩一个引用——正是这条断言抓出"写出器只写引用不写字节"）、这两样各再放一份 `deepCopy` 出来的副本（副本有新的 UUID，插入时没人替它落盘，于是逼写出器补字节）、尺子/量角器/圆规/坐标轴四件工具图元（坐标轴还带着刻度数字显隐这个状态位），以及页面背景类型与网格尺寸，还要验顶层图元的叠放次序；随后把同一页原样再存读一轮，要求第二轮结果与第一轮完全相同——累积型误差（每存开一次就挪一点、胖一点）只看一轮常常看不出来。最后把这份文档导出成 `.iwb` 再由导入器读回一份新文档，断言文本、图片与笔迹颜色都在、图片不是空位图，并且图片数目正好是四张——IWB 里没有矢量对象这一类，两个内嵌 SVG 只能以位图身份回来（这段查的是 IWB 互操作链路 `IwbDocumentAdaptor`+`IwbSubsetAdaptor`，工具图元不在 IWB 规范内，所以不比它们）。存盘与读回是界面上看不见的一条链路，属性名、命名空间或取整精度任何一侧动一下都只能靠这次往返发现；判据取退出码（非 0 即不一致的条数，75 表示已有实例在跑、按约定让路并跳过）。`tst_snap` 带上 `--snap-selftest`，查的是吸附的另一半：格距、页面背景类型与图元外框是怎么被喂进吸附数学的（空白底两轴都吸、横线稿纸只吸 Y、多角点取吸得力最强的那个、带画笔的图形先按笔宽缩回Qt 本就算进包围盒的那一半），这些要跑起整个应用才看得到，纯几何那条用例覆盖不到。`tst_shortcuts` 带上 `--shortcut-selftest`，先查装配：主窗口的动作是不是都登记进了 `core/ShortcutManager`、默认快捷键是不是还在原来的位置、出厂默认之间有没有两条抢同一个组合键；再换一张自建注册表跑完整回路：改绑能不能落盘、模拟重启后能不能读回来（含“明确不绑键”这种空值不能偷偷变回默认）、恢复默认有没有把配置键真删掉，最后把 `ui/ShortcutSettingsWidget` 拉起来，用选中行、点“应用/清除/恢复所选”这条路验一遍界面上的接线。`tst_cookierules` 只链接 `utils/CookieRules.cpp`，测白名单匹配的两种写法（带点覆盖子域与顶级域、不带点只整串相等）与策略名映射；`tst_packagelayout` 不链任何源文件，它把 `model/PackageLayout.hpp` 里 `.tbz` 包的磁盘字面量钉死（`images`/`objects`/`widgets`/`videos`/`audios`/`files` 与 `folders.xml` 的词表）——往返自检存读用的是同一份常量，改名对它完全自洽，只有钉在字面量上才会红（实测把 `images` 写成 `image`：`tst_format` 全绿，这条红）；`tst_cookies` 带 `--cookie-selftest`，查网络管理器的 Cookie 罐是不是可枚举的 `core/CookieJar`、三项隐私设置按名字读不读得到，再往罐里种三条已知 Cookie，验证“退出会清掉几个”这个计数真的按白名单把顶级域与子域留下、只算不匹配的那一条（把带点规则退回“只比后缀”，这条断言立刻变红）。`tst_import` 带 `--import-selftest`，用一个桩裁决器替用户答题，把导入时文档名撞车的三条路径变成可自动执行的案子：不撞名一次都不该问用户、点取消的那一份必须整包退回磁盘外（旧实现既不登记也不释放，磁盘上留一包没人认领的目录、下次启动还会自己冒出来）、改名两份共存且新名字写回盘上的元数据、同名替换则旧的删干净且画板跟到新文档；另外三案查 .tbx 整批导入那一腿（取消的不许留在磁盘上、改名的要进树）、“没装裁决钩子时不登记也不崩”，以及带新分组的包：删掉分组再导入，分组要连同里面那份文档被 `goTo()` 就地补回来、不许掉进“我的文档”根；最后一案断言删掉当前文档那一行后模型的当前节点真的归零。同一份自检的第 9、10 两条用例把导入这一侧也变成产物对账：用 `QPdfWriter` 现场造一份两页 PDF、用 `QPainter` 现场画一张 PNG，都走界面上同一个入口 `DocumentManager::importFile`，然后断 PDF 导入后是 2 页、每页恰好一个 `GraphicsPDFItem`、文档目录自包含、冷读回来图元还在；图片导入后是 1 页、回来的像素与源文件指纹一致、`images/` 里确有那张 PNG、冷读回来仍然一致（反向对照：把放进场景的位图换成空图 → 3 条红）。另一条反向对照有意思：把导入时那次 `folderToCopy` 短路掉**仍然全绿**——每存一页时 `pdfItemToLinkedPDF` 会把缺的 PDF 重新落盘（位图与 SVG 从这一轮起也是同样写法），所以"目录自包含"钉的是结果不是手段，真正承重的是写出器；这一点在测试代码里注明了，免得后人误读。第 11 案走的是另一条入口——`DocumentManager::addFilesToDocument`，也就是画板里"把文件拖进当前这份文档"那条路（界面上是 `DocumentController` 的追加图片与拖放）：现画一张 PNG 追加进去，断导入报 1 个、页数 +1、新的一页里位图像素与源图同指纹、`images/` 里确有那份文件，并钉住画板那份缩略图列表**只在追加的是自己当前那份文档时**才长一条（往另一份文档追加一次之后它必须不变——旧写法无条件插进画板的容器，被挤位的正是当前这份文档的图，而文档库一侧随后会整份重建缩略图所以看不出来）。两条反向对照各只打红该红的那一条：去掉那次缩略图插入 →"列表 1 条 vs 页数 2"；去掉"是不是当前文档"那道判断 →"被改成 3 条"。还有一条判据差点写错：target 文档最初用 `pageCount=1` 造，看着有 1 页其实没有页文件，`setActiveDocumentScene` 因此根本不切文档，于是"画板当前文档"一直是别人的那份——改成 `withEmptyPage=true` 并加一条"画板必须真的切过去"的前置断言，量的才是这条路。`tst_undoredo` 带 `--undoredo-selftest`，在画板**真实的可撤销场景**里跑整条撤销回路：切到新文档后撤销栈必须是空的、加一张位图应压入 1 条命令且图元出现在场景里、`undo()` 真把它移出场景、`redo()` 放回的是**同一个对象**（按 UUID 找回来、像素指纹一致，不是新建副本，否则资源文件会对不上）、界面上那条删除路（`Item::remove` → 命令持有图元）撤销后图元连内容一起回来、再重做又走掉、文本走同一条回路、`ClearUndoStack()` 之后栈空而场景不受影响、换到另一份文档时栈跟着清空（撤销栈这一轮刚归还给 `BoardController`，最后这条就是它的生命周期钉）。这一条第一次跑就全绿——说明撤销机制本身是好的，但现在有人盯着了：把 `addPixmap` 里那次 `push` 短路，3 条断言立刻变红。`tst_autosave` 带 `--autosave-selftest`，第一次把自动保存量出来：画板装配之后 `BoardController` 名下必须存在且只存在一个正在计时的 `QTimer`，间隔等于设置里的分钟 × 60000；改设置**当场**间隔就得跟着变（这一条刻意不借任何一次模式回调）；设成 0（关掉自动保存）之后不许还有活动的定时器（旧写法在这里直接 `return`，正在跑的定时器不停——用户在偏好设置里关了，画板其实还在按旧间隔往盘上写）；恢复原值后定时器要重新跑起来；再量主模式那一维（切出画板模式定时器要停、切回来要重新跑）；最后调一次公开的 `saveData()`，要求当前页的页文件真的变长。反向对照：把设 0 那段改回旧写法，第三条立刻变红；去掉 `autoSaveInterval` 那条 `changed` 连接（旧实现里重排只写在换主模式的时候，改完间隔要等下一次换模式才落下来），第二条与第三条立刻各变红（"改成 2 分钟之后定时器还是 180000 毫秒"、"设成 0 之后还有 1 个定时器在跑"）。顺带纠正一个我自己判错的事：原本以为"开机就是画板模式的用户根本没有自动保存"，反向对照把在 `init()` 里补的那次启动调用注释掉之后自检仍然全绿——`Application::exec` 结尾本来就会调 `showBoard()`，而每个 `show*()` 都会 `emit mainModeChanged`；那条调用是多余的，已删。`tst_pages` 带 `--pages-selftest`，在真实文档上做"新增两页→复制第 1 页→把第 1 页移到第 3 位→删掉第 2 页"，每一步之后把每一页摊成内容清单比较两份：一份从场景缓存直接拿（内存里的这一页），一份绕开缓存读页文件重建（盘上的这一页）——页操作同时牵着 `pageNNN.svg` 改名、场景缓存搬移与资源复制，错一位不崩不报错，只让某一页悄悄变成另一页。四步全绿；反向对照把 `duplicateDocumentScene` 的改名循环上界少挪一格，立刻 6 处红，其中一条就是"复制之后第 3 页内存里是 PAGE-TWO、盘上是 PAGE-THREE"（下一次自动保存就会把错的内容写进那一页）。对账前必须先 `persistCurrentScene` 把当前页落盘——应用本来就是离开这一页时才写它，不对齐这一步量出来的是假不一致。同一趟现在再加一条**资源归属**判据：每份 `images/<UUID>.png` 只许属于一页，而且必须真在盘上——内容清单看不出"两页共用同一份文件"（像素一样、文本也一样），只有删掉一页时另一页的图才跟着没，而页复制靠的就是 `SvgSubsetAdaptor::duplicateItemResources` 给副本补一份自己的文件并把图元换成新 UUID（这段知识原本抄在 `PersistenceManager` 里，这一轮搬回 content 的写出器）。反向对照：把那个函数开头短路，`tst_pages` 立刻 2 处红（"复制第 1 页之后"与"删掉第 2 页之后"各一句共用同一份包内文件）。`tst_media` 带 `--media-selftest`：现场合成一个合法的小 WAV 放在文档目录之外，分"接口拿到字节"与"只给文件路径"两条分支插进画板，
要求媒体真的成为文档的一部分（图元指向的路径必须在文档目录里且文件存在）、页文件里的引用指得动、存盘再冷读回来的图元数与内容都对。
它修掉的是 `addVideo/addAudio` 的 `bUseSource`：跳过"复制进文档"的那一支正好是拖本地文件那条路，而音视频写出器永远按 
`audios/<文件名>` 这种文档内相对路径写引用——文档因此带着一个指向外部文件的悬空引用，换台机器就放不出来。参数直接删掉（只有一种正确行为时不该留开关）。
反向对照：把复制改回可跳过，2 处红。另一个"看起来像第二个缺陷"的现象（拖桌面 wav 进来毫无反应）其实是同一个原因：
`createMediaItem` 只按路径里的 `audios`/`videos` 认类型；我先加了扩展名判断，再用反向对照发现它在新契约下是死代码，已回退。`tst_recording` 带 `--recording-selftest`，真录一段约两秒的屏幕：录屏这条链路（抓屏定时器 → Windows Media 写入器 → 音频队列）过去只过编译与冒烟，从没产出过真文件。自检要求目录里恰好新增一个 .wmv、不是空壳、开头四个字节是 ASF 容器的 Start Header GUID 前缀（小端是 `30 26 b2 75`，抄成 `30 26 a6 75` 会误判），再交给 Qt 的多媒体后端读一遍并要求时长是正数（实测 2 秒录制读出 2365 ms），产物随手删掉；没麦克风时退化成一趟"只录画面"，那是设计行为，所以音频只作诊断不作失败条件。正是它抓出 `RecordingController::start()` 里两处对未判空的懒建 QAction 解引用——调色板还没建就启动录制必崩。`tst_clipboard` 带 `--clipboard-selftest`，把"复制图元→粘贴→删掉原件→存盘再读回来"这条路整趟跑一遍（以前只过编译）：往画板当前页贴一张位图与一个内嵌 SVG，选中后 `copy()`，断言剪贴板里是两份 **UUID 与自己不同的副本**（旧写法让副本沿用源的 UUID，于是删掉原件时 `clearSource()` 按 UUID 把副本引用的文件一起删了），再 `paste()`，断言这一页上多出两个内容指纹一致的图元；然后按界面上那条删除路（`Item::remove` → `ClearUndoStack` 这个真正的释放点）删掉两张原件，断言盘上只剩副本那两份文件，最后存页、绕开场景缓存冷读回来，要求两份副本都带着原内容回来。这条自检还顺手抓出两处：`ClearUndoStack` 用来源 URL 当"还在剪贴板上"的替身，而**空 URL 也算匹配**，于是剪贴板里只要有一份无来源的图元，所有无来源的已删图元都不再被释放；以及 `ui/DocumentNavigator` 的 `Q_ASSERT(!pix->isNull())` 与 `updateSpecificThumbnail` 里对 `pageAt()` 结果的裸解引用（缩略图缺失时 Debug 直接 abort、Release 空指针）。`tst_export` 带 `--export-selftest`，把导出从"点了没报错"变成产出物对账：造一份两页文档（每页一块不同颜色的位图加一段自己的文字，其中一页设成深色底加网格），两个 PDF 导出器各导一遍，再用 `QPdfDocument` 读回来断页数等于文档页数、每页尺寸等于场景像素 × 72/物理 DPI、每页的文字就是这一页的（不是两页画同一张）、渲染出来确实有墨、导出前后文档页数不许变、深色底与网格背景画完被恢复、`ExportFullPDF` 不许留下 `_overlay.pdf`；再把文档打成 `.tbz` 用导入器读回来逐页比内容指纹，最后拿同一个包答"同名替换"再导入一次——**正是这一条把 `DocumentTreeModel::mCurrentNode` 从没初始化这个崩溃撞出来的**（Debug 下它是 `0xCD…` 填充，`removeRows` 里 `mCurrentNode && …` 恒真，拿它去走父链就是野指针；界面上的触发条件是"重新导入自己刚导出的那份文档"）。反向对照：短路 `pdfWriter.newPage()`，页数断言立刻变红。第 4 条用例再拿一份"声称两页、盘上一页都没有"的文档验导出必须是只读的——这一条把 `PersistenceManager::loadDocumentScene` 改掉了：它过去读不到某一页就当场 `createDocumentSceneAt(proxy, 0)` 补一页、再把第 0 页的内容挂到**请求的那个页号**上，于是同一份文档连导两趟 PDF 页数从 3 涨到 4、"第 N 页"其实是第 1 页。现在只有"一页都没有"才建第 0 页，其余记告警返回空；`ExportPDF` 补上判空跳过，并在真的什么都没画出来时把这件事报给用户。反向对照：把 `loadDocumentScene` 改回旧写法，这条 3 处变红。`tst_download` 带 `--download-selftest`，量的是"把一个放在文档外的文件交给下载队列"这条端到端路与服务层**只报事实**之后有没有人应答：入队时右侧面板那个下载标签页必须出现、文件必须按字节落进这份文档的 `audios/`（不是留在原处、也不是写到别处）、排空后标签页收回且源文件还在；同一趟还数一遍事实各报了几次（入队一次、排空一次、解除黑屏一次——解除只报事实，解冻网页小部件是画板应答的）。三条反向对照各自只打红对应那一条：不装目标文档提供者 → 4 处红（文件根本无处可去）；摘掉画板对下载起止的两条连线 → 只有"标签页没出现"这一处红，下载本身照旧成功；把 `unBlackout()` 里那句 `emit` 去掉 → 只有"黑屏已解除报了几次"这一处红。这一条盯的正是这一轮的改动本身：下载与多屏服务不再反过来命令画板，改成"只报事实 + 装配点连线 + 一个目标文档提供者钩子"，接线丢了界面无动于衷，只有产物对账才看得出来。
`tst_language` 加载 `Language/TianBoard_zh_CN.qm`，断言 `MainWindow` 的 “Preferences / Documents / Undo” 确实取到“设置 / 文档 / 撤销”，断言设置对话框的版本号与关于页文案落在 `PreferencesDialog` 上下文里且译文保留 `%1` 占位符，再断言摘掉翻译器后回落到源文本；它还断言 Qt 自带的 `qtbase_zh_CN.qm` 已随包部署且真能取到词
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
                 PDF 页图形项/渲染器，另有画板自己的两件界面控件 ToolWidget 与 Magnifier
src/content      文档格式读写与导入导出：.tbz/.tbx、IWB 子集与转换、SVG/RDF 元数据、
                 缩略图数据块、PDF 导入导出、图片导入、Web 导出
src/api          小部件与文档对外 API 门面（WebWidget*API）
src/web          内嵌浏览器与网页控制器
src/desktop      屏幕批注（平台无关部分）
src/recording    录屏：画面与声音采集、录制调色板、Windows Media 编码器
src/ui           全部界面控件与调色板
src/platform     平台相关实现（不带界面的部分）：PlatformUtils*、WindowCapture*、
                 KeyboardSender*、CredentialStore*、VirtualDesktop
src/utils        纯工具：StringUtils、GeometryUtils、Version、FileSystemUtils、Snapping、
                 CookieRules、TextTools、AutoSaver、Enums.h…
third/           内联的 quazip/zlib 与 qtsingleapplication
res/             images / etc / library / customizations / style / i18n(.ts) / qrc
tests/           Qt Test 用例（25 个）
docs/            架构说明、编码规范、疑点台账
packaging/       Inno Setup 脚本与打包脚本
scripts/         build.ps1 / format.ps1 / verify.ps1
```

每个模块的 `CMakeLists.txt` 显式列出全部源文件（不用 GLOB），新增文件不改清单就不进构建。
`src/utils`（出边为零）、`src/settings` 与 `src/platform`（都只依赖 utils）、`src/model`
（只依赖 settings 与 utils）、`src/recording`、`src/desktop`、`src/web`、`src/api` 与
`src/content`（后几个没有任何模块反向 include 它们）已经从 `OBJECT` 收紧成真 `STATIC` 库；
`core`、`ui`、`whiteboard` 因为还有 2 组双向 include 而暂时保持 `OBJECT`，拆环顺序见
[docs/architecture.md](docs/architecture.md) 第四、五节。`core` 对 `ui` 的出边已归零：
`Application`/`ApplicationController` 这两个装配角色的实现住在 `src/app`，头文件留在
`src/core` 供各模块拿入口指针。格式层与模型都不再认识 `core`：仓库契约
`model/IDocumentStore` 由 `PersistenceManager` 实现并在构造时装上，"向用户报一句"与
对话框落点走 `utils/UserNotices`，`.tbz` 目录名与 MIME 类型名在 `model`。

代码风格由根目录 `.clang-format` 兜住：全仓（`src/` + `tests/`，388 个文件）已用
clang-format 22.1.8 排过一次，日常提交前跑 `.\scripts\format.ps1 -Check`（0 才是干净）。
`third/` 与 `src/app/resource.h` 永久排除（后者是 ANSI 代码页给 `rc.exe` 读的符号表，
重排或转码会让 `.res` 静默出错），格式化结果绑定 clang-format 版本，升版本要单独一次全量提交。

注释规范见 [docs/coding-style.md](docs/coding-style.md) 第六节：`.h` 与没有对应头文件的 `.cpp`
开头是 `@file` + `@brief`，类型与接口用 Doxygen（中文描述、英文命令），实现文件不重复头文件
的文档、只写关键步骤；被注释掉的旧代码一律不留。
