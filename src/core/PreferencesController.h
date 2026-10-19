#pragma once

#include <QtWidgets>
#include <QDialog>

#include <memory>

class ColorPicker;
class Application;
class VirtualDesktop;
class Settings;
class PreferencesController;
class BrushPropertiesFrame;

class PreferencesDialog : public QDialog {
    Q_OBJECT

public:
    // Hand-written equivalent of the former ui_preferences.h form, defined in the .cpp.
    struct Ui;

    PreferencesDialog(PreferencesController *prefController, QWidget *parent = 0,
                      Qt::WindowFlags f = Qt::WindowFlags());
    ~PreferencesDialog();

    Ui &ui();

protected:
    void closeEvent(QCloseEvent *e);
    PreferencesController *mPreferencesController;

private:
    struct Private;
    std::unique_ptr<Private> d;
};

class PreferencesController : public QObject {
    Q_OBJECT

public:
    PreferencesController(QWidget *parent);
    virtual ~PreferencesController();

public slots:
    void show();

protected:
    void wire();
    void init();

    PreferencesDialog *mPreferencesWindow;
    PreferencesDialog::Ui *mPreferencesUI;
    BrushPropertiesFrame *mPenProperties;
    BrushPropertiesFrame *mMarkerProperties;
    ColorPicker *mDarkBackgroundGridColorPicker;
    ColorPicker *mLightBackgroundGridColorPicker;

public slots:
    // BrushPropertiesFrame 在自己的构造函数里连它，必须对外可见
    void colorSelected(const QColor &);

protected slots:
    void close();
    void defaultSettings();
    void penPreviewFromSizeChanged(int value);
    void darkBackgroundCrossOpacityValueChanged(int value);
    void lightBackgroundCrossOpacityValueChanged(int value);
    void widthSliderChanged(int value);
    void opacitySliderChanged(int value);
    void setCrossColorOnDarkBackground(const QColor &color);
    void setCrossColorOnLightBackground(const QColor &color);
    void toolbarPositionChanged(bool checked);
    void toolbarOrientationVertical(bool checked);
    void toolbarOrientationHorizontal(bool checked);
    void systemOSKCheckBoxToggled(bool checked);

private slots:
    void adjustScreens(int screen);

private:
    static qreal sSliderRatio;
    static qreal sMinPenWidth;
    static qreal sMaxPenWidth;
    VirtualDesktop *mDesktop;
};

class BrushPropertiesFrame {
public:
    // Hand-written equivalent of the former ui_brushProperties.h form, defined in the .cpp.
    struct Ui;

    BrushPropertiesFrame(QFrame *owner, const QList<QColor> &lightBackgroundColors,
                         const QList<QColor> &darkBackgroundColors,
                         const QList<QColor> &lightBackgroundSelectedColors,
                         const QList<QColor> &darkBackgroundSelectedColors,
                         PreferencesController *controller);

    virtual ~BrushPropertiesFrame();

    Ui &ui();

    QList<ColorPicker *> &lightBackgroundColorPickers();
    QList<ColorPicker *> &darkBackgroundColorPickers();

private:
    struct Private;
    std::unique_ptr<Private> d;
};
