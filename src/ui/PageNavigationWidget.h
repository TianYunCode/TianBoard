/**
 * @file PageNavigationWidget.h
 * @brief 页面导航停靠面板的声明：缩略图导航、页码指示与时钟。
 */

#pragma once

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QMouseEvent>
#include <QResizeEvent>
#include <QTimerEvent>
#include <QLabel>
#include <QString>

#include "ui/DocumentNavigator.h"
#include "ui/DockPaletteWidget.h"
#include "model/DocumentProxy.h"

/**
 * @brief 白板模式下使用的页面导航面板。
 *
 * 面板上半部分是 DocumentNavigator 提供的页面缩略图列表，底部一行显示
 * “当前页 / 总页数”和系统时间（每秒刷新，格式去掉秒）。由 BoardPaletteManager
 * 创建并停靠在主窗口一侧，仅在 Board 模式可见，切到网页/文档模式时随面板
 * 系统一起隐藏。
 */
class PageNavigationWidget : public DockPaletteWidget
{
    Q_OBJECT
public:
    /// 构造面板并搭建导航器、页码与时钟；构造时即启动 1 秒定时器。
    PageNavigationWidget(QWidget *parent = 0, const char *name = "PageNavigationWidget");
    ~PageNavigationWidget();

    /// 只有白板模式需要显示本面板，其余模式返回 false。
    bool visibleInMode(DockWidgetMode mode)
    {
        return mode == DockWidgetMode::Board;
    }

signals:

public slots:
    /// 更新底部的页码显示。
    void setPageNumber(int current, int total);

protected:
    /// 每秒触发一次，刷新时钟文本。
    virtual void timerEvent(QTimerEvent *event);

private:
    /// 用当前系统时间刷新时钟标签。
    void updateTime();
    /// 面板内容距边缘的自定义留白。
    int customMargin();
    /// 面板边框宽度。
    int border();

    DocumentNavigator *mNavigator;  ///< 页面缩略图导航器，占满面板上部。
    QVBoxLayout       *mLayout;     ///< 面板主布局。
    QHBoxLayout       *mHLayout;    ///< 底部页码与时钟的横向布局。
    QLabel            *mPageNbr;    ///< 页码标签。
    QLabel            *mClock;      ///< 时钟标签。
    QString            mTimeFormat; ///< 去掉秒字段后的本地短时间格式。
    int                mTimerID;    ///< 每秒刷新时钟的定时器 id。
};
