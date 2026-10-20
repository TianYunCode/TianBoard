/**
 * @file WebWidgetBoardAPI.h
 * @brief 声明注入网页小部件脚本环境的画板门面、Web Storage 形状与文档存储适配层。
 */

#pragma once

#include <QtCore>
#include <QGraphicsSceneDragDropEvent>

#include "whiteboard/IBoardDropHandler.h"
#include "core/DownloadManager.h"

class GraphicsScene;
class GraphicsWidgetItem;
class GraphicsWebWidgetItem;

class WebWidgetMessageAPI;
class DatastoreAPI;
class DocumentDatastoreAPI;

/**
 * @brief 网页小部件脚本与画板之间的 API 门面。
 *
 * 位于 api 层：脚本侧通过 QWebChannel 取得本对象，调用 setTool/drawLineTo 等槽，
 * 由门面翻译成 GraphicsScene 与 GraphicsWidgetItem 上的具体操作。
 * 生命周期由所属的 GraphicsWebWidgetItem 管理，与网页小部件同生共死；
 * 同时实现 whiteboard 的 IBoardDropHandler，拖放实现由装配点装上（见 app/main.cpp）。
 * 所有槽与属性读取均在 GUI 线程执行，内部不做加锁。
 */
class WebWidgetBoardAPI : public QObject, public IBoardDropHandler {
    Q_OBJECT

    Q_PROPERTY(int pageCount READ pageCount SCRIPTABLE true)

    Q_PROPERTY(int currentPageNumber READ currentPageNumber SCRIPTABLE true)

    Q_PROPERTY(QString uuid READ uuid SCRIPTABLE true)

    Q_PROPERTY(QString lang READ lang SCRIPTABLE true)

    Q_PROPERTY(QObject *messages READ messages SCRIPTABLE true)

    Q_PROPERTY(QObject *datastore READ datastore SCRIPTABLE true)

public:
    /**
     * @brief 构造门面，绑定场景与可选的小部件。
     * @param pScene 门面操作的画板场景，可为空，之后用 setScene() 补上。
     * @param widget 触发本 API 的图形小部件，用于反查网页对象与下载归属。
     */
    WebWidgetBoardAPI(GraphicsScene *pScene, GraphicsWidgetItem *widget = 0);
    /** @brief 销毁消息与数据存储子对象。 */
    ~WebWidgetBoardAPI();

    /** @brief 返回脚本侧的消息通道对象（WebWidgetMessageAPI），首次访问时创建。 */
    QObject *messages();

    /** @brief 返回脚本侧的文档存储对象（DatastoreAPI），首次访问时创建。 */
    QObject *datastore();

public slots:
    /** @brief 切换门面绑定的场景，供小部件在页面间迁移时复用。 */
    void setScene(GraphicsScene *pScene) { mScene = pScene; }

    /** @brief 切换当前绘制工具；工具名采用脚本侧约定的字符串。 */
    void setTool(const QString &toolString);

    /** @brief 设置钢笔颜色，参数为脚本可解析的颜色字符串。 */
    void setPenColor(const QString &penColor);

    /** @brief 设置荧光笔颜色，参数为脚本可解析的颜色字符串。 */
    void setMarkerColor(const QString &penColor);

    /** @brief 返回指定页的缩略图数据；页码非法时返回空字符串。 */
    QString pageThumbnail(const int pageNumber);

    /** @brief 以场景坐标 (x, y) 为锚点按 factor 缩放视图。 */
    void zoom(const qreal factor, const qreal x, const qreal y);

    /** @brief 视图按给定偏移量相对平移。 */
    void move(const qreal x, const qreal y);

    /** @brief 将视图中心移动到给定场景坐标。 */
    void moveTo(const qreal x, const qreal y);

    /** @brief 在当前笔画上追加一段到 (x, y)，pWidth 为笔宽。 */
    void drawLineTo(const qreal x, const qreal y, const qreal pWidth);

    /** @brief 沿橡皮轨迹追加一段到 (x, y)，pWidth 为擦除半径。 */
    void eraseLineTo(const qreal x, const qreal y, const qreal pWidth);

