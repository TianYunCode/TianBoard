/**
 * @file ExportAdaptor.h
 * @brief 导出适配器基类：统一导出入口、文件名/目录选择与告警上报。
 */

#pragma once

#include <QtWidgets>

class DocumentProxy;

/**
 * @brief 所有导出格式的抽象基类。
 *
 * 负责“导出”这一动作的公共部分：从 DocumentProxy 读取文档显示名生成默认文件名、
 * 弹出保存对话框、检查目标目录可写、维护等待光标，并把失败以信号而非对话框的形式
 * 抛给上层。具体格式由子类实现 persist()/persistsDocument()，DocumentProxy 只提供
 * 文档元数据与持久化目录，二者之间不产生反向依赖。
 *
 * @note 子类必须实现 exportName() 与 persist()；persistsDocument() 默认返回 false，
 *       仅表示“该格式不支持无交互写出”。
 */
class ExportAdaptor : public QObject
{
    Q_OBJECT

public:
    /** @brief 构造适配器，默认开启进度提示（verbose）。 */
    ExportAdaptor(QObject *parent = 0);
    virtual ~ExportAdaptor();

    /** @brief 返回展示给用户的导出格式名称，同时作为菜单项文案。 */
    virtual QString exportName() = 0;
    /** @brief 返回带点的扩展名（如 ".pdf"），用于补全文件名。 */
    virtual QString exportExtention()
    {
        return "";
    }
    /** @brief 交互式导出入口：询问位置并写出文档。 */
    virtual void persist(DocumentProxy *pDocument) = 0;
    /** @brief 把文档无交互地写到 filename；成功返回 true。 */
    virtual bool persistsDocument(DocumentProxy *pDocument, const QString &filename);
    /** @brief 判断当前选中的树节点能否使用本导出器；默认不可用。 */
    virtual bool associatedActionactionAvailableFor(const QModelIndex &selectedIndex)
    {
        Q_UNUSED(selectedIndex);
        return false;
    }
    /** @brief 返回与导出器绑定的菜单/工具栏动作，未绑定时为 0。 */
    QAction *associatedAction()
    {
        return mAssociatedAction;
    }
    /** @brief 绑定外部创建的动作，导出器不持有其所有权。 */
    void setAssociatedAction(QAction *pAssociatedAction)
    {
        mAssociatedAction = pAssociatedAction;
    }

    /** @brief 控制是否向状态栏输出导出进度。 */
    virtual void setVerbose(bool verbose)
    {
        mIsVerbose = verbose;
    }

    /** @brief 当前是否输出导出进度。 */
    virtual bool isVerbose() const
    {
        return mIsVerbose;
    }

signals:
    /** @brief 导出过程中的警告；由注册导出器的界面层决定如何展示。 */
    void exportWarnings(const QString &title, const QList<QString> &messages);

protected:
    /** @brief 弹出保存文件对话框，返回补全扩展名后的路径；用户取消时返回空串。 */
    QString askForFileName(DocumentProxy *pDocument, const QString &pDialogTitle);
    /** @brief 弹出目录选择对话框，返回“目录/文档名”路径；用户取消时返回空串。 */
    QString askForDirName(DocumentProxy *pDocument, const QString &pDialogTitle);

    /** @brief 交互式导出模板方法：选路径、检查权限、调用 persistsDocument()。 */
    virtual void persistLocally(DocumentProxy *pDocumentProxy, const QString &pDialogTitle);

    /** @brief 若错误列表非空则通过 exportWarnings 信号上报。 */
    void reportErrorsList(const QList<QString> &errorsList);

    bool     mIsVerbose;
    QAction *mAssociatedAction;
};
