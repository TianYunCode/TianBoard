/**
 * @file GraphicsWidgetItem.h
 * @brief 声明可嵌入画板的 W3C / Apple 小部件图元及其脚本门面数据载体。
 */

#pragma once

#include <QtWidgets>
#include <QtWebEngineWidgets>
#include <QWebEngineView>
#include <QDomElement>
#include <QGraphicsView>
#include <QGraphicsWidget>
#include "utils/Enums.h"

#include "whiteboard/Item.h"
#include "whiteboard/ResizableGraphicsItem.h"

class IBoardDropHandler;
class GraphicsScene;

/**
 * @brief 小部件包的种类，由 FileSystemUtils::mimeTypeFromFileName() 得到的 MIME 决定。
 */
struct WidgetType
{
    enum Enum
    {
        Web = 0, ///< W3C 小部件（.widget + config.xml）
        Apple,   ///< Apple 桌面小部件（.wdgt + Info.plist）
        Other    ///< 认不出来，按普通 HTML 处理
    };
};

/**
 * @brief 画板上的一个可嵌入小部件。
 *
 * 三个基类分工：QGraphicsWidget 提供 Qt 的部件布局，Item 提供白板图元契约（UUID、
 * 深拷贝、层级），ResizableGraphicsItem 声明尺寸读写。交互外观全部交给
 * GraphicsWidgetItemDelegate。
 *
 * 内容呈现走“快照”路线：freeze() 把当前画面拍成 mSnapshot，unFreeze() 恢复，导出与
 * 缩略图都只用这份位图，不做实时渲染。
 *
 * @warning 加载完成回调在 Qt6 的 WebEngine 迁移中没有接回来，mInitialLoadDone 恒为假，
 *          paint() 因此永远停留在“加载中”分支；size() 也恒返回空尺寸，resize() 是空的。
 *          这条链路的取舍记在 docs/code-suspicions.md，改之前先读它。
 *
 * 所有权：mDropHandler 由 app/main.cpp 装入的工厂创建，以场景为父对象，不由本图元释放。
 *
 * @note 线程约束：只能在 GUI 线程创建与操作。
 */
class GraphicsWidgetItem : public QGraphicsWidget, public Item, public ResizableGraphicsItem, public GraphicsItem
{
    Q_OBJECT
    Q_INTERFACES(QGraphicsItem);

public:
    /**
     * @brief 记下小部件包的位置并置好图层与事件标志。
     * @param pWidgetUrl 小部件包的目录 URL。
     * @param parent 父图元。
     */
    GraphicsWidgetItem(const QUrl &pWidgetUrl = QUrl(), QGraphicsItem *parent = 0);
    ~GraphicsWidgetItem();

    /// @brief 图元类型标识，供 qgraphicsitem_cast 与分组判定使用。
    enum
    {
        Type = GraphicsItemType::GraphicsWidgetItemType
    };

    /// @brief 返回 Type。
    virtual int type() const
    {
        return Type;
    }

    /// @brief 把最小尺寸设为名义尺寸并按可缩放与否配置委托边框的操作模式。
    virtual void initialize();

    /// @brief 按宽高缩放（当前是空实现，见类注释的警告）。
    virtual void resize(qreal w, qreal h);
    /// @brief 按尺寸缩放（当前是空实现）。
    virtual void resize(const QSizeF &size);
    /// @brief 恒返回空尺寸：小部件不走基于 size() 的缩放链路。
    virtual QSizeF size() const;

    /// @brief 小部件的入口 HTML 地址。
    QUrl mainHtml();
    /// @brief 重新加载入口 HTML，并把加载标记清回“未完成”。
    void loadMainHtml();
    /// @brief 小部件包目录的 URL。
    QUrl widgetUrl();
    /// @brief 改指到另一个小部件包目录。
    void widgetUrl(QUrl url)
    {
        mWidgetUrl = url;
    }
    /// @brief 入口 HTML 的文件名（不含目录）。
    QString mainHtmlFileName();

    /// @brief 该小部件声明自己能在 Windows 上当内容用。
    bool canBeContent();
    /// @brief 该小部件声明自己能在 Windows 上当工具用。
    bool canBeTool();

    /// @brief 读一条 preference（只在本会话有效，不落盘）。
    QString preference(const QString &key) const;
    /// @brief 写一条 preference；值没变时直接返回，避免无谓的落盘。
    void setPreference(const QString &key, QString value);
    /// @brief 全部 preference。
    QMap<QString, QString> preferences() const;
    /// @brief 删掉一条 preference。
    void removePreference(const QString &key);
    /// @brief 清空 preference（不触发保存）。
    void removeAllPreferences();

    /// @brief 读一条 datastore 条目（随文档一起持久化）。
    QString datastoreEntry(const QString &key) const;
    /// @brief 写一条 datastore 条目。
    void setDatastoreEntry(const QString &key, QString value);
    /// @brief 全部 datastore 条目。
    QMap<QString, QString> datastoreEntries() const;
    /// @brief 删掉一条 datastore。
    void removeDatastoreEntry(const QString &key);
    /// @brief 清空 datastore（不触发保存）。
    void removeAllDatastoreEntries();

