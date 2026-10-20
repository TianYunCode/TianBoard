/**
 * @file DrawingController.h
 * @brief 当前绘图工具与画笔/记号笔参数的单例控制器声明。
 */

#pragma once

#include <QtCore>

#include "utils/Enums.h"
#include "ui/StylusPalette.h"

class AbstractDrawRuler;


/**
 * @brief 维护“当前工具”与线宽/颜色索引，并把手写笔工具变化广播给界面。
 *
 * 单例通过 drawingController() 懒加载；构造函数把主窗口的动作（笔、橡皮、
 * 记号笔、选择器等）连接到槽，因此必须在 MainWindow 建立后首次取用。
 * 参数本身存放在 Settings 中，本类只做选择与通知，不持有图元。
 * 线程约束：仅在 GUI 线程使用。
 */
class DrawingController : public QObject {
    Q_OBJECT

private:
    DrawingController(QObject *parent = 0);
    virtual ~DrawingController();

public:
    /// @brief 取得单例。
    static DrawingController *drawingController();
    /// @brief 显式销毁单例。
    static void destroy();

    /// @brief 当前工具（StylusTool::Enum）。
    int stylusTool();
    /// @brief 最近使用过的绘图工具（笔/记号笔/直线），用于快速切回。
    int latestDrawingTool();

    /// @brief 当前工具是否为会落笔的绘图工具（笔、记号笔、直线）。
    bool isDrawingTool();

    /// @brief 当前工具的线宽索引；非绘图工具返回 -1。
    int currentToolWidthIndex();
    /// @brief 当前工具的实际线宽。
    qreal currentToolWidth();
    /// @brief 当前工具的颜色索引；非绘图工具返回 -1。
    int currentToolColorIndex();
    /// @brief 当前工具在当前背景下的颜色。
    QColor currentToolColor();
    /// @brief 指定背景明暗下的当前工具颜色；非绘图工具回退为黑/白。
    QColor toolColor(bool onDarkBackground);

    /// @brief 设置指定背景下的画笔调色板颜色。
    void setPenColor(bool onDarkBackground, const QColor &color, int pIndex);
    /// @brief 设置指定背景下的记号笔调色板颜色。
    void setMarkerColor(bool onDarkBackground, const QColor &color, int pIndex);
    /// @brief 设置记号笔透明度并刷新调色板。
    void setMarkerAlpha(qreal alpha);

    AbstractDrawRuler *mActiveRuler; ///< 当前处于活动状态的尺具（可为空），不持有所有权

    /// @brief 是否处于桌面批注模式。
    void setInDestopMode(bool mode) { mIsDesktopMode = mode; }

    /// @brief 是否处于桌面批注模式。
    bool isInDesktopMode() { return mIsDesktopMode; }

public slots:
    /// @brief 切换当前工具：取消选择、更新最近绘图工具并按新工具回发线宽/颜色索引。
    void setStylusTool(int tool);
    /// @brief 设置当前工具的线宽索引，非直线/选择器时顺带切回笔。
    void setLineWidthIndex(int index);
    /// @brief 设置当前工具的颜色索引。
    void setColorIndex(int index);
    /// @brief 切换到橡皮并设置橡皮宽度索引。
    void setEraserWidthIndex(int index);

signals:
    /// @brief 当前工具变化，参数为 StylusTool::Enum。
    void stylusToolChanged(int tool);
    /// @brief 调色板内容或背景上下文变化，界面需刷新颜色。
    void colorPaletteChanged();

    /// @brief 当前工具线宽索引变化。
    void lineWidthIndexChanged(int index);
    /// @brief 当前工具颜色索引变化。
    void colorIndexChanged(int index);

private:
    StylusTool::Enum mStylusTool;        ///< 当前工具
    StylusTool::Enum mLatestDrawingTool; ///< 最近一次使用的绘图工具
    bool mIsDesktopMode;                 ///< 桌面批注模式标志

    static DrawingController *sDrawingController; ///< 单例指针

private slots:
    void penToolSelected(bool checked);
    void eraserToolSelected(bool checked);
    void markerToolSelected(bool checked);
    void selectorToolSelected(bool checked);
    void playToolSelected(bool checked);
    void handToolSelected(bool checked);
    void zoomInToolSelected(bool checked);
    void zoomOutToolSelected(bool checked);
    void pointerToolSelected(bool checked);
    void lineToolSelected(bool checked);
    void textToolSelected(bool checked);
    void captureToolSelected(bool checked);
};
