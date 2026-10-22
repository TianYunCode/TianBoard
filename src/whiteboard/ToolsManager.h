/**
 * @file ToolsManager.h
 * @brief 尺规类内置工具的注册表声明。
 */

#pragma once

#include <QtWidgets>
#include "utils/StaticMemory.h"

#include "core/Application.h"

/**
 * @brief 工具注册表：集中登记尺、量角器、圆规、遮罩等内置工具的元数据。
 *
 * 单例通过 manager() 懒加载，父对象为 StaticMemory::cleaner()，
 * 因此随应用静态内存清理器统一销毁；destroy() 供提前释放使用。
 * 构造时填充描述符与图标映射，之后只读访问，仅在 GUI 线程使用。
 */
class ToolsManager : public QObject
{
    Q_OBJECT

public:
    /// @brief 单个工具的元数据：标识、图标、显示名与版本。
    class ToolDescriptor
    {
    public:
        QString id;      ///< 工具标识（tianboardtool:// 形式）
        QPixmap icon;    ///< 工具图标
        QString label;   ///< 可翻译的显示名
        QString version; ///< 工具版本号
    };

    /// @brief 取得单例，首次调用时创建并挂到应用静态内存清理器下。
    static ToolsManager *manager();
    /// @brief 显式销毁单例，主要供退出流程调用。
    static void destroy();

    /// @brief 所有已注册工具的列表。
    QList<ToolDescriptor> allTools()
    {
        return mDescriptors;
    }

    /// @brief 所有已注册工具的标识列表。
    QStringList allToolIDs()
    {
        QStringList ids;

        foreach (ToolDescriptor tool, allTools())
        {
            ids << tool.id;
        }

        return ids;
    }

    /// @brief 按标识查找工具；未找到时返回默认构造的空描述符。
    ToolDescriptor toolByID(const QString &id)
    {
        foreach (ToolDescriptor tool, allTools())
        {
            if (tool.id == id)
            {
                return tool;
            }
        }

        return ToolDescriptor();
    }

    ToolDescriptor ruler;      ///< 直尺工具
    ToolDescriptor protractor; ///< 量角器工具
    ToolDescriptor axes;       ///< 坐标轴工具
    ToolDescriptor compass;    ///< 圆规工具
    ToolDescriptor mask;       ///< 遮罩（聚光灯）工具
    ToolDescriptor triangle;   ///< 三角板工具
    ToolDescriptor magnifier;  ///< 放大镜工具
    ToolDescriptor cache;      ///< 幕布工具

    /// @brief 按工具标识返回图标资源路径；未知标识返回空串。
    QString iconFromToolId(QString id)
    {
        return mToolsIcon.value(id);
    }

private:
    ToolsManager(QObject *parent = 0);
    virtual ~ToolsManager();

    static ToolsManager *sManager;      ///< 单例指针

    QList<ToolDescriptor> mDescriptors; ///< 注册顺序即工具面板顺序

    QMap<QString, QString> mToolsIcon;  ///< 工具标识到图标资源路径的映射
};
