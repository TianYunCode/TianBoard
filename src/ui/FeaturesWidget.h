/**
 * @file FeaturesWidget.h
 * @brief 提供资源库（Features）停靠面板及模型、视图、拖放与属性页等配套控件。
 */

#pragma once

#include <QRegExp>
#include <QRegularExpression>

#include <QWidget>
#include <QListView>
#include <QGraphicsView>
#include <QAbstractListModel>
#include <QPixmap>
#include <QVBoxLayout>
#include <QSlider>
#include <QUrl>
#include <QStyledItemDelegate>
#include <QLocale>
#include <QGraphicsLinearLayout>
#include <QStackedWidget>
#include <QDropEvent>
#include <QPushButton>
#include <QProgressBar>
#include <QLabel>
#include <QTreeWidget>

#include <memory>

#include "ui/DockPaletteWidget.h"
#include "settings/Settings.h"
#include "whiteboard/FeaturesController.h"
#include "ui/FeaturesActionBar.h"
#include "ui/RubberBand.h"
#include <QtWebEngineWidgets>
#include <QWebEngineView>
#include <QWebEngineSettings>

/** 属性页原始缩略图的显示宽度。 */
#define THUMBNAIL_WIDTH 400
/** 中央堆叠控件中主列表页的下标。 */
#define ID_LISTVIEW 0
/** 中央堆叠控件中属性页的下标。 */
#define ID_PROPERTIES 1
/** 中央堆叠控件中网页视图页的下标。 */
#define ID_WEBVIEW 2

class ListModel;

class FeaturesModel;
class FeaturesItemDelegate;
class FeaturesPathItemDelegate;
class FeaturesProxyModel;
class FeaturesSearchProxyModel;
class FeaturesPathProxyModel;
class FeaturesPathViewer;
class FeatureProperties;
class FeatureItemButton;
class FeaturesListView;
class FeaturesWebWidget;
class FeaturesNavigatorWidget;
class FeaturesMimeData;
class FeaturesCentralWidget;
class FeaturesNewFolderDialog;
class FeaturesProgressBar;


/**
 * @brief 资源库停靠面板，右侧 DockPalette 的“资源库”标签页。
 *
 * 界面顶部是路径面包屑，中间是条目缩略图/属性页/网页视图，底部是动作条。
 * BoardPaletteManager 创建后注册到 RightPalette，在 Board 与 Desktop 模式可见。
 * 面板持有 FeaturesController，监听扫描进度与下载完成等信号，并把动作条的请求
 * 转成对控制器的操作；条目模型由控制器提供，本类负责视图层交互。
 */
class FeaturesWidget : public DockPaletteWidget {
    Q_OBJECT
public:
    /**
     * @brief 构造资源库面板：控制器、路径列表、中央控件与动作条。
     * @param parent 父控件
     * @param name 对象名
     */
    FeaturesWidget(QWidget *parent = 0, const char *name = "FeaturesWidget");
    virtual ~FeaturesWidget();

    /**
     * @brief 资源库在 Board 与 Desktop 模式显示。
     * @param mode 当前界面模式
     * @return 是否可见
     */
    bool visibleInMode(DockWidgetMode mode) {
        return mode == DockWidgetMode::Board || mode == DockWidgetMode::Desktop;
    }
    /** @brief 返回面板持有的资源库控制器。 */
    FeaturesController *getFeaturesController() const;
    /**
     * @brief 把图像导入资源库。
     * @param image 图像
     * @param fileName 目标文件名，空则自动命名
     */
    void importImage(const QImage &image, const QString &fileName = QString());

    static const int minThumbnailSize = 20;
    static const int maxThumbnailSize = 100;
    static const int defaultThumbnailSize = 40;

