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
* **不带 `WB`/`UB` 前缀，也不带尾部下划线**。历史名字已清空：`DownloadManager_` →
  `WebDownloadManager`、`ub_message_output` → `messageOutputHandler`、`WB_MAX_ZOOM` →
  `MAX_ZOOM_LEVEL`、`WB_FREE_CONTROL` 宏就地展开后删除、`CFF*` 一族 → `Iwb*`、
  `DemoLibrary` 已不存在、`eWBDock*` 四组枚举改成 `enum class`、`ub*`/`WBZ`/`UBZ` 标识符
  全部按语义改名。新代码不要再起 `WBFoo`/`UBFoo`/`Foo_` 这种名字。
* **文档协议的“值”冻结，只有标识符可以改**。`content/IwbConstants.h` 里
  `tPage*`/`aPage*`/`avPageText`/`svgPageNS` 这些常量的**右值**是写进 `.tbz` 的标签名、
  属性名与命名空间 URI（`"size"`、`"viewBox"`、`"g"`、`IMS 的 iwb_v1p0`……），
  改一个字符就是改格式；`IwbDocumentAdaptor`/`IwbSubsetAdaptor` 里
  `parsePage*`/`*FromPageElement` 是读写这些标签的函数名，同理只能改名字不能改语义。
  本轮的改名严格守住这条线：只动标识符，每个常量的右值一字未改（可用
  `git show` 对照 `IwbConstants.h` 的 diff 验证）。
* 命名空间：**全局类型不加项目前缀命名空间**。改造时把 `WB`/`UB` 前缀去掉后**没有换新的**，
  新标识符同样不加 `WB`/`UB`/`TianBoard` 前缀；确实需要分组时用一个作用域明确的小命名空间
  （已有的例子是 `ThumbnailUI`、`Ui`），不要为“看起来现代”而全仓套一层 `tianboard::`。

## 三、防重复包含

`src/` 下 172 个头文件里 171 个用 `#pragma once`，新增头文件也一律用它，不要再写
`#ifndef` 守卫。

**唯一例外是 `src/app/resource.h`**：它只被 `TianBoard.rc` include，而 `rc.exe` 的
预处理器不认 `#pragma once`。实测把它改成 pragma 之后 `rc.exe` 照样返回 0，但产出的
`.res` 只剩 32 字节，图标与版本信息静默消失——所以它保留 IDE 生成的
`#ifndef APSTUDIO_READONLY_SYMBOLS` 那套守卫，别“顺手统一”。

另外 `src/recording/WindowsMediaFile.h:5` 的 `#ifndef inteface` 不是 include guard
（守卫已经是 `#pragma once`），它包住的是 `#define interface struct` 这个 Windows SDK
老写法，别当残留删掉。

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
