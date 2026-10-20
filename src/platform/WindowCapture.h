#pragma once

#include <QtWidgets>

// 选一个外部窗口并抓它的内容。抓取期间把鼠标/键盘独占给 grabTarget（调用方自己的
// 调色板窗口），免得用户在挑窗口时点到自己的界面。平台层只按 QWidget 的
// grab/release 用它，不认识 desktop/ 里的任何类型。
class WindowCapture : public QObject {
    Q_OBJECT

public:
    explicit WindowCapture(QWidget *grabTarget, QObject *parent = nullptr);
    virtual ~WindowCapture();
    int execute();
    const QPixmap getCapturedWindow();

private:
    QPixmap mWindowPixmap;
    QWidget *mGrabTarget;
};