    /// @brief 处理拖进小部件的数据：首次调用时才创建脚本门面。
    void processDropEvent(QGraphicsSceneDragDropEvent *event);

    /// @brief 小部件解包后所在的目录。
    virtual QUrl getOwnFolder() const;
    /// @brief 设置解包目录。
    virtual void setOwnFolder(const QUrl &newFolder);
    /// @brief 设置快照文件的落点。
    virtual void setSnapshotPath(const QUrl &newFilePath);
    /// @brief 快照文件的落点。
    virtual QUrl getSnapshotPath();

    /// @brief 清除外部来源信息（URL 与快照文件）。
    virtual void clearSource();

    /// @brief 设置 UUID，同时写进图元数据，供场景按 UUID 找回图元。
    virtual void setUuid(const QUuid &pUuid);

    /// @brief 小部件声明的原始尺寸，缩放与导出都以它为基准。
    QSize nominalSize() const;

    /// @brief 能否冻结成位图（由 config.xml 的 tb:freezable 决定）。
    bool freezable();
    /// @brief 是否允许用户缩放。
    bool resizable();
    /// @brief 当前是不是冻在位图状态。
    bool isFrozen();

    /// @brief 当前缓存的快照。
    QPixmap snapshot();
    /// @brief 塞进一份快照（载入文档时用，避免重新渲染）。
    void setSnapshot(const QPixmap &pix);
    /// @brief 现场渲染一份快照并缓存下来。
    QPixmap takeSnapshot();

    /// @brief 深拷贝出配置相同的新小部件图元；由具体种类实现。
    virtual Item *deepCopy() const = 0;
    /// @brief 所属场景，不在场景中时为空指针。
    virtual GraphicsScene *scene();

    /// @brief 按包名判断小部件种类，返回 WidgetType 里的值。
    static int widgetType(const QUrl &pUrl);
    /// @brief 从 config.xml 或 Info.plist 读出显示名。
    static QString widgetName(const QUrl &pUrl);
    /// @brief 按规范允许的候选名找出图标文件，都没有则用缺省图标。
    static QString iconFilePath(const QUrl &pUrl);

public slots:
    /// @brief 冻结成位图：之后只画快照，不再碰内嵌页面。
    void freeze();
    /// @brief 解冻，恢复可交互状态。
    void unFreeze();

protected:
    /// @brief config.xml 的 roles 属性用到的位标志，表示小部件声明支持哪些平台。
    enum OSType
    {
        type_NONE = 0, // 0000
        type_WIN  = 1, // 0001
        type_MAC  = 2, // 0010
        type_UNIX = 4, // 0100
        type_ALL  = 7  // 0111
    };

    bool                   mFirstReleaseAfterMove; ///< 刚做完整体移动，本次松开不算“结束拖动”
    bool                   mInitialLoadDone;       ///< 首次加载是否完成；当前没有任何路径把它置真
    bool                   mIsFreezable;           ///< config.xml 允许冻结时才真正拍快照
    bool                   mIsResizable;           ///< 是否允许用户拖动边角缩放
    bool                   mMouseIsPressed;        ///< 左键是否仍按在小部件上
    int                    mCanBeContent;          ///< roles 解析出的“可作内容”平台位标志
    int                    mCanBeTool;             ///< roles 解析出的“可作工具”平台位标志
    QSize                  mNominalSize;           ///< 小部件声明的原始尺寸
    QString                mMainHtmlFileName;      ///< 入口 HTML 的文件名
    QUrl                   mMainHtmlUrl;           ///< 入口 HTML 的完整地址
    QUrl                   mWidgetUrl;             ///< 小部件包目录
    QMap<QString, QString> mDatastore;             ///< 随文档持久化的脚本存储
    QMap<QString, QString> mPreferences;           ///< 仅本会话有效的脚本配置

    virtual bool event(QEvent *event);
    virtual void dropEvent(QGraphicsSceneDragDropEvent *event);
    /// @brief 委托没吃掉按下时强制选中，并按内嵌页面是否接收决定能否整体拖动。
    virtual void mousePressEvent(QGraphicsSceneMouseEvent *event);
    virtual void mouseReleaseEvent(QGraphicsSceneMouseEvent *event);
    virtual void hoverEnterEvent(QGraphicsSceneHoverEvent *event);
    virtual void hoverLeaveEvent(QGraphicsSceneHoverEvent *event);
    virtual void wheelEvent(QGraphicsSceneWheelEvent *event);
    /// @brief 只画“加载中”提示与委托的后绘内容，内容本身靠快照显示。
    virtual void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = 0);

private:
    bool               mIsFrozen;         ///< 是否冻在位图状态
    bool               mIsTakingSnapshot; ///< 正在拍快照，避免 paint 递归重入
    bool               mShouldMoveWidget; ///< 本次按下是否应当整体拖动图元
    IBoardDropHandler *mDropHandler;      ///< 脚本与拖放门面，以场景为父对象，不由本图元释放
    QPixmap            mSnapshot;         ///< 冻结用的位图缓存
    QPointF            mLastMousePos;     ///< 上一次鼠标位置，按场景坐标记
    QUrl               mOwnFolder;        ///< 解包后的工作目录
    QUrl               mSnapshotFilePath; ///< 快照落盘位置
};