    /** 路径列表视图的对象名，用于在槽里区分信号来源。 */
    static const char *objNamePathList;
    /** 条目列表视图的对象名，用于在槽里区分信号来源。 */
    static const char *objNameFeatureList;

public:
    /** @brief 列表横向滚动条的额外留白。 */
    int scrollbarHorisontalPadding() const { return 10; }
    /** @brief 列表纵向滚动条的额外留白。 */
    int scrollbarVerticalIndent() const { return 0; }

signals:
    /** @brief 请求把现有文件夹名列表发给新建文件夹对话框，用于重名校验。 */
    void sendFileNameList(const QStringList lst);

private slots:
    /**
     * @brief 网页部件元数据中的预览图下载完成。
     * @param id 下载任务 id（未使用）
     * @param pSuccess 是否成功（未使用）
     * @param sourceUrl 实际请求地址（未使用）
     * @param originalUrl 原始地址（未使用）
     * @param pContentTypeHeader 响应类型（未使用）
     * @param pData 图片数据
     * @param pPos 目标位置（未使用）
     * @param pSize 目标尺寸（未使用）
     * @param isBackground 是否作为背景（未使用）
     */
    void onPreviewLoaded(int id, bool pSuccess, QUrl sourceUrl, QUrl originalUrl, QString pContentTypeHeader,
                         QByteArray pData, QPointF pPos, QSize pSize, bool isBackground);
    /**
     * @brief 列表或路径列表中的条目被选中：进入文件夹、显示属性或切换动作条状态。
     * @param current 被点击的索引
     */
    void currentSelected(const QModelIndex &);
    /**
     * @brief 把搜索文字交给控制器过滤当前列表。
     * @param pattern 搜索串
     */
    void searchStarted(const QString &);
    /** @brief 弹出新建文件夹输入区并下发已有名称列表。 */
    void createNewFolder();
    /**
     * @brief 删除拖放到动作条上的一组条目（回收站内直接删除）。
     * @param 资源库拖放数据
     */
    void deleteElements(const FeaturesMimeData *);
    /**
     * @brief 把拖放到动作条上的条目加入收藏。
     * @param 资源库拖放数据
     */
    void addToFavorite(const FeaturesMimeData *);
    /**
     * @brief 把拖放到动作条上的条目移出收藏。
     * @param 资源库拖放数据
     */
    void removeFromFavorite(const FeaturesMimeData *);
    /**
     * @brief 显示网页部件的元数据并异步抓取预览图。
     * @param metadata 描述该网页部件的键值对
     */
    void onDisplayMetadata(QMap<QString, QString>);
    /**
     * @brief 下载完成的文件加入资源库后刷新模型。
     * @param pSuccess 是否成功
     * @param sourceUrl 来源地址
     * @param pContentHeader 响应类型
     * @param pData 文件内容
     * @param pTitle 标题
     */
    void onAddDownloadedFileToLibrary(bool, QUrl, QString, QByteArray, QString pTitle);
    /** @brief 收藏当前属性页或列表中选中的条目。 */
    void addElementsToFavorite();
    /** @brief 取消收藏列表中选中的条目。 */
    void removeElementsFromFavorite();
    /** @brief 删除列表中选中的条目。 */
    void deleteSelectedElements();
    /** @brief 请求控制器重新扫描资源库。 */
    void rescanModel();
    /**
     * @brief 扫描期间锁定/解锁界面。
     * @param pLock true 锁定
     */
    void lockIt(bool pLock);

private:
    /** @brief 切回主列表视图（当前为空实现，视图切换由中央控件负责）。 */
    void switchToListView();
    /** @brief 切到属性视图（当前为空实现）。 */
    void switchToProperties();
    /** @brief 切到网页视图（当前为空实现）。 */
    void switchToWebView();

    struct Private;
    std::unique_ptr<Private> d;
};

/**
 * @brief 资源库专用的拖放数据，除 URL 外还携带完整的 Feature 列表。
 *
 * FeaturesModel::mimeData() 创建它，动作条和列表通过这些 Feature 判断类型、
 * 是否可删除以及目标路径。
 */
class FeaturesMimeData : public QMimeData {
    Q_OBJECT

public:
    FeaturesMimeData();
    virtual ~FeaturesMimeData();

    virtual QStringList formats() const;
    /** @brief 拖放携带的条目列表。 */
    QList<Feature> features() const;
    /**
     * @brief 设置拖放携带的条目列表。
     * @param fList 条目列表
     */
    void setFeatures(const QList<Feature> &fList);

private:
    struct Private;
    std::unique_ptr<Private> d;
};

/**
 * @brief 资源库缩略图列表视图，处理拖入与拖放判定。
 *
 * 由 FeaturesNavigatorWidget 创建；只允许把条目拖到有效文件夹上，拒绝拖到自身或
 * 非文件夹位置。缩略图尺寸通过 thumbnailSizeChanged() 与滑块联动并写入设置。
 */
