# TianBoard 编码规范

配套文档：[架构说明](architecture.md) · [疑点台账](code-suspicions.md)

本文只写**这个仓库现在遵循什么、新代码必须遵循什么**。规矩尽量由编译器与构建系统兜住：
裸写的同目录 include 编译不过（`CMAKE_INCLUDE_CURRENT_DIR` 是关的），新增源文件不登记
就进不了构建（每个模块显式列文件）。其余靠 review。

---

## 一、include 必须带模块前缀

唯一的写法：`#include "<模块目录>/<文件>.h"`，路径相对 `src/`。

```cpp
#include "core/Application.h"          // 跨模块
#include "whiteboard/BoardController.h"
#include "ui/MainWindow.h"             // 同一个模块内部也这么写
```

* **同目录裸 include（`#include "MainWindow.h"`）不允许**：每条 include 都要说出自己属于
  哪个模块，读者与脚本才能重建模块依赖图（`docs/architecture.md` 的边表就是这么来的）。
* 构建侧兜法：每个模块只有 `target_include_directories(tb_<模块> PRIVATE ${CMAKE_SOURCE_DIR}/src)`，
  且 `CMAKE_INCLUDE_CURRENT_DIR` 关闭；裸写就是 `fatal error C1083`。
* 系统库与 Qt 用尖括号（`#include <QVector>`）；第三方内联库保持它原本的引用方式
  （quazip 裸名、`third/singleapp/...` 长路径），不要顺手改。
* 新增/移动文件：先改 `src/<模块>/CMakeLists.txt` 登记，再写代码。

## 二、文件与命名

* **一个类一对同名文件**：`GraphicsStroke.h` + `GraphicsStroke.cpp`，文件名用
  PascalCase，与主类同名。AUTOMOC 靠这个配对决定 moc 谁。
* **只有头文件实现的（模板、inline-only 工具、接口）用 `.hpp`**。目前全仓 0 个 `.hpp`，
  新增这类文件一律 `.hpp`，别再出现“名字像类、其实是纯头文件”的 `.h`。
  例外：`Globals.h`、`Enums.h`、`MainWindowUi.h`、`IwbConstants.h` 这些是既有纯头文件，
  改名要单独排期（牵动 include 与 moc），不与功能改动混在一起。
* **纯抽象接口用 `I<名字>.h`**：`IDocumentStore.h`、`IThumbnailProvider.h`。
  拆依赖环时抽出来的接口一律走这个命名，一眼能看出它是“只有契约”的头。
  目前全仓还没有这个形式的文件（最接近的是 `platform/WindowCapture.h`），
  后续拆环时按这个名字补。
* **不带 `WB`/`UB` 前缀，也不带尾部下划线**。历史遗留的名字已经清掉一轮
  （`DownloadManager_` → `WebDownloadManager`，`ub_message_output` → `messageOutputHandler`）。
  新代码不要再起 `WBFoo`/`UBFoo`/`Foo_` 这种名字。仍然存在的宏与前缀是例外，属于下一阶段
  改名范围：`WB_FREE_CONTROL`、`DemoLibrary`（`WB_MAX_ZOOM` 已成 `MAX_ZOOM_LEVEL`，
  `CFF*` 一族已成 `Iwb*`）。
* **格式与表名的短大写标识符不算品牌前缀，禁止顺手改名**。`content/IwbConstants.h` 里的
  `tWBZ*`/`aWBZ*`/`avWBZ*`/`tUBZG`/`pageFileExtentionUBZ` 这类常量是 IWB/Sankoré 文档沿用下来的
  **XML 标签名与属性名**（值分别是 `"size"`、`"viewBox"`、`"g"`、`"svg"` 等），
  `svgWBZNS` 更是 IMS Global 的 `iwb_v1p0` 命名空间；`IwbDocumentAdaptor`/`IwbSubsetAdaptor`
  里成片的 `parseWBZ*`/`*FromUBZ` 是读写这些标签的函数名。它们和文档内容逐字节对应，改了
  就是改协议。改名只改包裹它们的类与文件（`CFFAdaptor` → `IwbDocumentAdaptor` 这一轮，
  这些 token 一字未动：每个文件行数与 `WBZ/UBZ/UBX` 出现次数 21/83/2 都保持原样）。
* 命名空间：**全局类型不加项目前缀命名空间**。改造时把 `WB`/`UB` 前缀去掉后**没有换新的**，
  新标识符同样不加 `WB`/`UB`/`TianBoard` 前缀；确实需要分组时用一个作用域明确的小命名空间
  （已有的例子是 `ThumbnailUI`、`Ui`），不要为“看起来现代”而全仓套一层 `tianboard::`。

