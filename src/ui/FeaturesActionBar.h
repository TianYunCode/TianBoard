/**
 * @file FeaturesActionBar.h
 * @brief 提供资源库底部动作条及拖放目标按钮。
 */

#pragma once

#include <QWidget>
#include <QToolButton>
#include <QDropEvent>
#include <QLineEdit>
#include <QLayout>

#include "whiteboard/FeaturesController.h"

#include <memory>

/** 动作条按钮的图标尺寸。 */
#define BUTTON_SIZE 24
/** 动作条固定高度。 */
#define ACTIONBAR_HEIGHT 42

class FeaturesMimeData;

/** 动作条当前展示的按钮集合。 */
typedef enum { eButtonSet_Default, eButtonSet_Properties, eButtonSet_Favorite } eButtonSet;


/**
 * @brief 资源库动作条上的图标按钮，直接绑定一个 QAction。
 *
 * FeaturesActionBar 为收藏、分享、删除、返回、新建文件夹等动作各创建一个实例；
 * 按钮同时充当拖放目标，动作条通过 childAt() 判断落点是否为某个按钮。
 */
class ActionButton : public QToolButton {
public:
    /**
     * @brief 构造按钮并绑定动作。
     * @param parent 父控件
     * @param action 要绑定的动作，可为空
     * @param name 对象名
     */
    ActionButton(QWidget *parent = 0, QAction *action = 0, const char *name = "ActionButton");
    ~ActionButton();
};

/** 资源库动作条所处的界面状态，决定显示哪些按钮。 */
enum FeaturesActionBarState { IN_ROOT, IN_FOLDER, IN_PROPERTIES, IN_FAVORITE, IN_TRASH };

/**
 * @brief 资源库底部动作条：搜索框、新建文件夹、收藏、删除、返回等操作。
 *
 * 由 FeaturesWidget 创建并持有，位于资源库面板底部。按钮集合随当前浏览位置
 * （根目录、文件夹、属性页、收藏、回收站）切换；同时接受资源库条目的拖放：
 * 拖到删除按钮移入回收站，拖到收藏按钮加入收藏，拖到取消收藏按钮移出收藏。
 * 扫描资源库期间由 FeaturesWidget 调用 lockIt()/unlockIt() 整体禁用。
 */
class FeaturesActionBar : public QWidget {
    Q_OBJECT
public:
    /**
     * @brief 构造动作条、动作与拖放目标按钮。
     * @param controller 资源库控制器
     * @param parent 父控件
     * @param name 对象名
     */
    FeaturesActionBar(FeaturesController *controller, QWidget *parent = 0,
                      const char *name = "FeaturesActionBar");
    ~FeaturesActionBar();

    /**
     * @brief 切换界面状态并刷新按钮可见性。
     * @param state 新状态
     */
    void setCurrentState(FeaturesActionBarState state);
    /** @brief 清空搜索框文字。 */
    void cleanText();

signals:
    /** @brief 搜索框文字变化。 */
    void searchElement(const QString &text);
    /** @brief 请求新建文件夹。 */
    void newFolderToCreate();
    /**
     * @brief 拖放删除一组条目。
     * @param data 被拖放的资源库数据
     */
    void deleteElements(const FeaturesMimeData *data);
    /**
     * @brief 拖放收藏一组条目。
     * @param data 被拖放的资源库数据
     */
    void addToFavorite(const FeaturesMimeData *data);
    /**
     * @brief 把一组条目移出收藏。
     * @param data 被拖放的资源库数据
     */
    void removeFromFavorite(const FeaturesMimeData *data);
    /** @brief 收藏当前选中条目。 */
    void addElementsToFavorite();
    /** @brief 取消收藏当前选中条目。 */
    void removeElementsFromFavorite();
    /** @brief 删除当前选中条目。 */
    void deleteSelectedElements();
    /** @brief 请求重新扫描资源库。 */
    void rescanModel();

private slots:
    /**
     * @brief 搜索文字变化时发出 searchElement()。
     * @param txt 新文字（实际读取的是搜索框当前内容）
     */
    void onSearchTextChanged(QString txt);
    /** @brief 新建文件夹动作。 */
    void onActionNewFolder();
    /** @brief 收藏动作。 */
    void onActionFavorite();
    /** @brief 取消收藏动作。 */
    void onActionRemoveFavorite();
    /** @brief 删除动作。 */
    void onActionTrash();
    /** @brief 重新扫描动作。 */
    void onActionRescanModel();

public slots:
    // FeaturesWidget 在扫描开始/结束时连这两个；跨类连接必须公开
    /** @brief 禁用整个动作条。 */
    void lockIt();
    /** @brief 重新启用整个动作条。 */
    void unlockIt();

protected:
    void dragEnterEvent(QDragEnterEvent *event);
    void dropEvent(QDropEvent *event);

private:
    /** @brief 按当前状态设置各按钮的显隐与可用性。 */
    void setButtons();

    struct Private;
    std::unique_ptr<Private> d;
};