class FeaturesListView : public QListView {
    Q_OBJECT

public:
    /**
     * @brief 构造列表视图。
     * @param parent 父控件
     * @param name 对象名
     */
    FeaturesListView(QWidget *parent = 0, const char *name = "FeaturesListView");
    virtual ~FeaturesListView() { ; }

protected:
    virtual void dragEnterEvent(QDragEnterEvent *event);
    virtual void dropEvent(QDropEvent *event);
    virtual void dragMoveEvent(QDragMoveEvent *event);

public slots:
    // FeaturesNavigatorWidget 的滑块连它；跨类连接必须公开
    /**
     * @brief 按滑块值调整图标与网格尺寸并保存设置。
     * @param value 新的缩略图边长
     */
    void thumbnailSizeChanged(int);
};

/**
 * @brief 资源库主列表容器：缩略图列表加底部尺寸滑块。
 *
 * FeaturesCentralWidget 创建并放入堆叠控件的第一页；滑块范围取自
 * FeaturesWidget 的静态常量，当前值由中央控件在初始化时恢复。
 */
class FeaturesNavigatorWidget : public QWidget {
    Q_OBJECT

public:
    /**
     * @brief 构造列表与滑块并连接二者。
     * @param parent 父控件
     * @param name 对象名（构造时会被固定为 FeaturesNavigatorWidget）
     */
    FeaturesNavigatorWidget(QWidget *parent, const char *name = "");
    /** @brief 内部缩略图列表。 */
    FeaturesListView *listView();
    /**
     * @brief 设置滑块位置，从而刷新缩略图尺寸。
     * @param pValue 滑块值
     */
    void setSliderPosition(int pValue);

private:
    struct Private;
    std::unique_ptr<Private> d;
};

/**
 * @brief 资源库中央区域：主列表、属性页、网页视图三选一，另带附加信息区。
 *
 * 由 FeaturesWidget 创建。上半部分用 QStackedWidget 切换三种视图；下半部分的
 * 附加区用于新建文件夹输入框和扫描进度条，显示时可要求模态（锁定主视图）。
 * 控制器扫描信号与动作条请求都汇总到这里再向外转发。
 */
class FeaturesCentralWidget : public QWidget {
    Q_OBJECT

public:
    /** 中央堆叠控件中的页面。 */
    enum StackElement {
        MainList = 0,          ///< 缩略图主列表
        FeaturePropertiesList, ///< 单个条目的属性页
        FeaturesWebView        ///< 网页部件视图
    };

    /** 附加信息区中的控件。 */
    enum AddWidget {
        NewFolderDialog = 0, ///< 新建文件夹输入框
        ProgressBarWidget    ///< 扫描进度条
    };

    /** 附加信息区的交互方式。 */
    enum AddWidgetState {
        NonModal = 0, ///< 不锁定主视图
        Modal         ///< 显示时锁定主视图
    };

    /**
     * @brief 构造三页堆叠视图与附加信息区。
     * @param parent 父控件
     */
    FeaturesCentralWidget(QWidget *parent = 0);
    /**
     * @brief 设置缩略图滑块位置。
     * @param pValue 滑块值
     */
    void setSliderPosition(int pValue);

    /** @brief 主列表视图。 */
    FeaturesListView *listView();
    /**
     * @brief 在指定视图中显示条目。
     * @param feature 要显示的条目
     * @param pView 目标视图；主列表页不会切换
     */
    void showElement(const Feature &feature, StackElement pView);
    /**
     * @brief 直接切换到指定视图。
     * @param pView 目标页
     */
    void switchTo(StackElement pView);
    /**
     * @brief 设置属性页的原始大图。
     * @param pix 图像
     */
    void setPropertiesPixmap(const QPixmap &pix);
    /**
     * @brief 设置属性页的缩略图。
     * @param pix 图像
     */
    void setPropertiesThumbnail(const QPixmap &pix);
    /** @brief 当前显示的视图页。 */
    StackElement currentView() const;
    /** @brief 属性页当前展示的条目。 */
    Feature getCurElementFromProperties();

    /**
     * @brief 只锁定主视图，不影响附加信息区。
     * @param pLock true 锁定
     */
    void setLockedExcludingAdditional(bool pLock);

signals:
    /** @brief 请求锁定/解锁主视图。 */
    void lockMainWidget(bool pLock);
    /** @brief 新建文件夹确认，携带文件夹名。 */
    void createNewFolderSignal(QString pStr);
    /** @brief 转发已有文件名列表。 */
    void sendFileNameList(const QStringList lst);