    /** @brief 清空当前页内容。 */
    void clear();

    /** @brief 设置背景：pIsDark 表示深色底，pIsCrossed 表示是否绘制网格线。 */
    void setBackground(bool pIsDark, bool pIsCrossed);

    /**
     * @brief 在指定位置添加一个网页对象。
     * @param pUrl 对象地址；尺寸为 0 时由小部件按默认尺寸显示。
     * @param background 为 true 时作为背景层插入。
     */
    void addObject(QString pUrl, int width = 0, int height = 0, int x = 0, int y = 0,
                   bool background = false);

    /** @brief 调整小部件在场景中的宽高。 */
    void resize(qreal width, qreal height);

    /** @brief 返回当前界面语言标识。 */
    QString locale();

    /** @brief 写入一项偏好设置。 */
    void setPreference(const QString &key, QString value);

    /** @brief 读取一项偏好设置，键不存在时返回 pDefault。 */
    QString preference(const QString &key, const QString &pDefault = QString());

    /** @brief 列出全部偏好设置的键。 */
    QStringList preferenceKeys();

    /** @brief 在画板浮层中显示一条提示消息。 */
    void showMessage(const QString &message);

    /** @brief 将视图中心移动到场景坐标 (x, y)。 */
    void centerOn(const qreal x, const qreal y);

    /**
     * @brief 在当前页插入一段文本。
     * @param height 为 -1 时按默认字号处理。
     * @param font 为空时使用默认字体。
     */
    void addText(const QString &text, const qreal x, const qreal y, const int height = -1,
                 const QString &font = "", bool bold = false, bool italic = false);

    /** @brief 向脚本回报某个方法的执行状态。 */
    void returnStatus(const QString &method, const QString &status);
    /** @brief 向脚本回报其使用过的方法列表。 */
    void usedMethods(QStringList methods);
    /** @brief 向脚本回报一次作答的对错结果。 */
    void response(bool correct);

    /** @brief 把下载文件的元数据推送给脚本。 */
    void sendFileMetadata(QString metaData);

    /** @brief 开关小部件是否接受拖放，默认开启。 */
    void enableDropOnWidget(bool enable = true);

    /** @brief 判断给定的 MIME 数据是否可拖入小部件。 */
    bool isDropableData(const QMimeData *pMimeData) const;

public:
    /**
     * @brief IBoardDropHandler 接口：把拖到小部件上的数据导入画板。
     *
     * 画板只依赖接口，具体装配在 app/main.cpp 完成，因此本类不直接持有拖放策略。
     */
    void processDropEvent(QGraphicsSceneDragDropEvent *event) override;

private slots:
    /** @brief 下载完成后按文件类型决定导入画板还是交给脚本。 */
    void onDownloadFinished(bool pSuccess, sDownloadFileDesc desc, QByteArray pData);

private:
    /** @brief 登记一个正在下载的请求 ID。 */
    inline void registerIDWidget(int id) { webDownloadIds.append(id); }
    /** @brief 取出并移除下载请求 ID，命中时返回 true。 */
    inline bool takeIDWidget(int id);

private:
    /** @brief 当前文档的 UUID。 */
    QString uuid();

    /** @brief 当前界面语言。 */
    QString lang();

    /** @brief 画板总页数。 */
    int pageCount();

    /** @brief 当前页页码。 */
    int currentPageNumber();
    /** @brief 返回存放网页对象数据的目录。 */
    QString getObjDir();
    /** @brief 构造拖放/下载时使用的 MIME 文本。 */
    QString createMimeText(bool downloaded, const QString &mimeType, const QString &fileName);
    /** @brief 判断 MIME 头是否属于受支持的类型。 */
    bool supportedTypeHeader(const QString &) const;
    /** @brief 把布尔值转成脚本侧使用的字符串。 */
    QString boolToStr(bool value) const { return value ? "true" : "false"; }

    GraphicsScene *mScene; ///< 门面操作的目标场景，可为空。

