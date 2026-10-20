/**
 * @file WebKitUtils.h
 * @brief 声明 WebKitUtils：从网页中收集 <embed>/<object> 等可嵌入对象的工具类。
 */

#pragma once

#include <QtWebEngineWidgets>
#include <QWebEnginePage>

/**
 * @brief 网页对象工具类：提供静态方法枚举页面中的可嵌入对象。
 *
 * 当前 Qt6 实现只发起脚本查询，尚未把结果转换为对象列表，因此 objectsInFrame 通常返回空。
 * 保留该类作为后续重新实现元素命中的入口。
 */
class WebKitUtils
{
public:
    /** @brief 构造工具类（无状态）。 */
    WebKitUtils();

    /** @brief 虚析构，便于作为基类使用。 */
    virtual ~WebKitUtils();

    /**
     * @brief 描述一个可嵌入的 HTML 对象：源地址与建议尺寸。
     */
    class HtmlObject
    {
    public:
        /**
         * @brief 构造对象描述。
         * @param pSource 对象源地址。
         * @param pWidth 建议宽度。
         * @param pHeight 建议高度。
         */
        HtmlObject(const QString &pSource, int pWidth, int pHeight) : source(pSource), width(pWidth), height(pHeight)
        {
        }

        /** @brief 对象源地址。 */
        QString source;
        /** @brief 建议宽度。 */
        int width;
        /** @brief 建议高度。 */
        int height;
    };

    /**
     * @brief 枚举页面中的可嵌入对象。
     * @param frame 目标页面。
     * @return 对象列表；当前实现尚未解析脚本返回值，实际为空。
     */
    static QList<WebKitUtils::HtmlObject> objectsInFrame(QWebEnginePage *frame);
};