    // 进度条相关信号
    /** @brief 扫描文件总数已确定。 */
    void maxFilesCountEvaluated(int pValue);
    /** @brief 已处理条目数加一。 */
    void increaseStatusBarValue();
    /** @brief 当前扫描的分类名。 */
    void scanCategory(const QString &);
    /** @brief 当前扫描的路径。 */
    void scanPath(const QString &);

public slots:
    /**
     * @brief 显示附加信息区中的某个控件。
     * @param pWidgetType 要显示的控件
     * @param pState 是否以模态方式锁定主视图
     */
    void showAdditionalData(AddWidget pWidgetType, AddWidgetState pState = NonModal);

    // FeaturesController 的扫描信号连这两个；跨类连接必须公开
    /** @brief 扫描开始：显示进度条。 */
    void scanStarted();
    /** @brief 扫描结束：隐藏附加信息区。 */
    void scanFinished();

private slots:
    /**
     * @brief 新建文件夹确认后转发给控制器并收起输入区。
     * @param pStr 文件夹名
     */
    void createNewFolderSlot(QString pStr);
    /** @brief 解锁主视图并隐藏附加信息区。 */
    void hideAdditionalData();

private:
    struct Private;
    std::unique_ptr<Private> d;
};

/**
 * @brief 新建文件夹输入区：名称校验、确定/取消按钮。
 *
 * 作为附加信息区的一页存在，由 FeaturesCentralWidget 创建。名称不能包含路径
 * 非法字符且不能与已有文件夹重名；不合法时确定按钮禁用、输入框变红。
 */
class FeaturesNewFolderDialog : public QWidget {
    Q_OBJECT

public:
    /**
     * @brief 构造输入框、校验器与按钮。
     * @param parent 父控件
     */
    FeaturesNewFolderDialog(QWidget *parent = 0);
    /**
     * @brief 替换名称校验用的正则表达式。
     * @param pRegExp 新表达式
     */
    void setRegexp(const QRegularExpression &pRegExp);
    /**
     * @brief 判断当前输入是否合法且不与已有名称重复。
     * @param pStr 待检查的名称
     * @return 可接受时返回 true
     */
    bool validString(const QString &pStr);

signals:
    /** @brief 用户确认新建，携带名称。 */
    void createNewFolder(QString str);
    /** @brief 用户取消，请求关闭输入区。 */
    void closeDialog();

private slots:
    /** @brief 确认：发出名称并清空输入。 */
    void accept();
    /** @brief 取消：清空输入并请求关闭。 */
    void reject();
    /**
     * @brief 输入变化时刷新确定按钮与输入框底色。
     * @param pStr 当前文字
     */
    void reactOnTextChanged(const QString &pStr);

public slots:
    // FeaturesCentralWidget 转发文件名列表；跨类连接必须公开
    /**
     * @brief 记录已有文件夹名，用于重名校验。
     * @param pLst 名称列表
     */
    void setFileNameList(const QStringList &pLst);

private:
    struct Private;
    std::unique_ptr<Private> d;
};

/**
 * @brief 资源库扫描进度控件：进度条加分类/路径文字。
 *
 * 由 FeaturesCentralWidget 放入附加信息区；扫描期间显示“Loading xx (%p%)”，
 * 详细路径靠右显示。进度值由控制器逐条累加。
 */
class FeaturesProgressInfo : public QWidget {
    Q_OBJECT

public:
    /**
     * @brief 构造进度条与两个文字标签。
     * @param parent 父控件
     */
    FeaturesProgressInfo(QWidget *parent = 0);

public slots:
    // FeaturesCentralWidget 转发扫描进度；跨类连接必须公开
    /**
     * @brief 设置进度条格式文字（分类名）。
     * @param str 分类名
     */
    void setCommonInfoText(const QString &str);
    /**
     * @brief 设置右侧详细路径文字。
     * @param str 路径
     */
    void setDetailedInfoText(const QString &str);
    /**
     * @brief 设置进度最大值。
     * @param pValue 文件总数
     */
    void setProgressMax(int pValue);
    /** @brief 进度值加一。 */
    void increaseProgressValue();

private slots:
    /** @brief 设置进度最小值。 */
    void setProgressMin(int pValue);
    /** @brief 保留的槽，当前不处理条目。 */
    void sendFeature(Feature pFeature);

private:
    struct Private;
    std::unique_ptr<Private> d;
};