    GraphicsWidgetItem *mGraphicsWidget; ///< 触发本 API 的图形小部件。

    bool mIsVisible; ///< 小部件当前是否可见。

    WebWidgetMessageAPI *mMessagesAPI; ///< 延迟创建的消息通道。

    DatastoreAPI *mDatastoreAPI; ///< 延迟创建的文档存储。
    QList<int> webDownloadIds;   ///< 本小部件发起、尚未完成的下载请求 ID。
};

/**
 * @brief 小部件脚本可见的 Web Storage 形状。
 *
 * 纯接口层：只声明 localStorage 式的 key/value 访问面，具体存取由
 * DocumentDatastoreAPI 落在图形对象自己身上。脚本按 localStorage 的用法调用，
 * length 通过属性暴露，因此实现类必须保证计数与内容一致。
 */
class WebWidgetStorage : public QObject {
    Q_OBJECT

    Q_PROPERTY(int length READ length SCRIPTABLE true);

public:
    /** @brief 构造空的存储接口对象。 */
    WebWidgetStorage(QObject *parent = nullptr) : QObject(parent) { /* 刻意留空 */ }
    /** @brief 虚析构，允许经基类指针释放具体实现。 */
    virtual ~WebWidgetStorage() { /* 刻意留空 */ }

public slots:
    /** @brief 返回第 index 个键名，越界时返回空字符串。 */
    virtual QString key(int index) = 0;
    /** @brief 读取键对应的值，不存在时返回空字符串。 */
    virtual QString getItem(const QString &key) = 0;
    /** @brief 写入键值；空值语义交由实现决定。 */
    virtual void setItem(const QString &key, const QString &value) = 0;
    /** @brief 删除指定键。 */
    virtual void removeItem(const QString &key) = 0;
    /** @brief 清空全部键值。 */
    virtual void clear() = 0;

protected:
    /** @brief 返回已存储的键数量。 */
    virtual int length() = 0;
};

/**
 * @brief 向脚本暴露 document 属性的存储门面。
 *
 * 由 WebWidgetBoardAPI::datastore() 延迟创建并持有，实际数据落在
 * DocumentDatastoreAPI 中，生命周期随所属网页小部件。
 */
class DatastoreAPI : public QObject {
    Q_OBJECT

    Q_PROPERTY(QObject *document READ document SCRIPTABLE true)

public:
    /** @brief 为一个网页小部件构造存储门面。 */
    DatastoreAPI(GraphicsWebWidgetItem *widget);
    /** @brief 释放内部的文档存储实现。 */
    virtual ~DatastoreAPI() { ; }

    /** @brief 返回当前文档的存储对象。 */
    QObject *document();

private:
    DocumentDatastoreAPI *mDocumentDatastore; ///< 文档级键值存储实现。
};

/**
 * @brief 把 Web Storage 语义落到某个网页小部件对应的文档数据上。
 *
 * 键值随图形对象持久化，因此同一文档重新打开后仍可读到；
 * 本类不负责跨文档共享，实例生命周期与所属小部件一致。
 */
class DocumentDatastoreAPI : public WebWidgetStorage {
    Q_OBJECT

public:
    /** @brief 绑定一个网页小部件作为数据宿主。 */
    DocumentDatastoreAPI(GraphicsWebWidgetItem *graphicsWidget);

    /** @brief 析构，不额外释放宿主对象。 */
    virtual ~DocumentDatastoreAPI();

public slots:
    /** @brief 返回第 index 个键名，越界时返回空字符串。 */
    virtual QString key(int index);
    /** @brief 读取键对应的值。 */
    virtual QString getItem(const QString &key);
    /** @brief 写入键值。 */
    virtual void setItem(const QString &key, const QString &value);
    /** @brief 删除指定键。 */
    virtual void removeItem(const QString &key);
    /** @brief 清空全部键值。 */
    virtual void clear();

protected:
    /** @brief 返回已存储的键数量。 */
    virtual int length();

private:
    GraphicsWebWidgetItem *mGraphicsWebWidget; ///< 数据的宿主对象，不拥有所有权。
};
