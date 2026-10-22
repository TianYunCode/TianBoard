/**
 * @file Resources.h
 * @brief 全局资源单例的声明：各种工具光标与用户自定义字体列表。
 */

#pragma once

#include <QtWidgets>
#include "utils/StaticMemory.h"

/**
 * @brief 应用级共享资源（光标与自定义字体）的单例。
 *
 * 不参与界面绘制，只提供一组预先构造好的 QCursor 和从用户字体目录加载的
 * 字体族名。首次调用 resources() 时创建，挂在 StaticMemory::cleaner()
 * 下随应用退出统一释放；白板视图、标尺、选择框等都从这里取光标，避免反复
 * 从磁盘加载图片。
 */
class Resources : public QObject
{
    Q_OBJECT

public:
    /// 返回单例；首次调用时完成光标初始化并扫描自定义字体目录。
    static Resources *resources();
    /// 返回已加载的自定义字体族名列表。
    QStringList customFontList()
    {
        return mCustomFontList;
    }

private:
    /// 私有构造：单例只能经 resources() 创建。
    Resources(QObject *pParent = 0);
    virtual ~Resources();

    /// 从资源文件构造全部光标。
    void init();

    static Resources *sSingleton; ///< 单例指针，由静态内存清理器托管。
    /// 把字体目录中的每个字体文件注册进 QFontDatabase 并收集族名。
    void        buildFontList();
    QStringList mCustomFontList; ///< 自定义字体族名列表。

public:
    QCursor penCursor;           ///< 钢笔光标。
    QCursor eraserCursor;        ///< 橡皮光标（自定义图片）。
    QCursor markerCursor;        ///< 荧光笔光标（自定义图片）。
    QCursor pointerCursor;       ///< 激光笔光标。
    QCursor handCursor;          ///< 抓手光标。
    QCursor zoomInCursor;        ///< 放大光标。
    QCursor zoomOutCursor;       ///< 缩小光标。
    QCursor arrowCursor;         ///< 普通箭头光标。
    QCursor playCursor;          ///< 播放/交互模式光标。
    QCursor textCursor;          ///< 文本工具光标（当前为箭头）。
    QCursor rotateCursor;        ///< 旋转手柄光标。
    QCursor drawLineRulerCursor; ///< 直尺画线光标。
};