/**
 * @brief 网页部件视图：在资源库内用 QWebEngineView 打开本地网页部件。
 *
 * 放在中央堆叠控件的第三页。showElement() 读取部件目录下 config.xml 的
 * content/src 属性，据此加载入口页面；网页设置启用脚本、插件与本地存储。
 */
class FeaturesWebWidget : public QWidget {
    Q_OBJECT
public:
    /**
     * @brief 构造内嵌浏览器与布局。
     * @param parent 父控件
     * @param name 对象名
     */
    FeaturesWebWidget(QWidget *parent = 0, const char *name = "FeaturesWebWidget");
    virtual ~FeaturesWebWidget();

    /**
     * @brief 加载给定网页部件。
     * @param elem 条目；其路径下应存在 config.xml
     * @note 找不到 config.xml 或没有 content/src 时会加载目录本身。
     */
    void showElement(const Feature &elem);

private:
    struct Private;
    std::unique_ptr<Private> d;
};

/**
 * @brief 资源库条目属性页：预览图、加入页面/资源库按钮和元数据表。
 *
 * FeaturesCentralWidget 创建并放在堆叠控件第二页。显示时缓存条目和原图，
 * 按面板宽度缩放预览；来自网络的条目才显示“Add to library”按钮。
 */
class FeatureProperties : public QWidget {
    Q_OBJECT
public:
    /**
     * @brief 构造预览区、按钮行与元数据表。
     * @param parent 父控件
     * @param name 对象名
     */
    FeatureProperties(QWidget *parent = 0, const char *name = "FeatureProperties");
    ~FeatureProperties();

    /**
     * @brief 显示指定条目并填充元数据。
     * @param elem 条目
     */
    void showElement(const Feature &elem);
    /** @brief 当前展示的条目，没有时为默认构造的 Feature。 */
    Feature getCurrentElement() const;
    /**
     * @brief 记录原始大图，后续按宽度自适应缩放。
     * @param pix 图像
     */
    void setOrigPixmap(const QPixmap &pix);
    /**
     * @brief 设置预览缩略图并重新适配尺寸。
     * @param pix 图像
     */
    void setThumbnail(const QPixmap &pix);

protected:
    void resizeEvent(QResizeEvent *event);
    void showEvent(QShowEvent *event);

private slots:
    /** @brief “Add to page”：把当前条目加入白板页面。 */
    void onAddToPage();
    /** @brief “Add to library”：把网络条目下载到本地资源库。 */
    void onAddToLib();
    /** @brief 把当前条目设为页面背景。 */
    void onSetAsBackground();

private:
    /** @brief 用条目元数据填满树控件。 */
    void populateMetadata();
    /** @brief 按当前宽度缩放预览图。 */
    void adaptSize();

    struct Private;
    std::unique_ptr<Private> d;
};

/**
 * @brief 属性页里使用的浅灰圆角文字按钮。
 */
class FeatureItemButton : public QPushButton {
public:
    /**
     * @brief 构造按钮并应用固定样式。
     * @param parent 父控件
     * @param name 对象名
     */
    FeatureItemButton(QWidget *parent = 0, const char *name = "FeatureItemButton");
    ~FeatureItemButton();
};

/**
 * @brief 资源库条目列表模型，包装控制器持有的 QList<Feature>。
 *
 * 模型不拥有条目列表，只按行读写；拖放时生成 FeaturesMimeData，并在同模型内
 * 移动时调用 moveData() 完成文件系统与虚拟路径的同步。
 */
class FeaturesModel : public QAbstractListModel {
    Q_OBJECT

signals:
    /** @brief 数据结构被重排，视图可据此刷新。 */
    void dataRestructured();

public:
    /**
     * @brief 构造模型。
     * @param pFeaturesList 外部条目列表，生命周期由调用方保证
     * @param parent 父对象
     */
    FeaturesModel(QList<Feature> *pFeaturesList, QObject *parent = 0);
    virtual ~FeaturesModel();