/**
 * @brief Apple 桌面小部件（.wdgt）。
 *
 * 元数据从二进制 Info.plist 里按字节位置抠出来（MainHTML、尺寸、图标），没有走 plist
 * 解析器；包内是 HTML 片段而非完整页面，因此用一份内置的包装页承载。
 */
class GraphicsAppleWidgetItem : public GraphicsWidgetItem
{
    Q_OBJECT

public:
    /// @brief 解析 .wdgt 包，读出入口文件、名义尺寸与图标。
    GraphicsAppleWidgetItem(const QUrl &pWidgetUrl, QGraphicsItem *parent = 0);
    ~GraphicsAppleWidgetItem();

    /// @brief 把包装页与尺寸信息复制给副本。
    virtual void copyItemParameters(Item *copy) const;
    /// @brief 只落到 Item 的实现，跳过 GraphicsWidgetItem 的图元数据写入。
    virtual void setUuid(const QUuid &pUuid);
    /// @brief 复制出一个同包的 Apple 小部件。
    virtual Item *deepCopy() const;
    QRectF        boundingRect() const override;
};

/**
 * @brief W3C 小部件（.widget + config.xml）。
 *
 * 解析 config.xml 得到入口页、尺寸、features/roles、preference 与作者信息；需要插件的
 * 内容再经 NPAPI 包装页承载（Qt6 下已无插件接口，见 docs/code-suspicions.md）。
 * 脚本通道（消息、存储）由 api 层的门面通过 IBoardDropHandler 接上。
 */
class GraphicsWebWidgetItem : public GraphicsWidgetItem
{
    Q_OBJECT

public:
    /// @brief config.xml 里一条 preference 的值与其只读标志。
    class PreferenceValue
    {
    public:
        PreferenceValue()
        {
        }

        PreferenceValue(const QString &pValue, bool pReadonly)
        {
            value    = pValue;
            readonly = pReadonly;
        }

        bool    readonly; ///< 小部件声明该 preference 不允许脚本改写
        QString value;    ///< 当前值
    };

    /// @brief config.xml 里的作者与版本等元信息。
    class Metadata
    {
    public:
        QString id;          ///< 小部件标识
        QString name;        ///< 显示名
        QString description; ///< 简介
        QString author;      ///< 作者
        QString authorEmail; ///< 作者邮箱
        QString authorHref;  ///< 作者主页
        QString version;     ///< 版本串
    };

    /// @brief 解析 config.xml，准备入口页与包装页。
    GraphicsWebWidgetItem(const QUrl &pWidgetUrl, QGraphicsItem *parent = 0);
    ~GraphicsWebWidgetItem();

    /// @brief 只落到 Item 的实现，跳过 GraphicsWidgetItem 的图元数据写入。
    virtual void setUuid(const QUuid &pUuid);
    /// @brief 复制出一个同包、同配置、同存储状态的 Web 小部件。
    virtual Item *deepCopy() const;
    /// @brief 把入口页、尺寸、preference 与 datastore 复制给副本。
    virtual void copyItemParameters(Item *copy) const;
    /// @brief config.xml 声明的 preference（含只读标志）。
    QMap<QString, PreferenceValue> preferences();
    /// @brief 解析到的作者与版本信息。
    Metadata metadatas() const;
    QRectF   boundingRect() const override;

    /// @brief 冻结态包装页在磁盘上的路径。
    static QString freezedWidgetFilePath();
    /// @brief 读出冻结态包装页的 HTML 文本（只读一次并缓存）。
    static QString freezedWidgetPage();
    /// @brief 该 MIME 类型是否备有 NPAPI 包装模板。
    static bool hasNPAPIWrapper(const QString &pMimeType);
    /// @brief 为需要插件的内容生成包装页，返回可加载的 URL。
    static QString createNPAPIWrapper(
        const QString &url, const QString &pMimeType = QString(), const QSize &sizeHint = QSize(300, 150), const QString &pName = QString());
    /// @brief 在指定目录里写出 NPAPI 包装页。
    static QString createNPAPIWrapperInDir(const QString &url, const QDir &pDir, const QString &pMimeType = QString(),
        const QSize &sizeHint = QSize(300, 150), const QString &pName = QString());
    /// @brief 在指定目录里写出一段 HTML 作为包装页，返回其路径。
    static QString createHtmlWrapperInDir(const QString &html, const QDir &pDir, const QSize &sizeHint, const QString &pName);

    Metadata mMetadatas; ///< 解析到的元信息，公开以便脚本门面与导出直接取用

private:
    static void    loadNPAPIWrappersTemplates();
    static QString textForSubElementByLocale(QDomElement rootElement, QString subTagName, QLocale locale);

    QMap<QString, PreferenceValue> mPreferences;

    static bool                   sTemplateLoaded;
    static QString                sNPAPIWrappperConfigTemplate;
    static QMap<QString, QString> sNPAPIWrapperTemplates;
};
