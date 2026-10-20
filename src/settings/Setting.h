/**
 * @file Setting.h
 * @brief 声明单个配置项及颜色列表配置项的读写封装。
 */

#pragma once

#include <QtWidgets>

class Settings;

/**
 * @brief 单个配置项的读写封装。
 *
 * 位于 settings 层：持有 domain/key/path 与默认值，读写委托给所属 Settings，
 * 因此修改会进入待落盘队列。生命周期由 Settings 构造时分配并持有，随 Settings 销毁。
 * set() 在值变化时发出 changed()，读取发生在 GUI 线程。
 */
class Setting : public QObject
{
    Q_OBJECT

public:
    /** @brief 构造无归属的空设置项。 */
    Setting(Settings *parent = 0);
    /** @brief 构造具体设置项；path 由 domain/key 组合得到。 */
    Setting(Settings *owner, const QString &pDomain, const QString &pKey, const QVariant &pDefaultValue);

    /** @brief 析构。 */
    virtual ~Setting();

    /** @brief 写入新值；与旧值不同时发出 changed()。 */
    virtual void set(const QVariant &pValue);
    /** @brief 读取当前值；未设置时返回默认值。 */
    virtual QVariant get();
    /** @brief 恢复默认值并返回它。 */
    virtual QVariant reset();

    /** @brief 返回所属配置域。 */
    virtual QString domain() const
    {
        return mDomain;
    }

    /** @brief 返回配置键名。 */
    virtual QString key() const
    {
        return mKey;
    }

    /** @brief 返回 domain/key 组合后的完整路径。 */
    virtual QString path() const
    {
        return mPath;
    }

    /** @brief 返回默认值。 */
    virtual QVariant defaultValue() const
    {
        return mDefaultValue;
    }

public slots:
    /** @brief 以 bool 语义写入。 */
    void setBool(bool pValue);
    /** @brief 以字符串写入。 */
    void setString(const QString &pValue);
    /** @brief 以整数写入。 */
    void setInt(int pValue);

signals:
    /** @brief 值发生变化时发出，携带新值。 */
    void changed(QVariant newValue);

protected:
    Settings *mOwner;        ///< 所属配置注册表，不拥有所有权。
    QString   mDomain;       ///< 配置域。
    QString   mKey;          ///< 配置键名。
    QString   mPath;         ///< 完整配置路径。
    QVariant  mDefaultValue; ///< 默认值。
};

/**
 * @brief 颜色列表类型的配置项。
 *
 * 在 Setting 基础上把 QVariant 解释成颜色列表，并可按索引改色、统一改透明度。
 * 由 Settings 持有，供钢笔/荧光笔/网格的配色使用。
 */
class ColorListSetting : public Setting
{
    Q_OBJECT

public:
    /** @brief 构造空的颜色列表设置。 */
    ColorListSetting(Settings *parent = 0);
    /** @brief 构造颜色列表设置，pAlpha 为统一透明度。 */
    ColorListSetting(Settings *owner, const QString &pDomain, const QString &pKey, const QVariant &pDefaultValue, qreal pAlpha = 1.0);

    /** @brief 析构。 */
    virtual ~ColorListSetting();

    /** @brief 恢复默认颜色列表。 */
    virtual QVariant reset();

    /** @brief 返回解析后的颜色列表。 */
    QList<QColor> colors() const;

    /** @brief 修改索引处的颜色；越界时忽略。 */
    void setColor(int pIndex, const QColor &color);

    /** @brief 设置列表统一透明度。 */
    void setAlpha(qreal pAlpha);

protected:
    QList<QColor> mColors; ///< 解析后的颜色列表。
    qreal         mAlpha;  ///< 统一透明度。
};