    /**
     * @brief 从收藏位置删除指定路径的条目。
     * @param path 完整路径
     */
    void deleteFavoriteItem(const QString &path);
    /**
     * @brief 按完整路径删除条目。
     * @param path 完整路径
     */
    void deleteItem(const QString &path);
    /**
     * @brief 按条目内容删除。
     * @param feature 要删除的条目
     */
    void deleteItem(const Feature &feature);

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
    QMimeData *mimeData(const QModelIndexList &indexes) const;
    QStringList mimeTypes() const;
    int rowCount(const QModelIndex &parent) const;
    Qt::ItemFlags flags(const QModelIndex &index) const;
    bool dropMimeData(const QMimeData *mimeData, Qt::DropAction action, int row, int column,
                      const QModelIndex &parent);
    bool removeRows(int row, int count, const QModelIndex &parent = QModelIndex());
    bool removeRow(int row, const QModelIndex &parent = QModelIndex());

    /**
     * @brief 把源条目移动/复制到目标文件夹。
     * @param source 源条目
     * @param destination 目标文件夹条目
     * @param action Move 时删除源，Copy 时保留
     * @param deleteManualy true 时在末尾额外从模型删除源条目
     */
    void moveData(const Feature &source, const Feature &destination, Qt::DropAction action,
                  bool deleteManualy = false);
    Qt::DropActions supportedDropActions() const { return Qt::MoveAction | Qt::CopyAction; }
    Qt::DropActions supportedDragActions() const { return Qt::MoveAction | Qt::CopyAction; }

public slots:
    /**
     * @brief 在末尾插入一个条目。
     * @param item 条目
     */
    void addItem(const Feature &item);

private:
    struct Private;
    std::unique_ptr<Private> d;
};

/**
 * @brief 按完整路径精确匹配的正则过滤代理，用于路径面包屑列表。
 */
class FeaturesProxyModel : public QSortFilterProxyModel {
    Q_OBJECT
public:
    /**
     * @brief 构造代理。
     * @param parent 父对象
     */
    FeaturesProxyModel(QObject *parent = 0) : QSortFilterProxyModel(parent) { ; }
    virtual ~FeaturesProxyModel() {}

protected:
    virtual bool filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const;
};

/**
 * @brief 资源库搜索过滤代理：只保留文件类条目，名称匹配且所在路径包含前缀。
 */
class FeaturesSearchProxyModel : public QSortFilterProxyModel {
    Q_OBJECT
public:
    /**
     * @brief 构造代理。
     * @param parent 父对象
     */
    FeaturesSearchProxyModel(QObject *parent = 0);
    virtual ~FeaturesSearchProxyModel();
    /**
     * @brief 设置搜索时要求条目所在虚拟路径包含的前缀。
     * @param newPrefix 路径前缀
     */
    void setFilterPrefix(const QString &newPrefix);

protected:
    virtual bool filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const;

private:
    struct Private;
    std::unique_ptr<Private> d;
};

/**
 * @brief 路径面包屑过滤代理：只保留当前路径及其各级父文件夹。
 */
class FeaturesPathProxyModel : public QSortFilterProxyModel {
    Q_OBJECT
public:
    /**
     * @brief 构造代理。
     * @param parent 父对象
     */
    FeaturesPathProxyModel(QObject *parent = 0);
    virtual ~FeaturesPathProxyModel();
    /**
     * @brief 设置当前虚拟路径。
     * @param p 路径
     */
    void setPath(const QString &p);

protected:
    virtual bool filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const;

private:
    struct Private;
    std::unique_ptr<Private> d;
};

/**
 * @brief 资源库缩略图列表的绘制委托：去掉条目扩展名并按网格宽度省略文字。
 */
class FeaturesItemDelegate : public QStyledItemDelegate {
    Q_OBJECT
public:
    /**
     * @brief 构造委托。
     * @param parent 父对象
     * @param lw 关联的列表视图，用于取字体度量和网格尺寸
     */
    FeaturesItemDelegate(QObject *parent = 0, const QListView *lw = 0);
    ~FeaturesItemDelegate();
    virtual QString displayText(const QVariant &value, const QLocale &locale) const;

private:
    struct Private;
    std::unique_ptr<Private> d;
};

/**
 * @brief 路径面包屑的绘制委托：画分隔箭头与条目缩略图，不显示文字。
 */
class FeaturesPathItemDelegate : public QStyledItemDelegate {
    Q_OBJECT
public:
    /**
     * @brief 构造委托并加载箭头图标。
     * @param parent 父对象
     */
    FeaturesPathItemDelegate(QObject *parent = 0);
    ~FeaturesPathItemDelegate();
    virtual QString displayText(const QVariant &value, const QLocale &locale) const;
    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const;

private:
    struct Private;
    std::unique_ptr<Private> d;
};