## 三、include guard

形如 `TIANBOARD_<PATH>_<FILE>_H`，其中 `<FILE>` 是文件名大写在最外层，`<PATH>` 为
可选的模块段。现存 173 个头的实际形态是 `TIANBOARD_<FILE>_H`（例：
`TIANBOARD_BLACKOUTWIDGET_H`），因为全仓头文件名唯一，不带模块段也不歧义。

```cpp
#ifndef TIANBOARD_BOARDCONTROLLER_H
#define TIANBOARD_BOARDCONTROLLER_H
...
#endif // TIANBOARD_BOARDCONTROLLER_H
```

* `#endif` 后面带 guard 名注释；
* guard 以下划线结尾的旧写法已全部清掉，新文件不要再产生；
* 两个例外要知道：`app/resource.h`（IDE 生成的 `APSTUDIO` guard，别动）与
  `recording/WindowsMediaFile.h`（`#ifndef inteface`，见疑点台账）；
* 新头文件如果与 `third/` 里的头同名（`ioapi.h`、`unzip.h`、`zip.h`、`zconf.h`……），
  必须加上模块段：`TIANBOARD_CONTENT_ZIP_H`。

## 四、Pimpl

已有 77 个类转成了 Pimpl（分布在 47 个头文件里，少数头文件带多个类），统一用这个形态
（不要另发明一套）：

```cpp
// .h —— 对外只暴露指针类型，不带任何实现相关的 include
class BlackoutWidget : public QWidget
{
    Q_OBJECT
public:
    BlackoutWidget(QWidget *parent = nullptr);
    ~BlackoutWidget() override;

private:
    struct Private;                    // 嵌套前置声明，外部不可见
    std::unique_ptr<Private> d;        // 成员名固定用 d
};

// .cpp —— 定义放在使用之前，构造用 std::make_unique
struct BlackoutWidget::Private
{
    QGridLayout* gridLayout{nullptr};
    QToolButton* iconButton{nullptr};
};
```

规则：
* 成员固定叫 `d`；嵌套类型固定叫 `Private`；所有权用 `std::unique_ptr`（不要
  `QScopedPointer`，也不要有裸 `new`）。
* 头文件里**不许**出现实现细节的 include（Qt 控件、第三方头）。需要类型时前置声明。
* 析构函数必须在 `.cpp` 里定义（`= default` 也要写出来），否则 `unique_ptr` 删除
  不完整类型直接编译失败。
* 带 `Q_OBJECT` 的宿主类，其头文件必须列进所属模块的 `CMakeLists.txt` 源文件清单——
  AUTOMOC 只自动配对同名 `.h`/`.cpp`，头文件不列上就不生成 moc，信号槽在运行期**静默失效**
  （编译链接一句话都不报）。现存唯一需要特别当心的例子：`platform/WindowCapture.h` 有
  `Q_OBJECT`，实现却在 `platform/WindowCapture_win.cpp`，没有 `WindowCapture.cpp`，
  因此它必须被显式列出。其余头文件与 `.cpp` 都是同名配对，跨模块实现宿主类的情况现在是 0 例；
  列头文件的规矩不要为了“看起来多余”而删掉。
* 哪些类该 Pimpl：**对外只该有接口、内部状态会牵动重编**的界面控件与控制器。
  纯数据/绘图热路径的小对象不必（`GraphicsStroke` 那种，一次 include 传播换来的是
  内联损失）。别为了统一而统一。

## 五、其它既有约定

* C++17，MSVC2022；源码里有中文注释，编译一律 `/utf-8`，文件保存 UTF-8（无 BOM，
  `.rc` 是 UTF-16 例外，用工具改，别用文本编辑器）。
* `.ui` 表单已全部改成手写 C++，**不要再引入 `.ui`**；可翻译文案集中在一个
  `retranslateUi()` 里（见 `ui/MainWindowUi.h`、`PreferencesDialog::Ui`），
  否则语言切换功能覆盖不到新界面。
* 换行混用 CRLF/LF：`.gitattributes` 里 `* -text` 禁止任何自动转换，提交前不要跑
  全局换行符“整理”。`scripts/format.ps1` 只按 `.clang-format` 排空白，不动行尾。
* 日志：`qDebug()/qWarning()/qCritical()`；应用自己会写用户目录下的 log
  （`main.cpp` 的 `messageOutputHandler`），不要用 `std::cout`。
* 字符串：面向用户的文案一律 `tr()`，原文不许为了编译而过改；`.ts` 文件由
  `lupdate`/`lrelease` 管线管理，手工改动前先确认是不是抽取覆盖不到的地方。
