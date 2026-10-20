/**
 * @file IBoardDropHandler.h
 * @brief 网页小部件内部拖放的处理接口与全局工厂安装点声明。
 */

#pragma once

class QGraphicsSceneDragDropEvent;
class GraphicsScene;
class GraphicsWidgetItem;


/**
 * @brief 把拖放到网页小部件上的内容接入画板的抽象接口。
 *
 * 真正干活的是 api 层的门面（需要下载管理器与文档对象目录）。画板若反过来
 * include api 层就把依赖方向写反了，因此接口留在本模块，实现由装配点在启动时
 * 通过 installBoardDropHandlerFactory() 装入（见 app/main.cpp）。
 *
 * 所有权：工厂创建的对象自己认父对象（跟随场景回收），调用方不接管所有权。
 * 线程约束：拖放事件在 GUI 线程派发。
 */
class IBoardDropHandler {
public:
    virtual ~IBoardDropHandler() = default;

    /// @brief 处理落在网页小部件上的拖放事件；事件对象归 Qt 所有，不得保存。
    virtual void processDropEvent(QGraphicsSceneDragDropEvent *event) = 0;
};

/**
 * @brief 创建拖放处理器的工厂函数签名。
 * @param scene 接收内容的画板场景。
 * @param widget 发生拖放的网页小部件。
 * @return 新建的处理器；对象自行认父，调用方不需要释放。
 */
using IBoardDropHandlerFactory = IBoardDropHandler *(*)(GraphicsScene * scene, GraphicsWidgetItem *widget);

/// @brief 安装全局工厂，通常只在应用启动装配阶段调用一次。
void installBoardDropHandlerFactory(IBoardDropHandlerFactory factory);
/// @brief 用已安装的工厂创建处理器；未安装工厂时返回空指针。
IBoardDropHandler *createBoardDropHandler(GraphicsScene *scene, GraphicsWidgetItem *widget);
