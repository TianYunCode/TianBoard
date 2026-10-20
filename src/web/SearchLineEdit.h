/**
 * @file SearchLineEdit.h
 * @brief 声明地址/搜索框的辅助部件：右侧清除按钮 ClearButton 与带搜索图标的 SearchLineEdit。
 */

#pragma once

#include "web/UrlLineEdit.h"

#include <QtWidgets>
#include <QAbstractButton>

class SearchButton;


/**
 * @brief 行编辑框右侧的清除按钮：有文本时显示，点击清空输入。
 *
 * 由 ExLineEdit 创建，自绘一个带叉的圆形按钮；可见性由 textChanged 槽跟随文本变化。
 */
class ClearButton : public QAbstractButton {
    Q_OBJECT;

public:
    /** @brief 构造按钮：无焦点、箭头光标、初始隐藏。 */
    ClearButton(QWidget *parent = 0);

    /** @brief 自绘圆形清除图标。 */
    void paintEvent(QPaintEvent *event);

public slots:
    /** @brief 文本为空时隐藏，否则显示。 */
    void textChanged(const QString &text);
};

/**
 * @brief 搜索/地址输入框：左侧搜索图标（可挂菜单），无文本且失焦时显示占位提示。
 *
 * 继承 ExLineEdit，复用其内嵌 QLineEdit 与左侧部件布局。当前网页地址栏实际使用
 * UrlLineEdit；本类提供搜索框能力，可由外部设置菜单与占位文本。
 */
class SearchLineEdit : public ExLineEdit {
    Q_OBJECT;
    Q_PROPERTY(QString inactiveText READ inactiveText WRITE setInactiveText)

public:
    /** @brief 构造输入框，安装搜索按钮与默认占位文本。 */
    SearchLineEdit(QWidget *parent = 0);

    /** @brief 返回占位提示文本。 */
    QString inactiveText() const;

    /** @brief 设置占位提示文本。 */
    void setInactiveText(const QString &text);

    /**
     * @brief 返回左侧搜索按钮的下拉菜单，未创建时惰性创建。
     */
    QMenu *menu() const;

    /**
     * @brief 设置左侧搜索按钮的下拉菜单，旧菜单延迟销毁。
     */
    void setMenu(QMenu *menu);

    /** @brief 同时显隐内部行编辑框与搜索按钮。 */
    void setVisible(bool pVisible);

protected:
    /** @brief 尺寸变化时重排左侧搜索按钮。 */
    void resizeEvent(QResizeEvent *event);
    /** @brief 在空文本失焦时绘制占位提示。 */
    void paintEvent(QPaintEvent *event);
signals:
    /** @brief 文本变化，转发自内部 QLineEdit。 */
    void textChanged(const QString &text);

private:
    /** @brief 按是否有菜单调整搜索按钮的宽度与位置。 */
    void updateGeometries();

    /** @brief 左侧搜索按钮。 */
    SearchButton *mSearchButton;
    /** @brief 占位提示文本。 */
    QString mInactiveText;
};
