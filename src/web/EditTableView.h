/**
 * @file EditTableView.h
 * @brief 声明 EditTableView：支持 Delete/Backspace 删除行的下载列表视图。
 */

#pragma once

#include <QtWidgets>
#include <QTableView>


/**
 * @brief 可编辑表格视图：下载管理器的列表控件，键盘删除即删除当前下载条目。
 *
 * 只服务于 web 域的下载对话框。真正的数据删除由模型 removeRows 完成，视图负责
 * 在删除后把选择落到相邻行。
 */
class EditTableView : public QTableView {
    Q_OBJECT;

public:
    /** @brief 构造视图。 */
    EditTableView(QWidget *parent = 0);

    /** @brief 拦截 Delete/Backspace 删除当前行，其余按键交给基类。 */
    void keyPressEvent(QKeyEvent *event);

public slots:
    /** @brief 删除当前行并把选择移到相邻行。 */
    void removeOne();

    /** @brief 删除所有行。 */
    void removeAll();
};
