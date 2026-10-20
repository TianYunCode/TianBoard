/**
 * @file UrlLineEdit.h
 * @brief 声明网页地址栏部件：带左右附加部件的行编辑框 ExLineEdit 及其子类 UrlLineEdit。
 */

#pragma once

#include <QtWidgets>
#include <QWidget>
#include <QLineEdit>
#include <QStyleOption>

#include "web/WebView.h"

#include <memory>

class ClearButton;

/**
 * @brief 增强行编辑框：在 QLineEdit 左右两侧各放一个附加部件（左侧图标、右侧清除按钮）。
 *
 * 把内嵌 QLineEdit 的焦点、输入法、尺寸策略转发到自身，使外部可把整个控件当作行编辑框
 * 使用。地址栏、搜索框都基于它；部件不拥有左侧部件的创建逻辑，setLeftWidget 会删除旧部件。
 */
class ExLineEdit : public QWidget
{
    Q_OBJECT;

public:
    /** @brief 构造控件并创建内嵌行编辑框与清除按钮。 */
    ExLineEdit(QWidget *parent = 0);

    /** @brief 析构。 */
    ~ExLineEdit() override;

    /** @brief 返回内嵌行编辑框。 */
    QLineEdit *lineEdit() const;

    /**
     * @brief 设置左侧附加部件，旧部件会被删除，新部件归本控件所有。
     */
    void setLeftWidget(QWidget *widget);

    /** @brief 返回左侧附加部件。 */
    QWidget *leftWidget() const;

    /** @brief 以临时启用边框的方式取得行编辑框的建议尺寸。 */
    QSize sizeHint() const;

    /** @brief 把输入法查询转发给内嵌行编辑框。 */
    QVariant inputMethodQuery(Qt::InputMethodQuery property) const;

    /** @brief 同时显隐内嵌行编辑框、清除按钮与左侧部件。 */
    void setVisible(bool pVisible);

protected:
    /** @brief 焦点进入时转发给内嵌行编辑框。 */
    void focusInEvent(QFocusEvent *event);
    /** @brief 失焦时补上补全器“回车选中”的连接。 */
    void focusOutEvent(QFocusEvent *event);
    /** @brief 按键转发给内嵌行编辑框。 */
    void keyPressEvent(QKeyEvent *event);
    /** @brief 自身不绘制背景，交给样式与内嵌控件。 */
    void paintEvent(QPaintEvent *event);
    /** @brief 尺寸变化时重排内部控件。 */
    void resizeEvent(QResizeEvent *event);
    /** @brief 输入法事件转发给内嵌行编辑框。 */
    void inputMethodEvent(QInputMethodEvent *e);
    /** @brief 拦截 ShortcutOverride，保证行编辑框优先处理快捷键。 */
    bool event(QEvent *event);

protected:
    /** @brief 按当前尺寸重排左侧部件、行编辑框与清除按钮。 */
    void updateGeometries();
    /** @brief 填充行编辑框的样式选项。 */
    void initStyleOption(QStyleOptionFrame *option) const;

private:
    /** @brief 私有实现。 */
    struct Private;
    std::unique_ptr<Private> d;
};

class WebView;

/**
 * @brief 网页地址栏：把 URL 变化同步到行编辑框，失焦时用当前 URL 补空文本。
 *
 * 与一个 WebView 绑定（setWebView），显示其当前地址，并在加载进度变化时触发重绘。
 * 左侧部件初始为空容器，标签页加载出 favicon 后由 TabWidget 替换为图标标签。
 */
class UrlLineEdit : public ExLineEdit
{
    Q_OBJECT;

public:
    /** @brief 构造地址栏。 */
    UrlLineEdit(QWidget *parent = 0);

    /** @brief 析构。 */
    ~UrlLineEdit() override;

    /**
     * @brief 绑定 WebView，监听 URL 与加载进度变化。
     */
    void setWebView(WebView *webView);

protected:
    /** @brief 失焦且文本为空时显示当前页面 URL。 */
    void focusOutEvent(QFocusEvent *event);

private slots:
    /** @brief URL 变化时刷新地址文本并回到行首。 */
    void webViewUrlChanged(const QUrl &url);

private:
    /** @brief 私有实现。 */
    struct Private;
    std::unique_ptr<Private> d;
};
