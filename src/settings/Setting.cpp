#include "settings/Setting.h"

#include <QtWidgets>

#include "settings/Settings.h"


Setting::Setting(Settings *parent) : QObject(parent) {
    //NOOP
}

Setting::Setting(Settings *owner, const QString &pDomain, const QString &pKey, const QVariant &pDefaultValue)
    : QObject(owner),
      mOwner(owner),
      mDomain(pDomain),
      mKey(pKey),
      mPath(pDomain + "/" + pKey),
      mDefaultValue(pDefaultValue) {
    get(); // force caching of the setting
}

Setting::~Setting() {
    // NOOP
}

void Setting::set(const QVariant &pValue) {
    if (pValue != get()) {
        mOwner->setValue(mPath, pValue);
        emit changed(pValue);
    }
}

QVariant Setting::get() {
    return mOwner->value(mPath, mDefaultValue);
}

QVariant Setting::reset() {
    set(mDefaultValue);
    return mDefaultValue;
}


void Setting::setBool(bool pValue) {
    set(pValue);
}

void Setting::setString(const QString &pValue) {
    set(pValue);
}
void Setting::setInt(int pValue) {
    set(pValue);
}


ColorListSetting::ColorListSetting(Settings *parent) : Setting(parent) {
    //NOOP
}

ColorListSetting::ColorListSetting(Settings *owner, const QString &pDomain, const QString &pKey,
                                   const QVariant &pDefaultValue, qreal pAlpha)
    : Setting(owner, pDomain, pKey, pDefaultValue), mAlpha(pAlpha) {
    foreach (QString s, get().toStringList()) {
        QColor color;
        color.setNamedColor(s);
        if (mAlpha >= 0)
            color.setAlphaF(mAlpha);
        mColors.append(color);
    }
}

ColorListSetting::~ColorListSetting() {
    // NOOP
}

QVariant ColorListSetting::reset() {
    QVariant result = Setting::reset();

    mColors.clear();

    foreach (QString s, get().toStringList()) {
        QColor color;
        color.setNamedColor(s);
        if (mAlpha >= 0)
            color.setAlphaF(mAlpha);

        mColors.append(color);
    }

    mOwner->colorChanged();

    return result;
}

QList<QColor> ColorListSetting::colors() const {
    return mColors;
}

void ColorListSetting::setColor(int pIndex, const QColor &color) {
    QStringList list = get().toStringList();
    list.replace(pIndex, color.name(QColor::HexArgb));
    if (mAlpha >= 0) {
        QColor c = color;
        c.setAlphaF(mAlpha);
        mColors.replace(pIndex, c);
    } else
        mColors.replace(pIndex, color);
    set(list);
}

void ColorListSetting::setAlpha(qreal pAlpha) {
    mAlpha = pAlpha;

    for (int i = 0; i < mColors.size(); i++) {
        QColor c = mColors.at(i);
        c.setAlphaF(mAlpha);
        mColors.replace(i, c);
    }

    mOwner->colorChanged();
}
