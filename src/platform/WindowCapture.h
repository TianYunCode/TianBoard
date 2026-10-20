/**
 * @file WindowCapture.h
 * @brief 声明“选择一个外部窗口并抓取其内容”的平台接口。
 */

#pragma once

#include <QtWidgets>

/**
 * @brief 选一个外部窗口并抓取它的内容。
 *
 * 位于 Win32 平台层：抓取期间把鼠标与键盘独占给 grabTarget（调用方自己的调色板窗口），
 * 避免用户在挑选窗口时误点到本程序界面。平台层只按 QWidget 的 grab/release 使用它，
 * 不认识 desktop/ 里的任何类型。execute() 会进入平台消息循环，必须在 GUI 线程调用。
 */
class WindowCapture : public QObject {
    Q_OBJECT

public:
    /** @brief 绑定抓取期间的输入接收窗口。 */
    explicit WindowCapture(QWidget *grabTarget, QObject *parent = nullptr);
    /** @brief 析构，不释放 grabTarget。 */
    virtual ~WindowCapture();
    /** @brief 进入选择流程并抓取，返回平台约定的结果码。 */
    int execute();
    /** @brief 返回最近一次抓取到的窗口图像；未抓取时为空。 */
    const QPixmap getCapturedWindow();

private:
    QPixmap mWindowPixmap; ///< 最近一次抓取的窗口内容。
    QWidget *mGrabTarget;  ///< 抓取期间独占输入的目标窗口，不拥有所有权。
};
