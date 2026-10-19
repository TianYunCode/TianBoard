#pragma once

#include <QtWidgets>

class Settings;

class Setting : public QObject {
    Q_OBJECT

public:
    Setting(Settings *parent = 0);
    Setting(Settings *owner, const QString &pDomain, const QString &pKey, const QVariant &pDefaultValue);

    virtual ~Setting();

    virtual void set(const QVariant &pValue);
    virtual QVariant get();
    virtual QVariant reset();

    virtual QString domain() const { return mDomain; }

    virtual QString key() const { return mKey; }

    virtual QString path() const { return mPath; }

    virtual QVariant defaultValue() const { return mDefaultValue; }

public slots:
    void setBool(bool pValue);
    void setString(const QString &pValue);
    void setInt(int pValue);

signals:
    void changed(QVariant newValue);

protected:
    Settings *mOwner;
    QString mDomain;
    QString mKey;
    QString mPath;
    QVariant mDefaultValue;
};

class ColorListSetting : public Setting {
    Q_OBJECT

public:
    ColorListSetting(Settings *parent = 0);
    ColorListSetting(Settings *owner, const QString &pDomain, const QString &pKey,
                     const QVariant &pDefaultValue, qreal pAlpha = 1.0);

    virtual ~ColorListSetting();

    virtual QVariant reset();

    QList<QColor> colors() const;

    void setColor(int pIndex, const QColor &color);

    void setAlpha(qreal pAlpha);

protected:
    QList<QColor> mColors;
    qreal mAlpha;
};
