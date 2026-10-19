#include "core/PreferencesController.h"

#include <QDate>

#include "settings/Settings.h"
#include "core/Application.h"

#include "ui/CircleFrame.h"

#include "settings/Setting.h"
#include "core/LanguageManager.h"
#include "core/ApplicationController.h"
#include "core/DisplayManager.h"

#include "whiteboard/BoardController.h"
#include "model/DocumentController.h"
#include "whiteboard/GraphicsScene.h"
#include "whiteboard/DrawingController.h"
#include "recording/RecordingController.h"

#include "ui/ColorPicker.h"


#include "core/VirtualDesktop.h"

qreal PreferencesController::sSliderRatio = 10.0;
qreal PreferencesController::sMinPenWidth = 0.5;
qreal PreferencesController::sMaxPenWidth = 50.0;


// Hand-written replacement for the generated ui_brushProperties.h form (uic 6.9.3).
// The form is built into the QFrame handed to BrushPropertiesFrame, which owns it as
// its parent widget; the Ui struct holds raw pointers only. Object names and the
// translated source texts must stay exactly as uic produced them. The translation
// context is kept as "brushProperties" because BrushPropertiesFrame is not a QObject,
// so no tr() is available for it.
struct BrushPropertiesFrame::Ui {
    QGridLayout *gridLayout;
    QFrame *pressureFrame;
    QHBoxLayout *horizontalLayout_4;
    QCheckBox *pressureSensitiveCheckBox;
    QSpacerItem *seSpacer;
    QFrame *opacityFrame;
    QHBoxLayout *horizontalLayout;
    QSpacerItem *wSpacer_2;
    QLabel *opacityLabel;
    QSlider *opacitySlider;
    QSpacerItem *eSpacer_2;
    QFrame *colorFrame;
    QVBoxLayout *verticalLayout;
    QFrame *lightBackgroundFrame;
    QHBoxLayout *lightBackgroundLayout;
    QLabel *lightBackgroundLabel;
    QSpacerItem *horizontalSpacer;
    ColorPicker *lightBackgroundColorPicker0;
    QSpacerItem *horizontalSpacer_3;
    QFrame *darkBackgroundFrame;
    QHBoxLayout *darkBackgroundLayout;
    QLabel *darkBackgroundLabel;
    QSpacerItem *horizontalSpacer_2;
    ColorPicker *darkBackgroundColorPicker0;
    QGroupBox *lineWidthGroupBox;
    QGridLayout *gridLayout_2;
    QSpacerItem *wSpacer;
    QSpacerItem *eSpacer;
    QSlider *fineSlider;
    QLabel *mediumLabel;
    QSlider *mediumSlider;
    QLabel *strongLabel;
    QSlider *strongSlider;
    CircleFrame *fineDisplayFrame;
    CircleFrame *mediumDisplayFrame;
    CircleFrame *strongDisplayFrame;
    QLabel *fineLabel;
    QSpacerItem *cSpacer;
    QFrame *circleFrame;
    QHBoxLayout *horizontalLayout_5;
    QCheckBox *circleCheckBox;
    QSpinBox *circleSpinBox;
    QLabel *pxlabel;
    QSpacerItem *cspacerright;

    void setupUi(QFrame *brushProperties) {
        if (brushProperties->objectName().isEmpty())
            brushProperties->setObjectName("brushProperties");
        brushProperties->resize(605, 808);
        brushProperties->setFrameShape(QFrame::NoFrame);
        brushProperties->setFrameShadow(QFrame::Plain);
        gridLayout = new QGridLayout(brushProperties);
        gridLayout->setObjectName("gridLayout");
        pressureFrame = new QFrame(brushProperties);
        pressureFrame->setObjectName("pressureFrame");
        pressureFrame->setFrameShape(QFrame::NoFrame);
        pressureFrame->setFrameShadow(QFrame::Raised);
        horizontalLayout_4 = new QHBoxLayout(pressureFrame);
        horizontalLayout_4->setObjectName("horizontalLayout_4");
        horizontalLayout_4->setContentsMargins(0, 0, 0, 0);
        pressureSensitiveCheckBox = new QCheckBox(pressureFrame);
        pressureSensitiveCheckBox->setObjectName("pressureSensitiveCheckBox");
        QSizePolicy sizePolicy(QSizePolicy::Policy::MinimumExpanding, QSizePolicy::Policy::Fixed);
        sizePolicy.setHorizontalStretch(0);
        sizePolicy.setVerticalStretch(0);
        sizePolicy.setHeightForWidth(pressureSensitiveCheckBox->sizePolicy().hasHeightForWidth());
        pressureSensitiveCheckBox->setSizePolicy(sizePolicy);

        horizontalLayout_4->addWidget(pressureSensitiveCheckBox);

        seSpacer = new QSpacerItem(198, 20, QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Minimum);

        horizontalLayout_4->addItem(seSpacer);


        gridLayout->addWidget(pressureFrame, 6, 1, 1, 4);

        opacityFrame = new QFrame(brushProperties);
        opacityFrame->setObjectName("opacityFrame");
        opacityFrame->setFrameShape(QFrame::NoFrame);
        opacityFrame->setFrameShadow(QFrame::Raised);
        horizontalLayout = new QHBoxLayout(opacityFrame);
        horizontalLayout->setObjectName("horizontalLayout");
        wSpacer_2 = new QSpacerItem(154, 20, QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Minimum);

        horizontalLayout->addItem(wSpacer_2);

        opacityLabel = new QLabel(opacityFrame);
        opacityLabel->setObjectName("opacityLabel");

        horizontalLayout->addWidget(opacityLabel);

        opacitySlider = new QSlider(opacityFrame);
        opacitySlider->setObjectName("opacitySlider");
        opacitySlider->setMinimum(20);
        opacitySlider->setMaximum(100);
        opacitySlider->setValue(50);
        opacitySlider->setOrientation(Qt::Horizontal);
        opacitySlider->setTickPosition(QSlider::TicksAbove);
        opacitySlider->setTickInterval(20);

        horizontalLayout->addWidget(opacitySlider);

        eSpacer_2 = new QSpacerItem(156, 20, QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Minimum);

        horizontalLayout->addItem(eSpacer_2);


        gridLayout->addWidget(opacityFrame, 2, 1, 1, 4);

        colorFrame = new QFrame(brushProperties);
        colorFrame->setObjectName("colorFrame");
        colorFrame->setLayoutDirection(Qt::LeftToRight);
        colorFrame->setFrameShape(QFrame::NoFrame);
        colorFrame->setFrameShadow(QFrame::Raised);
        verticalLayout = new QVBoxLayout(colorFrame);
        verticalLayout->setSpacing(0);
        verticalLayout->setObjectName("verticalLayout");
        lightBackgroundFrame = new QFrame(colorFrame);
        lightBackgroundFrame->setObjectName("lightBackgroundFrame");
        lightBackgroundFrame->setFrameShape(QFrame::NoFrame);
        lightBackgroundFrame->setFrameShadow(QFrame::Raised);
        lightBackgroundLayout = new QHBoxLayout(lightBackgroundFrame);
        lightBackgroundLayout->setObjectName("lightBackgroundLayout");
        lightBackgroundLabel = new QLabel(lightBackgroundFrame);
        lightBackgroundLabel->setObjectName("lightBackgroundLabel");

        lightBackgroundLayout->addWidget(lightBackgroundLabel);

        horizontalSpacer =
            new QSpacerItem(40, 20, QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Minimum);

        lightBackgroundLayout->addItem(horizontalSpacer);

        lightBackgroundColorPicker0 = new ColorPicker(lightBackgroundFrame);
        lightBackgroundColorPicker0->setObjectName("lightBackgroundColorPicker0");
        lightBackgroundColorPicker0->setMinimumSize(QSize(32, 32));
        lightBackgroundColorPicker0->setFrameShape(QFrame::StyledPanel);
        lightBackgroundColorPicker0->setFrameShadow(QFrame::Raised);

        lightBackgroundLayout->addWidget(lightBackgroundColorPicker0);


        verticalLayout->addWidget(lightBackgroundFrame);

        horizontalSpacer_3 =
            new QSpacerItem(40, 20, QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Minimum);

        verticalLayout->addItem(horizontalSpacer_3);

        darkBackgroundFrame = new QFrame(colorFrame);
        darkBackgroundFrame->setObjectName("darkBackgroundFrame");
        darkBackgroundFrame->setFrameShape(QFrame::NoFrame);
        darkBackgroundFrame->setFrameShadow(QFrame::Raised);
        darkBackgroundLayout = new QHBoxLayout(darkBackgroundFrame);
        darkBackgroundLayout->setObjectName("darkBackgroundLayout");
        darkBackgroundLabel = new QLabel(darkBackgroundFrame);
        darkBackgroundLabel->setObjectName("darkBackgroundLabel");

        darkBackgroundLayout->addWidget(darkBackgroundLabel);

        horizontalSpacer_2 =
            new QSpacerItem(40, 20, QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Minimum);

        darkBackgroundLayout->addItem(horizontalSpacer_2);

        darkBackgroundColorPicker0 = new ColorPicker(darkBackgroundFrame);
        darkBackgroundColorPicker0->setObjectName("darkBackgroundColorPicker0");
        darkBackgroundColorPicker0->setMinimumSize(QSize(32, 32));
        darkBackgroundColorPicker0->setFrameShape(QFrame::StyledPanel);
        darkBackgroundColorPicker0->setFrameShadow(QFrame::Raised);

        darkBackgroundLayout->addWidget(darkBackgroundColorPicker0);


        verticalLayout->addWidget(darkBackgroundFrame);


        gridLayout->addWidget(colorFrame, 1, 1, 1, 4);

        lineWidthGroupBox = new QGroupBox(brushProperties);
        lineWidthGroupBox->setObjectName("lineWidthGroupBox");
        QSizePolicy sizePolicy1(QSizePolicy::Policy::Minimum, QSizePolicy::Policy::Preferred);
        sizePolicy1.setHorizontalStretch(0);
        sizePolicy1.setVerticalStretch(0);
        sizePolicy1.setHeightForWidth(lineWidthGroupBox->sizePolicy().hasHeightForWidth());
        lineWidthGroupBox->setSizePolicy(sizePolicy1);
        gridLayout_2 = new QGridLayout(lineWidthGroupBox);
        gridLayout_2->setObjectName("gridLayout_2");
        wSpacer = new QSpacerItem(40, 20, QSizePolicy::Policy::Fixed, QSizePolicy::Policy::Minimum);

        gridLayout_2->addItem(wSpacer, 1, 1, 1, 1);

        eSpacer = new QSpacerItem(40, 20, QSizePolicy::Policy::Fixed, QSizePolicy::Policy::Minimum);

        gridLayout_2->addItem(eSpacer, 1, 3, 1, 1);

        fineSlider = new QSlider(lineWidthGroupBox);
        fineSlider->setObjectName("fineSlider");
        fineSlider->setMinimum(5);
        fineSlider->setMaximum(500);
        fineSlider->setValue(5);
        fineSlider->setOrientation(Qt::Horizontal);
        fineSlider->setTickPosition(QSlider::TicksAbove);
        fineSlider->setTickInterval(100);

        gridLayout_2->addWidget(fineSlider, 1, 2, 1, 1);

        mediumLabel = new QLabel(lineWidthGroupBox);
        mediumLabel->setObjectName("mediumLabel");

        gridLayout_2->addWidget(mediumLabel, 2, 0, 1, 1);

        mediumSlider = new QSlider(lineWidthGroupBox);
        mediumSlider->setObjectName("mediumSlider");
        mediumSlider->setMinimum(5);
        mediumSlider->setMaximum(500);
        mediumSlider->setOrientation(Qt::Horizontal);
        mediumSlider->setTickPosition(QSlider::TicksAbove);
        mediumSlider->setTickInterval(100);

        gridLayout_2->addWidget(mediumSlider, 2, 2, 1, 1);

        strongLabel = new QLabel(lineWidthGroupBox);
        strongLabel->setObjectName("strongLabel");

        gridLayout_2->addWidget(strongLabel, 3, 0, 1, 1);

        strongSlider = new QSlider(lineWidthGroupBox);
        strongSlider->setObjectName("strongSlider");
        strongSlider->setMinimum(5);
        strongSlider->setMaximum(500);
        strongSlider->setOrientation(Qt::Horizontal);
        strongSlider->setTickPosition(QSlider::TicksAbove);
        strongSlider->setTickInterval(100);

        gridLayout_2->addWidget(strongSlider, 3, 2, 1, 1);

        fineDisplayFrame = new CircleFrame(lineWidthGroupBox);
        fineDisplayFrame->setObjectName("fineDisplayFrame");
        QSizePolicy sizePolicy2(QSizePolicy::Policy::Fixed, QSizePolicy::Policy::Fixed);
        sizePolicy2.setHorizontalStretch(60);
        sizePolicy2.setVerticalStretch(60);
        sizePolicy2.setHeightForWidth(fineDisplayFrame->sizePolicy().hasHeightForWidth());
        fineDisplayFrame->setSizePolicy(sizePolicy2);
        fineDisplayFrame->setMinimumSize(QSize(60, 60));
        fineDisplayFrame->setFrameShape(QFrame::StyledPanel);
        fineDisplayFrame->setFrameShadow(QFrame::Raised);

        gridLayout_2->addWidget(fineDisplayFrame, 1, 4, 1, 1);

        mediumDisplayFrame = new CircleFrame(lineWidthGroupBox);
        mediumDisplayFrame->setObjectName("mediumDisplayFrame");
        sizePolicy2.setHeightForWidth(mediumDisplayFrame->sizePolicy().hasHeightForWidth());
        mediumDisplayFrame->setSizePolicy(sizePolicy2);
        mediumDisplayFrame->setMinimumSize(QSize(60, 60));
        mediumDisplayFrame->setFrameShape(QFrame::StyledPanel);
        mediumDisplayFrame->setFrameShadow(QFrame::Raised);

        gridLayout_2->addWidget(mediumDisplayFrame, 2, 4, 1, 1);

        strongDisplayFrame = new CircleFrame(lineWidthGroupBox);
        strongDisplayFrame->setObjectName("strongDisplayFrame");
        sizePolicy2.setHeightForWidth(strongDisplayFrame->sizePolicy().hasHeightForWidth());
        strongDisplayFrame->setSizePolicy(sizePolicy2);
        strongDisplayFrame->setMinimumSize(QSize(60, 60));
        strongDisplayFrame->setFrameShape(QFrame::StyledPanel);
        strongDisplayFrame->setFrameShadow(QFrame::Raised);

        gridLayout_2->addWidget(strongDisplayFrame, 3, 4, 1, 1);

        fineLabel = new QLabel(lineWidthGroupBox);
        fineLabel->setObjectName("fineLabel");

        gridLayout_2->addWidget(fineLabel, 1, 0, 1, 1);


        gridLayout->addWidget(lineWidthGroupBox, 4, 1, 1, 4);

        cSpacer = new QSpacerItem(20, 40, QSizePolicy::Policy::Minimum, QSizePolicy::Policy::Expanding);

        gridLayout->addItem(cSpacer, 3, 2, 1, 1);

        circleFrame = new QFrame(brushProperties);
        circleFrame->setObjectName("circleFrame");
        circleFrame->setFrameShape(QFrame::NoFrame);
        circleFrame->setFrameShadow(QFrame::Raised);
        horizontalLayout_5 = new QHBoxLayout(circleFrame);
        horizontalLayout_5->setObjectName("horizontalLayout_5");
        horizontalLayout_5->setContentsMargins(0, 0, 0, 0);
        circleCheckBox = new QCheckBox(circleFrame);
        circleCheckBox->setObjectName("circleCheckBox");
        sizePolicy.setHeightForWidth(circleCheckBox->sizePolicy().hasHeightForWidth());
        circleCheckBox->setSizePolicy(sizePolicy);

        horizontalLayout_5->addWidget(circleCheckBox);

        circleSpinBox = new QSpinBox(circleFrame);
        circleSpinBox->setObjectName("circleSpinBox");

        horizontalLayout_5->addWidget(circleSpinBox);

        pxlabel = new QLabel(circleFrame);
        pxlabel->setObjectName("pxlabel");

        horizontalLayout_5->addWidget(pxlabel);


        gridLayout->addWidget(circleFrame, 5, 1, 1, 1);

        cspacerright = new QSpacerItem(198, 20, QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Minimum);

        gridLayout->addItem(cspacerright, 5, 3, 1, 1);


        retranslateUi(brushProperties);

        // QMetaObject::connectSlotsByName() omitted on purpose: BrushPropertiesFrame is
        // not a QObject and declares no slot at all.
    } // setupUi

    void retranslateUi(QFrame *brushProperties) {
        brushProperties->setWindowTitle(QString());
        pressureSensitiveCheckBox->setText(
            QCoreApplication::translate("brushProperties", "Pen is Pressure Sensitive", nullptr));
        opacityLabel->setText(QCoreApplication::translate("brushProperties", "Opacity", nullptr));
        lightBackgroundLabel->setText(
            QCoreApplication::translate("brushProperties", "On Light Background", nullptr));
        darkBackgroundLabel->setText(
            QCoreApplication::translate("brushProperties", "On Dark Background", nullptr));
        lineWidthGroupBox->setTitle(QCoreApplication::translate("brushProperties", "Line Width", nullptr));
        mediumLabel->setText(QCoreApplication::translate("brushProperties", "Medium", nullptr));
        strongLabel->setText(QCoreApplication::translate("brushProperties", "Strong", nullptr));
        fineLabel->setText(QCoreApplication::translate("brushProperties", "Fine", nullptr));
        circleCheckBox->setText(
            QCoreApplication::translate("brushProperties", "Show preview circle from", nullptr));
        pxlabel->setText(QCoreApplication::translate("brushProperties", "px", nullptr));
    } // retranslateUi
};

struct BrushPropertiesFrame::Private {
    Ui ui;
    QList<ColorPicker *> lightBackgroundColorPickers;
    QList<ColorPicker *> darkBackgroundColorPickers;
};


// Hand-written replacement for the generated ui_preferences.h form (uic 6.9.3).
// Every widget and layout created below is parented to the dialog, so Qt owns and
// destroys them: this struct holds raw pointers only and never deletes anything.
// Object names and translated source texts must stay exactly as uic produced them.
struct PreferencesDialog::Ui {
    QGridLayout *gridLayout;
    QFrame *frame;
    QHBoxLayout *horizontalLayout_2;
    QPushButton *defaultSettingsButton;
    QSpacerItem *bottomHSpacer;
    QHBoxLayout *horizontalLayout;
    QPushButton *closeButton;
    QLabel *versionLabel;
    QTabWidget *mainTabWidget;
    QWidget *displayTab;
    QGridLayout *gridLayout_3;
    QGroupBox *multiDisplayGroupBox;
    QGridLayout *gridLayout_2;
    QSpacerItem *mirroringRSpacer;
    QSpacerItem *mirroringLSpacer;
    QCheckBox *swapControlAndDisplayScreensCheckBox;
    QCheckBox *displayBrowserPageCheckBox;
    QCheckBox *swapDisplayScreensCheckBox;
    QSpacerItem *modeSpacer;
    QGroupBox *modeGroupBox;
    QGridLayout *gridLayout_15;
    QSpacerItem *horizontalSpacer_5;
    QGridLayout *gridLayout_16;
    QLabel *modeLabel;
    QComboBox *startModeComboBox;
    QLabel *languageLabel;
    QComboBox *languageComboBox;
    QSpacerItem *horizontalSpacer_6;
    QGroupBox *stylusPaletteGroupBox;
    QGridLayout *gridLayout_14;
    QSpacerItem *horizontalSpacer_4;
    QSpacerItem *horizontalSpacer_3;
    QGridLayout *gridLayout_5;
    QRadioButton *horizontalChoice;
    QRadioButton *verticalChoice;
    QSpacerItem *verticalSpacer_5;
    QGroupBox *keyboardGroupBox;
    QGridLayout *gridLayout_13;
    QGridLayout *oskGridLayout;
    QComboBox *keyboardPaletteKeyButtonSize;
    QLabel *keyboardPaletteKeyButtonSize_Label;
    QCheckBox *useSystemOSKCheckBox;
    QSpacerItem *horizontalSpacer;
    QSpacerItem *horizontalSpacer_2;
    QSpacerItem *verticalSpacer_3;
    QGroupBox *toolbarGroupBox;
    QGridLayout *gridLayout_9;
    QGridLayout *gridLayout_10;
    QRadioButton *toolbarAtTopRadioButton;
    QRadioButton *toolbarAtBottomRadioButton;
    QSpacerItem *verticalSpacer;
    QSpacerItem *toolbarLSpacer;
    QSpacerItem *toolbarRSpacer;
    QCheckBox *toolbarDisplayTextCheckBox;
    QSpacerItem *verticalSpacer_7;
    QSpacerItem *keyboardSpacer;
    QSpacerItem *toolbarSpacer;
    QGroupBox *documentsGroupBox;
    QGridLayout *gridLayout_19;
    QGridLayout *gridLayout_18;
    QCheckBox *showDateColumnOnAlphabeticalSort;
    QCheckBox *emptyTrashForOlderDocuments;
    QSpinBox *emptyTrashDaysValue;
    QLabel *emptyTrashDaysLabel;
    QSpacerItem *horizontalSpacer_9;
    QSpacerItem *horizontalSpacer_10;
    QWidget *gridTab;
    QGridLayout *gridLayout_17;
    QFrame *gridFrame;
    QGridLayout *gridLayout_11;
    QFrame *crossColorLightBackgroundFrame;
    QHBoxLayout *crossColorLightBackgroundLayout;
    QLabel *crossColorLightBackgroundLabel;
    QSpacerItem *horizontalSpacer_7;
    ColorPicker *crossColorLightBackgroundColorPicker;
    QFrame *darkBackgroundOpacityFrame;
    QHBoxLayout *horizontalLayout_8;
    QSpacerItem *wSpacer_2;
    QLabel *darkBackgroundOpacityLabel;
    QSlider *darkBackgroundOpacitySlider;
    QSpacerItem *eSpacer_2;
    QFrame *crossColorDarkBackgroundFrame;
    QHBoxLayout *crossColorDarkBackgroundLayout;
    QLabel *crossColorDarkBackgroundLabel;
    QSpacerItem *horizontalSpacer_8;
    ColorPicker *crossColorDarkBackgroundColorPicker;
    QFrame *lightBackgroundOpacityFrame;
    QHBoxLayout *horizontalLayout_10;
    QSpacerItem *wSpacer_3;
    QLabel *lightBackgroundOpacityLabel;
    QSlider *lightBackgroundOpacitySlider;
    QSpacerItem *eSpacer_3;
    QSpacerItem *verticalSpacer_4;
    QWidget *penTab;
    QGridLayout *gridLayout_4;
    QFrame *penFrame;
    QWidget *markerTab;
    QVBoxLayout *verticalLayout;
    QFrame *markerFrame;
    QWidget *networkTab;
    QGridLayout *gridLayout_6;
    QGroupBox *internetGroupBox;
    QGridLayout *gridLayout_7;
    QGridLayout *gridLayout_8;
    QCheckBox *useExternalBrowserCheckBox;
    QGridLayout *gridLayout_12;
    QLineEdit *webHomePage;
    QLabel *labelHomePage;
    QLabel *label;
    QWidget *widget;
    QHBoxLayout *horizontalLayout_5;
    QLineEdit *proxyUsername;
    QLabel *label_3;
    QLineEdit *proxyPassword;
    QSpacerItem *internetLSpacer;
    QSpacerItem *verticalSpacer_2;
    QWidget *aboutTab;
    QVBoxLayout *verticalLayout_2;
    QTextBrowser *copyrightTextBrowser;

    void setupUi(QWidget *preferencesDialog) {
        if (preferencesDialog->objectName().isEmpty())
            preferencesDialog->setObjectName("preferencesDialog");
        preferencesDialog->resize(825, 767);
        gridLayout = new QGridLayout(preferencesDialog);
        gridLayout->setObjectName("gridLayout");
        frame = new QFrame(preferencesDialog);
        frame->setObjectName("frame");
        frame->setFrameShape(QFrame::NoFrame);
        frame->setFrameShadow(QFrame::Raised);
        horizontalLayout_2 = new QHBoxLayout(frame);
        horizontalLayout_2->setObjectName("horizontalLayout_2");
        horizontalLayout_2->setContentsMargins(0, 0, 0, 0);
        defaultSettingsButton = new QPushButton(frame);
        defaultSettingsButton->setObjectName("defaultSettingsButton");

        horizontalLayout_2->addWidget(defaultSettingsButton);

        bottomHSpacer = new QSpacerItem(40, 20, QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Minimum);

        horizontalLayout_2->addItem(bottomHSpacer);

        horizontalLayout = new QHBoxLayout();
        horizontalLayout->setObjectName("horizontalLayout");
        closeButton = new QPushButton(frame);
        closeButton->setObjectName("closeButton");

        horizontalLayout->addWidget(closeButton);


        horizontalLayout_2->addLayout(horizontalLayout);


        gridLayout->addWidget(frame, 4, 0, 1, 1);

        versionLabel = new QLabel(preferencesDialog);
        versionLabel->setObjectName("versionLabel");

        gridLayout->addWidget(versionLabel, 0, 0, 1, 1);

        mainTabWidget = new QTabWidget(preferencesDialog);
        mainTabWidget->setObjectName("mainTabWidget");
        displayTab = new QWidget();
        displayTab->setObjectName("displayTab");
        gridLayout_3 = new QGridLayout(displayTab);
        gridLayout_3->setObjectName("gridLayout_3");
        multiDisplayGroupBox = new QGroupBox(displayTab);
        multiDisplayGroupBox->setObjectName("multiDisplayGroupBox");
        gridLayout_2 = new QGridLayout(multiDisplayGroupBox);
        gridLayout_2->setObjectName("gridLayout_2");
        mirroringRSpacer =
            new QSpacerItem(40, 20, QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Minimum);

        gridLayout_2->addItem(mirroringRSpacer, 1, 3, 1, 1);

        mirroringLSpacer =
            new QSpacerItem(40, 20, QSizePolicy::Policy::Preferred, QSizePolicy::Policy::Minimum);

        gridLayout_2->addItem(mirroringLSpacer, 1, 0, 1, 1);

        swapControlAndDisplayScreensCheckBox = new QCheckBox(multiDisplayGroupBox);
        swapControlAndDisplayScreensCheckBox->setObjectName("swapControlAndDisplayScreensCheckBox");

        gridLayout_2->addWidget(swapControlAndDisplayScreensCheckBox, 2, 2, 1, 1);

        displayBrowserPageCheckBox = new QCheckBox(multiDisplayGroupBox);
        displayBrowserPageCheckBox->setObjectName("displayBrowserPageCheckBox");

        gridLayout_2->addWidget(displayBrowserPageCheckBox, 1, 2, 1, 1);

        swapDisplayScreensCheckBox = new QCheckBox(multiDisplayGroupBox);
        swapDisplayScreensCheckBox->setObjectName("swapDisplayScreensCheckBox");

        gridLayout_2->addWidget(swapDisplayScreensCheckBox, 2, 3, 1, 1);


        gridLayout_3->addWidget(multiDisplayGroupBox, 2, 0, 1, 1);

        modeSpacer = new QSpacerItem(20, 40, QSizePolicy::Policy::Minimum, QSizePolicy::Policy::Expanding);

        gridLayout_3->addItem(modeSpacer, 14, 0, 1, 1);

        modeGroupBox = new QGroupBox(displayTab);
        modeGroupBox->setObjectName("modeGroupBox");
        QSizePolicy sizePolicy(QSizePolicy::Policy::MinimumExpanding, QSizePolicy::Policy::Expanding);
        sizePolicy.setHorizontalStretch(0);
        sizePolicy.setVerticalStretch(0);
        sizePolicy.setHeightForWidth(modeGroupBox->sizePolicy().hasHeightForWidth());
        modeGroupBox->setSizePolicy(sizePolicy);
        gridLayout_15 = new QGridLayout(modeGroupBox);
        gridLayout_15->setObjectName("gridLayout_15");
        horizontalSpacer_5 =
            new QSpacerItem(40, 20, QSizePolicy::Policy::Preferred, QSizePolicy::Policy::Minimum);

        gridLayout_15->addItem(horizontalSpacer_5, 0, 0, 1, 1);

        gridLayout_16 = new QGridLayout();
        gridLayout_16->setObjectName("gridLayout_16");
        modeLabel = new QLabel(modeGroupBox);
        modeLabel->setObjectName("modeLabel");

        gridLayout_16->addWidget(modeLabel, 0, 0, 1, 1);

        startModeComboBox = new QComboBox(modeGroupBox);
        startModeComboBox->addItem(QString());
        startModeComboBox->addItem(QString());
        startModeComboBox->setObjectName("startModeComboBox");

        gridLayout_16->addWidget(startModeComboBox, 0, 1, 1, 1);

        horizontalSpacer_6 =
            new QSpacerItem(40, 20, QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Minimum);

        gridLayout_16->addItem(horizontalSpacer_6, 0, 2, 1, 1);

        languageLabel = new QLabel(modeGroupBox);
        languageLabel->setObjectName("languageLabel");

        gridLayout_16->addWidget(languageLabel, 1, 0, 1, 1);

        languageComboBox = new QComboBox(modeGroupBox);
        languageComboBox->setObjectName("languageComboBox");

        gridLayout_16->addWidget(languageComboBox, 1, 1, 1, 1);


        gridLayout_15->addLayout(gridLayout_16, 0, 1, 2, 1);


        gridLayout_3->addWidget(modeGroupBox, 15, 0, 1, 1);

        stylusPaletteGroupBox = new QGroupBox(displayTab);
        stylusPaletteGroupBox->setObjectName("stylusPaletteGroupBox");
        stylusPaletteGroupBox->setMinimumSize(QSize(0, 0));
        gridLayout_14 = new QGridLayout(stylusPaletteGroupBox);
        gridLayout_14->setObjectName("gridLayout_14");
        gridLayout_14->setContentsMargins(12, -1, -1, -1);
        horizontalSpacer_4 =
            new QSpacerItem(40, 20, QSizePolicy::Policy::Preferred, QSizePolicy::Policy::Minimum);

        gridLayout_14->addItem(horizontalSpacer_4, 0, 0, 1, 1);

        horizontalSpacer_3 =
            new QSpacerItem(40, 20, QSizePolicy::Policy::Preferred, QSizePolicy::Policy::Minimum);

        gridLayout_14->addItem(horizontalSpacer_3, 0, 2, 1, 1);

        gridLayout_5 = new QGridLayout();
        gridLayout_5->setObjectName("gridLayout_5");
        gridLayout_5->setSizeConstraint(QLayout::SetDefaultConstraint);
        horizontalChoice = new QRadioButton(stylusPaletteGroupBox);
        horizontalChoice->setObjectName("horizontalChoice");
        QSizePolicy sizePolicy1(QSizePolicy::Policy::Minimum, QSizePolicy::Policy::Fixed);
        sizePolicy1.setHorizontalStretch(1);
        sizePolicy1.setVerticalStretch(0);
        sizePolicy1.setHeightForWidth(horizontalChoice->sizePolicy().hasHeightForWidth());
        horizontalChoice->setSizePolicy(sizePolicy1);
        horizontalChoice->setMinimumSize(QSize(0, 0));
        horizontalChoice->setChecked(false);

        gridLayout_5->addWidget(horizontalChoice, 0, 0, 1, 1);

        verticalChoice = new QRadioButton(stylusPaletteGroupBox);
        verticalChoice->setObjectName("verticalChoice");
        verticalChoice->setChecked(true);

        gridLayout_5->addWidget(verticalChoice, 1, 0, 1, 1);

        verticalSpacer_5 =
            new QSpacerItem(20, 40, QSizePolicy::Policy::Minimum, QSizePolicy::Policy::Expanding);

        gridLayout_5->addItem(verticalSpacer_5, 2, 0, 1, 1);


        gridLayout_14->addLayout(gridLayout_5, 0, 1, 1, 1);


        gridLayout_3->addWidget(stylusPaletteGroupBox, 4, 0, 2, 1);

        keyboardGroupBox = new QGroupBox(displayTab);
        keyboardGroupBox->setObjectName("keyboardGroupBox");
        sizePolicy.setHeightForWidth(keyboardGroupBox->sizePolicy().hasHeightForWidth());
        keyboardGroupBox->setSizePolicy(sizePolicy);
        gridLayout_13 = new QGridLayout(keyboardGroupBox);
        gridLayout_13->setObjectName("gridLayout_13");
        oskGridLayout = new QGridLayout();
        oskGridLayout->setObjectName("oskGridLayout");
        keyboardPaletteKeyButtonSize = new QComboBox(keyboardGroupBox);
        keyboardPaletteKeyButtonSize->setObjectName("keyboardPaletteKeyButtonSize");

        oskGridLayout->addWidget(keyboardPaletteKeyButtonSize, 3, 1, 1, 1);

        keyboardPaletteKeyButtonSize_Label = new QLabel(keyboardGroupBox);
        keyboardPaletteKeyButtonSize_Label->setObjectName("keyboardPaletteKeyButtonSize_Label");

        oskGridLayout->addWidget(keyboardPaletteKeyButtonSize_Label, 3, 0, 1, 1);

        useSystemOSKCheckBox = new QCheckBox(keyboardGroupBox);
        useSystemOSKCheckBox->setObjectName("useSystemOSKCheckBox");

        oskGridLayout->addWidget(useSystemOSKCheckBox, 2, 0, 1, 1);


        gridLayout_13->addLayout(oskGridLayout, 0, 1, 1, 2);

        horizontalSpacer =
            new QSpacerItem(40, 20, QSizePolicy::Policy::Preferred, QSizePolicy::Policy::Minimum);

        gridLayout_13->addItem(horizontalSpacer, 0, 0, 1, 1);

        horizontalSpacer_2 =
            new QSpacerItem(40, 20, QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Minimum);

        gridLayout_13->addItem(horizontalSpacer_2, 0, 3, 1, 1);


        gridLayout_3->addWidget(keyboardGroupBox, 13, 0, 1, 1);

        verticalSpacer_3 =
            new QSpacerItem(20, 40, QSizePolicy::Policy::Minimum, QSizePolicy::Policy::Expanding);

        gridLayout_3->addItem(verticalSpacer_3, 7, 0, 1, 1);

        toolbarGroupBox = new QGroupBox(displayTab);
        toolbarGroupBox->setObjectName("toolbarGroupBox");
        sizePolicy.setHeightForWidth(toolbarGroupBox->sizePolicy().hasHeightForWidth());
        toolbarGroupBox->setSizePolicy(sizePolicy);
        gridLayout_9 = new QGridLayout(toolbarGroupBox);
        gridLayout_9->setObjectName("gridLayout_9");
        gridLayout_10 = new QGridLayout();
        gridLayout_10->setObjectName("gridLayout_10");
        toolbarAtTopRadioButton = new QRadioButton(toolbarGroupBox);
        toolbarAtTopRadioButton->setObjectName("toolbarAtTopRadioButton");

        gridLayout_10->addWidget(toolbarAtTopRadioButton, 0, 0, 1, 1);

        toolbarAtBottomRadioButton = new QRadioButton(toolbarGroupBox);
        toolbarAtBottomRadioButton->setObjectName("toolbarAtBottomRadioButton");

        gridLayout_10->addWidget(toolbarAtBottomRadioButton, 1, 0, 1, 1);

        verticalSpacer = new QSpacerItem(20, 20, QSizePolicy::Policy::Minimum, QSizePolicy::Policy::Fixed);

        gridLayout_10->addItem(verticalSpacer, 2, 0, 1, 1);


        gridLayout_9->addLayout(gridLayout_10, 0, 1, 1, 1);

        toolbarLSpacer =
            new QSpacerItem(40, 20, QSizePolicy::Policy::Preferred, QSizePolicy::Policy::Minimum);

        gridLayout_9->addItem(toolbarLSpacer, 0, 0, 1, 1);

        toolbarRSpacer =
            new QSpacerItem(40, 20, QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Minimum);

        gridLayout_9->addItem(toolbarRSpacer, 0, 2, 1, 1);

        toolbarDisplayTextCheckBox = new QCheckBox(toolbarGroupBox);
        toolbarDisplayTextCheckBox->setObjectName("toolbarDisplayTextCheckBox");

        gridLayout_9->addWidget(toolbarDisplayTextCheckBox, 1, 1, 1, 1);


        gridLayout_3->addWidget(toolbarGroupBox, 8, 0, 1, 1);

        verticalSpacer_7 =
            new QSpacerItem(20, 40, QSizePolicy::Policy::Minimum, QSizePolicy::Policy::Expanding);

        gridLayout_3->addItem(verticalSpacer_7, 17, 0, 1, 1);

        keyboardSpacer =
            new QSpacerItem(20, 40, QSizePolicy::Policy::Minimum, QSizePolicy::Policy::Expanding);

        gridLayout_3->addItem(keyboardSpacer, 9, 0, 1, 1);

        toolbarSpacer = new QSpacerItem(20, 40, QSizePolicy::Policy::Minimum, QSizePolicy::Policy::Expanding);

        gridLayout_3->addItem(toolbarSpacer, 3, 0, 1, 1);

        documentsGroupBox = new QGroupBox(displayTab);
        documentsGroupBox->setObjectName("documentsGroupBox");
        sizePolicy.setHeightForWidth(documentsGroupBox->sizePolicy().hasHeightForWidth());
        documentsGroupBox->setSizePolicy(sizePolicy);
        gridLayout_19 = new QGridLayout(documentsGroupBox);
        gridLayout_19->setObjectName("gridLayout_19");
        gridLayout_18 = new QGridLayout();
        gridLayout_18->setObjectName("gridLayout_18");
        showDateColumnOnAlphabeticalSort = new QCheckBox(documentsGroupBox);
        showDateColumnOnAlphabeticalSort->setObjectName("showDateColumnOnAlphabeticalSort");
        QSizePolicy sizePolicy2(QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Preferred);
        sizePolicy2.setHorizontalStretch(0);
        sizePolicy2.setVerticalStretch(0);
        sizePolicy2.setHeightForWidth(showDateColumnOnAlphabeticalSort->sizePolicy().hasHeightForWidth());
        showDateColumnOnAlphabeticalSort->setSizePolicy(sizePolicy2);

        gridLayout_18->addWidget(showDateColumnOnAlphabeticalSort, 0, 1, 1, 1);

        emptyTrashForOlderDocuments = new QCheckBox(documentsGroupBox);
        emptyTrashForOlderDocuments->setObjectName("emptyTrashForOlderDocuments");

        gridLayout_18->addWidget(emptyTrashForOlderDocuments, 3, 1, 1, 1);

        emptyTrashDaysValue = new QSpinBox(documentsGroupBox);
        emptyTrashDaysValue->setObjectName("emptyTrashDaysValue");

        gridLayout_18->addWidget(emptyTrashDaysValue, 3, 2, 1, 1);

        emptyTrashDaysLabel = new QLabel(documentsGroupBox);
        emptyTrashDaysLabel->setObjectName("emptyTrashDaysLabel");

        gridLayout_18->addWidget(emptyTrashDaysLabel, 3, 3, 1, 1);


        gridLayout_19->addLayout(gridLayout_18, 0, 1, 1, 1);

        horizontalSpacer_9 =
            new QSpacerItem(40, 20, QSizePolicy::Policy::Fixed, QSizePolicy::Policy::Minimum);

        gridLayout_19->addItem(horizontalSpacer_9, 0, 0, 2, 1);

        horizontalSpacer_10 =
            new QSpacerItem(40, 20, QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Minimum);

        gridLayout_19->addItem(horizontalSpacer_10, 0, 2, 1, 1);


        gridLayout_3->addWidget(documentsGroupBox, 16, 0, 1, 1);

        mainTabWidget->addTab(displayTab, QString());
        gridTab = new QWidget();
        gridTab->setObjectName("gridTab");
        gridLayout_17 = new QGridLayout(gridTab);
        gridLayout_17->setObjectName("gridLayout_17");
        gridFrame = new QFrame(gridTab);
        gridFrame->setObjectName("gridFrame");
        gridFrame->setEnabled(true);
        gridFrame->setFrameShape(QFrame::NoFrame);
        gridFrame->setFrameShadow(QFrame::Raised);
        gridLayout_11 = new QGridLayout(gridFrame);
        gridLayout_11->setObjectName("gridLayout_11");
        crossColorLightBackgroundFrame = new QFrame(gridFrame);
        crossColorLightBackgroundFrame->setObjectName("crossColorLightBackgroundFrame");
        crossColorLightBackgroundFrame->setFrameShape(QFrame::NoFrame);
        crossColorLightBackgroundFrame->setFrameShadow(QFrame::Raised);
        crossColorLightBackgroundLayout = new QHBoxLayout(crossColorLightBackgroundFrame);
        crossColorLightBackgroundLayout->setObjectName("crossColorLightBackgroundLayout");
        crossColorLightBackgroundLabel = new QLabel(crossColorLightBackgroundFrame);
        crossColorLightBackgroundLabel->setObjectName("crossColorLightBackgroundLabel");

        crossColorLightBackgroundLayout->addWidget(crossColorLightBackgroundLabel);

        horizontalSpacer_7 =
            new QSpacerItem(40, 20, QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Minimum);

        crossColorLightBackgroundLayout->addItem(horizontalSpacer_7);

        crossColorLightBackgroundColorPicker = new ColorPicker(crossColorLightBackgroundFrame);
        crossColorLightBackgroundColorPicker->setObjectName("crossColorLightBackgroundColorPicker");
        crossColorLightBackgroundColorPicker->setMinimumSize(QSize(32, 32));
        crossColorLightBackgroundColorPicker->setFrameShape(QFrame::StyledPanel);
        crossColorLightBackgroundColorPicker->setFrameShadow(QFrame::Raised);

        crossColorLightBackgroundLayout->addWidget(crossColorLightBackgroundColorPicker);


        gridLayout_11->addWidget(crossColorLightBackgroundFrame, 2, 0, 1, 1);

        darkBackgroundOpacityFrame = new QFrame(gridFrame);
        darkBackgroundOpacityFrame->setObjectName("darkBackgroundOpacityFrame");
        darkBackgroundOpacityFrame->setFrameShape(QFrame::NoFrame);
        darkBackgroundOpacityFrame->setFrameShadow(QFrame::Raised);
        horizontalLayout_8 = new QHBoxLayout(darkBackgroundOpacityFrame);
        horizontalLayout_8->setObjectName("horizontalLayout_8");
        wSpacer_2 = new QSpacerItem(154, 20, QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Minimum);

        horizontalLayout_8->addItem(wSpacer_2);

        darkBackgroundOpacityLabel = new QLabel(darkBackgroundOpacityFrame);
        darkBackgroundOpacityLabel->setObjectName("darkBackgroundOpacityLabel");

        horizontalLayout_8->addWidget(darkBackgroundOpacityLabel);

        darkBackgroundOpacitySlider = new QSlider(darkBackgroundOpacityFrame);
        darkBackgroundOpacitySlider->setObjectName("darkBackgroundOpacitySlider");
        darkBackgroundOpacitySlider->setMinimum(20);
        darkBackgroundOpacitySlider->setMaximum(100);
        darkBackgroundOpacitySlider->setValue(50);
        darkBackgroundOpacitySlider->setOrientation(Qt::Horizontal);
        darkBackgroundOpacitySlider->setTickPosition(QSlider::TicksAbove);
        darkBackgroundOpacitySlider->setTickInterval(20);

        horizontalLayout_8->addWidget(darkBackgroundOpacitySlider);

        eSpacer_2 = new QSpacerItem(156, 20, QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Minimum);

        horizontalLayout_8->addItem(eSpacer_2);


        gridLayout_11->addWidget(darkBackgroundOpacityFrame, 5, 0, 1, 1);

        crossColorDarkBackgroundFrame = new QFrame(gridFrame);
        crossColorDarkBackgroundFrame->setObjectName("crossColorDarkBackgroundFrame");
        crossColorDarkBackgroundFrame->setFrameShape(QFrame::NoFrame);
        crossColorDarkBackgroundFrame->setFrameShadow(QFrame::Raised);
        crossColorDarkBackgroundLayout = new QHBoxLayout(crossColorDarkBackgroundFrame);
        crossColorDarkBackgroundLayout->setObjectName("crossColorDarkBackgroundLayout");
        crossColorDarkBackgroundLabel = new QLabel(crossColorDarkBackgroundFrame);
        crossColorDarkBackgroundLabel->setObjectName("crossColorDarkBackgroundLabel");

        crossColorDarkBackgroundLayout->addWidget(crossColorDarkBackgroundLabel);

        horizontalSpacer_8 =
            new QSpacerItem(40, 20, QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Minimum);

        crossColorDarkBackgroundLayout->addItem(horizontalSpacer_8);

        crossColorDarkBackgroundColorPicker = new ColorPicker(crossColorDarkBackgroundFrame);
        crossColorDarkBackgroundColorPicker->setObjectName("crossColorDarkBackgroundColorPicker");
        crossColorDarkBackgroundColorPicker->setMinimumSize(QSize(32, 32));
        crossColorDarkBackgroundColorPicker->setFrameShape(QFrame::StyledPanel);
        crossColorDarkBackgroundColorPicker->setFrameShadow(QFrame::Raised);

        crossColorDarkBackgroundLayout->addWidget(crossColorDarkBackgroundColorPicker);


        gridLayout_11->addWidget(crossColorDarkBackgroundFrame, 4, 0, 1, 1);

        lightBackgroundOpacityFrame = new QFrame(gridFrame);
        lightBackgroundOpacityFrame->setObjectName("lightBackgroundOpacityFrame");
        lightBackgroundOpacityFrame->setFrameShape(QFrame::NoFrame);
        lightBackgroundOpacityFrame->setFrameShadow(QFrame::Raised);
        horizontalLayout_10 = new QHBoxLayout(lightBackgroundOpacityFrame);
        horizontalLayout_10->setObjectName("horizontalLayout_10");
        wSpacer_3 = new QSpacerItem(154, 20, QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Minimum);

        horizontalLayout_10->addItem(wSpacer_3);

        lightBackgroundOpacityLabel = new QLabel(lightBackgroundOpacityFrame);
        lightBackgroundOpacityLabel->setObjectName("lightBackgroundOpacityLabel");

        horizontalLayout_10->addWidget(lightBackgroundOpacityLabel);

        lightBackgroundOpacitySlider = new QSlider(lightBackgroundOpacityFrame);
        lightBackgroundOpacitySlider->setObjectName("lightBackgroundOpacitySlider");
        lightBackgroundOpacitySlider->setMinimum(20);
        lightBackgroundOpacitySlider->setMaximum(100);
        lightBackgroundOpacitySlider->setValue(50);
        lightBackgroundOpacitySlider->setOrientation(Qt::Horizontal);
        lightBackgroundOpacitySlider->setTickPosition(QSlider::TicksAbove);
        lightBackgroundOpacitySlider->setTickInterval(20);

        horizontalLayout_10->addWidget(lightBackgroundOpacitySlider);

        eSpacer_3 = new QSpacerItem(156, 20, QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Minimum);

        horizontalLayout_10->addItem(eSpacer_3);


        gridLayout_11->addWidget(lightBackgroundOpacityFrame, 3, 0, 1, 1);


        gridLayout_17->addWidget(gridFrame, 0, 0, 1, 1);

        verticalSpacer_4 =
            new QSpacerItem(20, 342, QSizePolicy::Policy::Minimum, QSizePolicy::Policy::Expanding);

        gridLayout_17->addItem(verticalSpacer_4, 1, 0, 1, 1);

        mainTabWidget->addTab(gridTab, QString());
        penTab = new QWidget();
        penTab->setObjectName("penTab");
        gridLayout_4 = new QGridLayout(penTab);
        gridLayout_4->setObjectName("gridLayout_4");
        penFrame = new QFrame(penTab);
        penFrame->setObjectName("penFrame");
        penFrame->setFrameShape(QFrame::NoFrame);
        penFrame->setFrameShadow(QFrame::Raised);

        gridLayout_4->addWidget(penFrame, 0, 0, 1, 2);

        mainTabWidget->addTab(penTab, QString());
        markerTab = new QWidget();
        markerTab->setObjectName("markerTab");
        verticalLayout = new QVBoxLayout(markerTab);
        verticalLayout->setObjectName("verticalLayout");
        markerFrame = new QFrame(markerTab);
        markerFrame->setObjectName("markerFrame");
        markerFrame->setFrameShape(QFrame::NoFrame);
        markerFrame->setFrameShadow(QFrame::Raised);

        verticalLayout->addWidget(markerFrame);

        mainTabWidget->addTab(markerTab, QString());
        networkTab = new QWidget();
        networkTab->setObjectName("networkTab");
        gridLayout_6 = new QGridLayout(networkTab);
        gridLayout_6->setObjectName("gridLayout_6");
        internetGroupBox = new QGroupBox(networkTab);
        internetGroupBox->setObjectName("internetGroupBox");
        QSizePolicy sizePolicy3(QSizePolicy::Policy::MinimumExpanding, QSizePolicy::Policy::Fixed);
        sizePolicy3.setHorizontalStretch(0);
        sizePolicy3.setVerticalStretch(0);
        sizePolicy3.setHeightForWidth(internetGroupBox->sizePolicy().hasHeightForWidth());
        internetGroupBox->setSizePolicy(sizePolicy3);
        gridLayout_7 = new QGridLayout(internetGroupBox);
        gridLayout_7->setObjectName("gridLayout_7");
        gridLayout_8 = new QGridLayout();
        gridLayout_8->setObjectName("gridLayout_8");
        useExternalBrowserCheckBox = new QCheckBox(internetGroupBox);
        useExternalBrowserCheckBox->setObjectName("useExternalBrowserCheckBox");

        gridLayout_8->addWidget(useExternalBrowserCheckBox, 0, 0, 1, 1);


        gridLayout_7->addLayout(gridLayout_8, 0, 1, 1, 1);

        gridLayout_12 = new QGridLayout();
        gridLayout_12->setObjectName("gridLayout_12");
        webHomePage = new QLineEdit(internetGroupBox);
        webHomePage->setObjectName("webHomePage");
        QSizePolicy sizePolicy4(QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Fixed);
        sizePolicy4.setHorizontalStretch(0);
        sizePolicy4.setVerticalStretch(0);
        sizePolicy4.setHeightForWidth(webHomePage->sizePolicy().hasHeightForWidth());
        webHomePage->setSizePolicy(sizePolicy4);

        gridLayout_12->addWidget(webHomePage, 0, 1, 1, 1);

        labelHomePage = new QLabel(internetGroupBox);
        labelHomePage->setObjectName("labelHomePage");

        gridLayout_12->addWidget(labelHomePage, 0, 0, 1, 1);

        label = new QLabel(internetGroupBox);
        label->setObjectName("label");

        gridLayout_12->addWidget(label, 1, 0, 1, 1);

        widget = new QWidget(internetGroupBox);
        widget->setObjectName("widget");
        horizontalLayout_5 = new QHBoxLayout(widget);
        horizontalLayout_5->setSpacing(5);
        horizontalLayout_5->setObjectName("horizontalLayout_5");
        horizontalLayout_5->setContentsMargins(0, 0, 0, 0);
        proxyUsername = new QLineEdit(widget);
        proxyUsername->setObjectName("proxyUsername");

        horizontalLayout_5->addWidget(proxyUsername);

        label_3 = new QLabel(widget);
        label_3->setObjectName("label_3");

        horizontalLayout_5->addWidget(label_3);

        proxyPassword = new QLineEdit(widget);
        proxyPassword->setObjectName("proxyPassword");
        proxyPassword->setEchoMode(QLineEdit::Password);

        horizontalLayout_5->addWidget(proxyPassword);


        gridLayout_12->addWidget(widget, 1, 1, 1, 1);


        gridLayout_7->addLayout(gridLayout_12, 3, 1, 1, 1);

        internetLSpacer =
            new QSpacerItem(40, 20, QSizePolicy::Policy::Preferred, QSizePolicy::Policy::Minimum);

        gridLayout_7->addItem(internetLSpacer, 0, 0, 1, 1);


        gridLayout_6->addWidget(internetGroupBox, 0, 0, 1, 1);

        verticalSpacer_2 =
            new QSpacerItem(20, 40, QSizePolicy::Policy::Minimum, QSizePolicy::Policy::Expanding);

        gridLayout_6->addItem(verticalSpacer_2, 2, 0, 1, 1);

        mainTabWidget->addTab(networkTab, QString());
        aboutTab = new QWidget();
        aboutTab->setObjectName("aboutTab");
        aboutTab->setEnabled(true);
        verticalLayout_2 = new QVBoxLayout(aboutTab);
        verticalLayout_2->setObjectName("verticalLayout_2");
        copyrightTextBrowser = new QTextBrowser(aboutTab);
        copyrightTextBrowser->setObjectName("copyrightTextBrowser");
        // 文案（含版本号）在 retranslateUi() 里排，切语言和拿到 applicationVersion()
        // 都只要走那一条路径。
        copyrightTextBrowser->setOpenExternalLinks(true);

        verticalLayout_2->addWidget(copyrightTextBrowser);

        mainTabWidget->addTab(aboutTab, QString());

        gridLayout->addWidget(mainTabWidget, 1, 0, 1, 1);

        QWidget::setTabOrder(useExternalBrowserCheckBox, displayBrowserPageCheckBox);
        QWidget::setTabOrder(displayBrowserPageCheckBox, toolbarAtTopRadioButton);
        QWidget::setTabOrder(toolbarAtTopRadioButton, toolbarAtBottomRadioButton);
        QWidget::setTabOrder(toolbarAtBottomRadioButton, defaultSettingsButton);
        QWidget::setTabOrder(defaultSettingsButton, closeButton);

        retranslateUi(preferencesDialog);

        closeButton->setDefault(true);
        mainTabWidget->setCurrentIndex(3);

        // QMetaObject::connectSlotsByName() omitted on purpose: the dialog and its
        // controller declare no on_<object>_<signal>() slot.
    } // setupUi

    void retranslateUi(QWidget *preferencesDialog) {
        preferencesDialog->setWindowTitle(PreferencesDialog::tr("Preferences", nullptr));
        defaultSettingsButton->setText(PreferencesDialog::tr("Default Settings", nullptr));
        closeButton->setText(PreferencesDialog::tr("Close", nullptr));
        versionLabel->setText(
            PreferencesDialog::tr("version: %1", nullptr).arg(Application::applicationVersion()));
        copyrightTextBrowser->setHtml(
            QStringLiteral("<h3>TianBoard %1</h3><p>%2</p>")
                .arg(
                    Application::applicationVersion(),
                    PreferencesDialog::tr("TianBoard is copyright \302\251 %1. All rights reserved.", nullptr)
                        .arg(QDate::currentDate().year())));
        multiDisplayGroupBox->setTitle(PreferencesDialog::tr("Multi display", nullptr));
        swapControlAndDisplayScreensCheckBox->setText(
            PreferencesDialog::tr("Swap control display and view display", nullptr));
        displayBrowserPageCheckBox->setText(PreferencesDialog::tr(
            "Show internal web page content on secondary screen or projector", nullptr));
        swapDisplayScreensCheckBox->setText(
            PreferencesDialog::tr("Swap first and second view displays", nullptr));
        modeGroupBox->setTitle(PreferencesDialog::tr("Mode", nullptr));
        modeLabel->setText(PreferencesDialog::tr("Mode to start in:", nullptr));
        startModeComboBox->setItemText(0, PreferencesDialog::tr("Board", nullptr));
        startModeComboBox->setItemText(1, PreferencesDialog::tr("Desktop", nullptr));

        languageLabel->setText(PreferencesDialog::tr("Language", nullptr));

        stylusPaletteGroupBox->setTitle(PreferencesDialog::tr("Stylus Palette", nullptr));
        horizontalChoice->setText(PreferencesDialog::tr("Horizontal", nullptr));
        verticalChoice->setText(PreferencesDialog::tr("Vertical", nullptr));
        keyboardGroupBox->setTitle(PreferencesDialog::tr("Virtual Keyboard", nullptr));
        keyboardPaletteKeyButtonSize_Label->setText(
            PreferencesDialog::tr("Built-in virtual keyboard button size:", nullptr));
        useSystemOSKCheckBox->setText(PreferencesDialog::tr("Use system keyboard (recommended)", nullptr));
        toolbarGroupBox->setTitle(PreferencesDialog::tr("Toolbar", nullptr));
        toolbarAtTopRadioButton->setText(
            PreferencesDialog::tr("Positioned at the Top (recommended for tablets)", nullptr));
        toolbarAtBottomRadioButton->setText(
            PreferencesDialog::tr("Positioned at the Bottom (recommended for white boards)", nullptr));
        toolbarDisplayTextCheckBox->setText(PreferencesDialog::tr("Display Text Under Button", nullptr));
        documentsGroupBox->setTitle(PreferencesDialog::tr("Documents Mode", nullptr));
        showDateColumnOnAlphabeticalSort->setText(
            PreferencesDialog::tr("Display date column on alphabetical sort", nullptr));
        emptyTrashForOlderDocuments->setText(
            PreferencesDialog::tr("Empty trash for documents older than", nullptr));
        emptyTrashDaysLabel->setText(PreferencesDialog::tr("days", nullptr));
        mainTabWidget->setTabText(mainTabWidget->indexOf(displayTab),
                                  PreferencesDialog::tr("Display", nullptr));
        crossColorLightBackgroundLabel->setText(PreferencesDialog::tr("On Light Background", nullptr));
        darkBackgroundOpacityLabel->setText(PreferencesDialog::tr("Opacity", nullptr));
        crossColorDarkBackgroundLabel->setText(PreferencesDialog::tr("On Dark Background", nullptr));
        lightBackgroundOpacityLabel->setText(PreferencesDialog::tr("Opacity", nullptr));
        mainTabWidget->setTabText(mainTabWidget->indexOf(gridTab), PreferencesDialog::tr("Grid", nullptr));
        mainTabWidget->setTabText(mainTabWidget->indexOf(penTab), PreferencesDialog::tr("Pen", nullptr));
        mainTabWidget->setTabText(mainTabWidget->indexOf(markerTab),
                                  PreferencesDialog::tr("Marker", nullptr));
        internetGroupBox->setTitle(PreferencesDialog::tr("Internet", nullptr));
        useExternalBrowserCheckBox->setText(
            PreferencesDialog::tr("Show Page with External Browser", nullptr));
        labelHomePage->setText(PreferencesDialog::tr("Home Page:", nullptr));
        label->setText(PreferencesDialog::tr("Proxy User:", nullptr));
        label_3->setText(PreferencesDialog::tr("Pass:", nullptr));
        mainTabWidget->setTabText(mainTabWidget->indexOf(networkTab),
                                  PreferencesDialog::tr("Network", nullptr));
        mainTabWidget->setTabText(mainTabWidget->indexOf(aboutTab), PreferencesDialog::tr("About", nullptr));
    } // retranslateUi
};

struct PreferencesDialog::Private {
    std::unique_ptr<Ui> ui;
};


PreferencesDialog::PreferencesDialog(PreferencesController *prefController, QWidget *parent,
                                     Qt::WindowFlags f)
    : QDialog(parent, f), mPreferencesController(prefController), d(new Private) {
    d->ui = std::make_unique<Ui>();
    d->ui->setupUi(this);

    connect(LanguageManager::instance(), &LanguageManager::languageChanged, this,
            [this](const QString &) { d->ui->retranslateUi(this); });
}

PreferencesDialog::~PreferencesDialog() = default;

PreferencesDialog::Ui &PreferencesDialog::ui() {
    return *d->ui;
}

void PreferencesDialog::closeEvent(QCloseEvent *e) {
    e->accept();
}


PreferencesController::PreferencesController(QWidget *parent)
    : QObject(parent),
      mPreferencesWindow(0),
      mPreferencesUI(0),
      mPenProperties(0),
      mMarkerProperties(0),
      mDarkBackgroundGridColorPicker(0),
      mLightBackgroundGridColorPicker(0) {
    mDesktop = VirtualDesktop::instance();
    mPreferencesWindow = new PreferencesDialog(this, parent, Qt::Dialog);
    // The dialog owns its form; this is a borrowed pointer, never deleted here.
    mPreferencesUI = &mPreferencesWindow->ui();
    adjustScreens(1);
    connect(mDesktop, &VirtualDesktop::screenCountChanged, this, &PreferencesController::adjustScreens);

    wire();
}


PreferencesController::~PreferencesController() {
    delete mPreferencesWindow;

    delete mPenProperties;

    delete mMarkerProperties;
}

void PreferencesController::adjustScreens(int screen) {
    Q_UNUSED(screen);
    DisplayManager displayManager;
    mPreferencesUI->multiDisplayGroupBox->setEnabled(displayManager.numScreens() > 1);
}

void PreferencesController::show() {
    init();

    mPreferencesWindow->exec();
}

void PreferencesController::wire() {
    Settings *settings = Settings::settings();

    // main tab
    mPreferencesUI->mainTabWidget->setCurrentWidget(mPreferencesUI->displayTab);

    connect(mPreferencesUI->closeButton, &QPushButton::released, this, &PreferencesController::close);
    connect(mPreferencesUI->defaultSettingsButton, &QPushButton::released, this,
            &PreferencesController::defaultSettings);


    // OSK preferences

    for (int i = 0; i < settings->supportedKeyboardSizes->size(); i++)
        mPreferencesUI->keyboardPaletteKeyButtonSize->addItem(settings->supportedKeyboardSizes->at(i));


    connect(mPreferencesUI->useSystemOSKCheckBox, &QCheckBox::clicked, settings->useSystemOnScreenKeyboard,
            &Setting::setBool);
    connect(mPreferencesUI->useSystemOSKCheckBox, &QCheckBox::clicked, this,
            &PreferencesController::systemOSKCheckBoxToggled);

    // Documents Mode preferences
    connect(mPreferencesUI->showDateColumnOnAlphabeticalSort, &QCheckBox::clicked,
            settings->showDateColumnOnAlphabeticalSort, &Setting::setBool);
    connect(mPreferencesUI->showDateColumnOnAlphabeticalSort, &QCheckBox::clicked,
            Application::documentController, &DocumentController::refreshDateColumns);
    connect(mPreferencesUI->emptyTrashForOlderDocuments, &QCheckBox::clicked,
            settings->emptyTrashForOlderDocuments, &Setting::setBool);
    connect(mPreferencesUI->emptyTrashDaysValue, &QSpinBox::valueChanged, settings->emptyTrashDaysValue,
            &Setting::setInt);


    connect(mPreferencesUI->keyboardPaletteKeyButtonSize, &QComboBox::currentTextChanged,
            settings->boardKeyboardPaletteKeyBtnSize, &Setting::setString);
    connect(mPreferencesUI->startModeComboBox, &QComboBox::currentIndexChanged, settings->appStartMode,
            &Setting::setInt);

    connect(mPreferencesUI->languageComboBox, qOverload<int>(&QComboBox::activated), this, [this](int index) {
        LanguageManager::instance()->setLanguage(
            mPreferencesUI->languageComboBox->itemData(index).toString());
    });

    connect(mPreferencesUI->useExternalBrowserCheckBox, &QCheckBox::clicked, settings->webUseExternalBrowser,
            &Setting::setBool);
    connect(mPreferencesUI->displayBrowserPageCheckBox, &QCheckBox::clicked,
            settings->webShowPageImmediatelyOnMirroredScreen, &Setting::setBool);
    connect(mPreferencesUI->swapControlAndDisplayScreensCheckBox, &QCheckBox::clicked,
            settings->swapControlAndDisplayScreens, &Setting::setBool);
    connect(mPreferencesUI->swapControlAndDisplayScreensCheckBox, &QCheckBox::clicked,
            Application::applicationController->displayManager(), &DisplayManager::reinitScreens);
    if (settings->appHideSwapDisplayScreens->get().toBool()) {
        mPreferencesUI->swapDisplayScreensCheckBox->hide();
    } else {
        connect(mPreferencesUI->swapDisplayScreensCheckBox, &QCheckBox::clicked,
                Application::applicationController->displayManager(), &DisplayManager::swapDisplayScreens);
    }

    connect(mPreferencesUI->toolbarAtTopRadioButton, &QRadioButton::clicked, this,
            &PreferencesController::toolbarPositionChanged);
    connect(mPreferencesUI->toolbarAtBottomRadioButton, &QRadioButton::clicked, this,
            &PreferencesController::toolbarPositionChanged);
    connect(mPreferencesUI->horizontalChoice, &QRadioButton::clicked, this,
            &PreferencesController::toolbarOrientationHorizontal);
    connect(mPreferencesUI->verticalChoice, &QRadioButton::clicked, this,
            &PreferencesController::toolbarOrientationVertical);
    connect(mPreferencesUI->toolbarDisplayTextCheckBox, &QCheckBox::clicked, settings->appToolBarDisplayText,
            &Setting::setBool);

    //grid tab
    //On light background
    QPalette lightBackgroundPalette = QApplication::palette();
    lightBackgroundPalette.setColor(QPalette::Window, Qt::white);

    mPreferencesUI->crossColorLightBackgroundFrame->setAutoFillBackground(true);
    mPreferencesUI->crossColorLightBackgroundFrame->setPalette(lightBackgroundPalette);

    QPalette darkBackgroundPalette = QApplication::palette();
    darkBackgroundPalette.setColor(QPalette::Window, Qt::black);
    darkBackgroundPalette.setColor(QPalette::ButtonText, Qt::white);
    darkBackgroundPalette.setColor(QPalette::WindowText, Qt::white);

    mPreferencesUI->crossColorDarkBackgroundFrame->setAutoFillBackground(true);
    mPreferencesUI->crossColorDarkBackgroundFrame->setPalette(darkBackgroundPalette);
    mPreferencesUI->crossColorDarkBackgroundLabel->setPalette(darkBackgroundPalette);

    QList<QColor> gridLightBackgroundColors = settings->boardGridLightBackgroundColors->colors();
    QColor selectedCrossColorLightBackground(settings->boardCrossColorLightBackground->get().toString());

    mLightBackgroundGridColorPicker = new ColorPicker(mPreferencesUI->crossColorLightBackgroundFrame);
    mLightBackgroundGridColorPicker->setObjectName(QString::fromUtf8("crossColorLightBackgroundFrame"));
    mLightBackgroundGridColorPicker->setMinimumSize(QSize(32, 32));
    mLightBackgroundGridColorPicker->setFrameShape(QFrame::StyledPanel);
    mLightBackgroundGridColorPicker->setFrameShadow(QFrame::Raised);

    mPreferencesUI->crossColorLightBackgroundLayout->addWidget(mLightBackgroundGridColorPicker);
    mLightBackgroundGridColorPicker->setColors(gridLightBackgroundColors);
    mLightBackgroundGridColorPicker->setSelectedColorIndex(
        gridLightBackgroundColors.indexOf(selectedCrossColorLightBackground));
    mPreferencesUI->lightBackgroundOpacitySlider->setValue(selectedCrossColorLightBackground.alpha() * 100 /
                                                           255);

    QObject::connect(mLightBackgroundGridColorPicker, &ColorPicker::colorSelected, this,
                     &PreferencesController::setCrossColorOnLightBackground);
    connect(mPreferencesUI->lightBackgroundOpacitySlider, &QSlider::valueChanged, this,
            &PreferencesController::lightBackgroundCrossOpacityValueChanged);

    //On dark background
    QList<QColor> gridDarkBackgroundColors = settings->boardGridDarkBackgroundColors->colors();
    QColor selectedCrossColorDarkBackground(settings->boardCrossColorDarkBackground->get().toString());

    mDarkBackgroundGridColorPicker = new ColorPicker(mPreferencesUI->crossColorDarkBackgroundFrame);
    mDarkBackgroundGridColorPicker->setObjectName(QString::fromUtf8("crossColorDarkBackgroundFrame"));
    mDarkBackgroundGridColorPicker->setMinimumSize(QSize(32, 32));
    mDarkBackgroundGridColorPicker->setFrameShape(QFrame::StyledPanel);
    mDarkBackgroundGridColorPicker->setFrameShadow(QFrame::Raised);

    mPreferencesUI->crossColorDarkBackgroundLayout->addWidget(mDarkBackgroundGridColorPicker);
    mDarkBackgroundGridColorPicker->setColors(gridDarkBackgroundColors);
    mDarkBackgroundGridColorPicker->setSelectedColorIndex(
        gridDarkBackgroundColors.indexOf(selectedCrossColorDarkBackground));
    mPreferencesUI->darkBackgroundOpacitySlider->setValue(selectedCrossColorDarkBackground.alpha() * 100 /
                                                          255);

    QObject::connect(mDarkBackgroundGridColorPicker, &ColorPicker::colorSelected, this,
                     &PreferencesController::setCrossColorOnDarkBackground);
    connect(mPreferencesUI->darkBackgroundOpacitySlider, &QSlider::valueChanged, this,
            &PreferencesController::darkBackgroundCrossOpacityValueChanged);

    // pen
    QList<QColor> penLightBackgroundColors = settings->boardPenLightBackgroundColors->colors();
    QList<QColor> penDarkBackgroundColors = settings->boardPenDarkBackgroundColors->colors();
    QList<QColor> penLightBackgroundSelectedColors =
        settings->boardPenLightBackgroundSelectedColors->colors();
    QList<QColor> penDarkBackgroundSelectedColors = settings->boardPenDarkBackgroundSelectedColors->colors();

    mPenProperties =
        new BrushPropertiesFrame(mPreferencesUI->penFrame, penLightBackgroundColors, penDarkBackgroundColors,
                                 penLightBackgroundSelectedColors, penDarkBackgroundSelectedColors, this);

    mPenProperties->ui().opacityFrame->hide();

    connect(mPenProperties->ui().fineSlider, &QSlider::valueChanged, this,
            &PreferencesController::widthSliderChanged);
    connect(mPenProperties->ui().mediumSlider, &QSlider::valueChanged, this,
            &PreferencesController::widthSliderChanged);
    connect(mPenProperties->ui().strongSlider, &QSlider::valueChanged, this,
            &PreferencesController::widthSliderChanged);
    connect(mPenProperties->ui().pressureSensitiveCheckBox, &QCheckBox::clicked, settings,
            &Settings::setPenPressureSensitive);
    connect(mPenProperties->ui().circleCheckBox, &QCheckBox::clicked, settings,
            &Settings::setPenPreviewCircle);
    connect(mPenProperties->ui().circleSpinBox, &QSpinBox::valueChanged, this,
            &PreferencesController::penPreviewFromSizeChanged);

    // marker
    QList<QColor> markerLightBackgroundColors = settings->boardMarkerLightBackgroundColors->colors();
    QList<QColor> markerDarkBackgroundColors = settings->boardMarkerDarkBackgroundColors->colors();
    QList<QColor> markerLightBackgroundSelectedColors =
        settings->boardMarkerLightBackgroundSelectedColors->colors();
    QList<QColor> markerDarkBackgroundSelectedColors =
        settings->boardMarkerDarkBackgroundSelectedColors->colors();

    mMarkerProperties = new BrushPropertiesFrame(
        mPreferencesUI->markerFrame, markerLightBackgroundColors, markerDarkBackgroundColors,
        markerLightBackgroundSelectedColors, markerDarkBackgroundSelectedColors, this);

    mMarkerProperties->ui().pressureSensitiveCheckBox->setText(tr("Marker is pressure sensitive"));

    mMarkerProperties->ui().circleFrame->hide();

    connect(mMarkerProperties->ui().fineSlider, &QSlider::valueChanged, this,
            &PreferencesController::widthSliderChanged);
    connect(mMarkerProperties->ui().mediumSlider, &QSlider::valueChanged, this,
            &PreferencesController::widthSliderChanged);
    connect(mMarkerProperties->ui().strongSlider, &QSlider::valueChanged, this,
            &PreferencesController::widthSliderChanged);
    connect(mMarkerProperties->ui().pressureSensitiveCheckBox, &QCheckBox::clicked, settings,
            &Settings::setMarkerPressureSensitive);
    connect(mMarkerProperties->ui().opacitySlider, &QSlider::valueChanged, this,
            &PreferencesController::opacitySliderChanged);
}

void PreferencesController::init() {
    Settings *settings = Settings::settings();

    // display tab
    for (int i = 0; i < mPreferencesUI->keyboardPaletteKeyButtonSize->count(); i++)
        if (mPreferencesUI->keyboardPaletteKeyButtonSize->itemText(i) ==
            settings->boardKeyboardPaletteKeyBtnSize->get().toString()) {
            mPreferencesUI->keyboardPaletteKeyButtonSize->setCurrentIndex(i);
            break;
        }

    mPreferencesUI->useSystemOSKCheckBox->setChecked(settings->useSystemOnScreenKeyboard->get().toBool());
    this->systemOSKCheckBoxToggled(mPreferencesUI->useSystemOSKCheckBox->isChecked());

    mPreferencesUI->showDateColumnOnAlphabeticalSort->setChecked(
        settings->showDateColumnOnAlphabeticalSort->get().toBool());
    mPreferencesUI->emptyTrashForOlderDocuments->setChecked(
        settings->emptyTrashForOlderDocuments->get().toBool());
    mPreferencesUI->emptyTrashDaysValue->setValue(settings->emptyTrashDaysValue->get().toInt());

    mPreferencesUI->startModeComboBox->setCurrentIndex(settings->appStartMode->get().toInt());

    QComboBox *languageBox = mPreferencesUI->languageComboBox;
    languageBox->blockSignals(true);
    languageBox->clear();
    const QList<LanguageManager::Language> availableLanguages = LanguageManager::instance()->languages();
    for (const LanguageManager::Language &language : availableLanguages)
        languageBox->addItem(language.name, language.code);

    const QString currentLanguage = LanguageManager::instance()->currentLanguage();
    for (int i = 0; i < languageBox->count(); i++) {
        if (languageBox->itemData(i).toString().compare(currentLanguage, Qt::CaseInsensitive) == 0) {
            languageBox->setCurrentIndex(i);
            break;
        }
    }
    languageBox->blockSignals(false);

    mPreferencesUI->useExternalBrowserCheckBox->setChecked(settings->webUseExternalBrowser->get().toBool());
    mPreferencesUI->displayBrowserPageCheckBox->setChecked(
        settings->webShowPageImmediatelyOnMirroredScreen->get().toBool());
    mPreferencesUI->swapControlAndDisplayScreensCheckBox->setChecked(
        settings->swapControlAndDisplayScreens->get().toBool());
    mPreferencesUI->webHomePage->setText(settings->webHomePage->get().toString());

    mPreferencesUI->proxyUsername->setText(settings->proxyUsername());
    mPreferencesUI->proxyPassword->setText(settings->proxyPassword());

    mPreferencesUI->toolbarAtTopRadioButton->setChecked(settings->appToolBarPositionedAtTop->get().toBool());
    mPreferencesUI->toolbarAtBottomRadioButton->setChecked(
        !settings->appToolBarPositionedAtTop->get().toBool());
    mPreferencesUI->toolbarDisplayTextCheckBox->setChecked(settings->appToolBarDisplayText->get().toBool());
    mPreferencesUI->verticalChoice->setChecked(settings->appToolBarOrientationVertical->get().toBool());
    mPreferencesUI->horizontalChoice->setChecked(!settings->appToolBarOrientationVertical->get().toBool());

    // pen tab
    mPenProperties->ui().fineSlider->setValue(settings->boardPenFineWidth->get().toDouble() * sSliderRatio);
    mPenProperties->ui().mediumSlider->setValue(settings->boardPenMediumWidth->get().toDouble() *
                                                sSliderRatio);
    mPenProperties->ui().strongSlider->setValue(settings->boardPenStrongWidth->get().toDouble() *
                                                sSliderRatio);
    mPenProperties->ui().pressureSensitiveCheckBox->setChecked(
        settings->boardPenPressureSensitive->get().toBool());
    mPenProperties->ui().circleCheckBox->setChecked(settings->showPenPreviewCircle->get().toBool());
    mPenProperties->ui().circleSpinBox->setValue(settings->penPreviewFromSize->get().toInt());

    // marker tab
    mMarkerProperties->ui().fineSlider->setValue(settings->boardMarkerFineWidth->get().toDouble() *
                                                 sSliderRatio);
    mMarkerProperties->ui().mediumSlider->setValue(settings->boardMarkerMediumWidth->get().toDouble() *
                                                   sSliderRatio);
    mMarkerProperties->ui().strongSlider->setValue(settings->boardMarkerStrongWidth->get().toDouble() *
                                                   sSliderRatio);
    mMarkerProperties->ui().pressureSensitiveCheckBox->setChecked(
        settings->boardMarkerPressureSensitive->get().toBool());

    mMarkerProperties->ui().opacitySlider->setValue(settings->boardMarkerAlpha->get().toDouble() * 100);
}

void PreferencesController::close() {
    //web
    QString homePage = mPreferencesUI->webHomePage->text();

    Settings::settings()->webHomePage->set(homePage);
    Settings::settings()->setProxyUsername(mPreferencesUI->proxyUsername->text());
    Settings::settings()->setProxyPassword(mPreferencesUI->proxyPassword->text());

    mPreferencesWindow->accept();
}


void PreferencesController::defaultSettings() {
    Settings *settings = Settings::settings();

    if (mPreferencesUI->mainTabWidget->currentWidget() == mPreferencesUI->displayTab) {
        bool defaultValue = settings->appToolBarPositionedAtTop->reset().toBool();
        mPreferencesUI->toolbarAtTopRadioButton->setChecked(defaultValue);
        mPreferencesUI->toolbarAtBottomRadioButton->setChecked(!defaultValue);

        defaultValue = settings->appToolBarDisplayText->reset().toBool();
        mPreferencesUI->toolbarDisplayTextCheckBox->setChecked(defaultValue);
        mPreferencesUI->verticalChoice->setChecked(settings->appToolBarOrientationVertical->reset().toBool());
        mPreferencesUI->horizontalChoice->setChecked(
            !settings->appToolBarOrientationVertical->reset().toBool());
        mPreferencesUI->startModeComboBox->setCurrentIndex(0);

        mPreferencesUI->useSystemOSKCheckBox->setChecked(
            settings->useSystemOnScreenKeyboard->reset().toBool());

        mPreferencesUI->showDateColumnOnAlphabeticalSort->setChecked(
            settings->showDateColumnOnAlphabeticalSort->reset().toBool());
        Application::documentController->refreshDateColumns();

        mPreferencesUI->emptyTrashForOlderDocuments->setChecked(
            settings->emptyTrashForOlderDocuments->reset().toBool());
        mPreferencesUI->emptyTrashDaysValue->setValue(settings->emptyTrashDaysValue->reset().toInt());

    } else if (mPreferencesUI->mainTabWidget->currentWidget() == mPreferencesUI->penTab) {
        mPenProperties->ui().fineSlider->setValue(settings->boardPenFineWidth->reset().toDouble() *
                                                  sSliderRatio);
        mPenProperties->ui().mediumSlider->setValue(settings->boardPenMediumWidth->reset().toDouble() *
                                                    sSliderRatio);
        mPenProperties->ui().strongSlider->setValue(settings->boardPenStrongWidth->reset().toDouble() *
                                                    sSliderRatio);
        mPenProperties->ui().pressureSensitiveCheckBox->setChecked(
            settings->boardPenPressureSensitive->reset().toBool());
        mPenProperties->ui().circleCheckBox->setChecked(settings->showPenPreviewCircle->reset().toBool());
        mPenProperties->ui().circleSpinBox->setValue(settings->penPreviewFromSize->reset().toInt());

        settings->boardPenLightBackgroundSelectedColors->reset();
        QList<QColor> lightBackgroundSelectedColors =
            settings->boardPenLightBackgroundSelectedColors->colors();

        settings->boardPenDarkBackgroundSelectedColors->reset();
        QList<QColor> darkBackgroundSelectedColors = settings->boardPenDarkBackgroundSelectedColors->colors();

        for (int i = 0; i < settings->colorPaletteSize; i++) {
            mPenProperties->lightBackgroundColorPickers()[i]->setSelectedColorIndex(
                lightBackgroundSelectedColors.indexOf(settings->penColors(false).at(i)));
            mPenProperties->darkBackgroundColorPickers()[i]->setSelectedColorIndex(
                darkBackgroundSelectedColors.indexOf(settings->penColors(true).at(i)));
        }
    } else if (mPreferencesUI->mainTabWidget->currentWidget() == mPreferencesUI->markerTab) {
        mMarkerProperties->ui().fineSlider->setValue(settings->boardMarkerFineWidth->reset().toDouble() *
                                                     sSliderRatio);
        mMarkerProperties->ui().mediumSlider->setValue(settings->boardMarkerMediumWidth->reset().toDouble() *
                                                       sSliderRatio);
        mMarkerProperties->ui().strongSlider->setValue(settings->boardMarkerStrongWidth->reset().toDouble() *
                                                       sSliderRatio);
        mMarkerProperties->ui().pressureSensitiveCheckBox->setChecked(
            settings->boardMarkerPressureSensitive->reset().toBool());

        mMarkerProperties->ui().opacitySlider->setValue(settings->boardMarkerAlpha->reset().toDouble() * 100);

        settings->boardMarkerLightBackgroundSelectedColors->reset();
        QList<QColor> lightBackgroundSelectedColors =
            settings->boardMarkerLightBackgroundSelectedColors->colors();

        settings->boardMarkerDarkBackgroundSelectedColors->reset();
        QList<QColor> darkBackgroundSelectedColors =
            settings->boardMarkerDarkBackgroundSelectedColors->colors();

        for (int i = 0; i < settings->colorPaletteSize; i++) {
            mMarkerProperties->lightBackgroundColorPickers()[i]->setSelectedColorIndex(
                lightBackgroundSelectedColors.indexOf(settings->markerColors(false).at(i)));
            mMarkerProperties->darkBackgroundColorPickers()[i]->setSelectedColorIndex(
                darkBackgroundSelectedColors.indexOf(settings->markerColors(true).at(i)));
        }
    } else if (mPreferencesUI->mainTabWidget->currentWidget() == mPreferencesUI->networkTab) {
        bool defaultValue = settings->webUseExternalBrowser->reset().toBool();
        mPreferencesUI->useExternalBrowserCheckBox->setChecked(defaultValue);
        defaultValue = settings->webShowPageImmediatelyOnMirroredScreen->reset().toBool();
        mPreferencesUI->displayBrowserPageCheckBox->setChecked(defaultValue);

        mPreferencesUI->webHomePage->setText(settings->webHomePage->reset().toString());
    } else if (mPreferencesUI->mainTabWidget->currentWidget() == mPreferencesUI->gridTab) {
        settings->boardCrossColorDarkBackground->reset();
        settings->boardGridDarkBackgroundColors->reset();

        QList<QColor> gridDarkBackgroundColors = settings->boardGridDarkBackgroundColors->colors();
        QColor selectedCrossColorDarkBackground(settings->boardCrossColorDarkBackground->get().toString());

        mDarkBackgroundGridColorPicker->setColors(gridDarkBackgroundColors);
        mDarkBackgroundGridColorPicker->setSelectedColorIndex(
            gridDarkBackgroundColors.indexOf(selectedCrossColorDarkBackground));
        int darkBackgroundOpacity = selectedCrossColorDarkBackground.alpha() * 100 / 255;
        mPreferencesUI->darkBackgroundOpacitySlider->setValue(darkBackgroundOpacity);
        darkBackgroundCrossOpacityValueChanged(darkBackgroundOpacity);

        settings->boardCrossColorLightBackground->reset();
        settings->boardGridLightBackgroundColors->reset();

        QList<QColor> gridLightBackgroundColors = settings->boardGridLightBackgroundColors->colors();
        QColor selectedCrossColorLightBackground(settings->boardCrossColorLightBackground->get().toString());

        mLightBackgroundGridColorPicker->setColors(gridLightBackgroundColors);
        mLightBackgroundGridColorPicker->setSelectedColorIndex(
            gridLightBackgroundColors.indexOf(selectedCrossColorLightBackground));
        int lightBackgroundOpacity = selectedCrossColorLightBackground.alpha() * 100 / 255;
        mPreferencesUI->lightBackgroundOpacitySlider->setValue(lightBackgroundOpacity);
        lightBackgroundCrossOpacityValueChanged(lightBackgroundOpacity);
    }
}

void PreferencesController::darkBackgroundCrossOpacityValueChanged(int value) {
    Settings *settings = Settings::settings();
    int opacity = value * 255 / 100;

    QList<QColor> gridDarkBackgroundColors = settings->boardGridDarkBackgroundColors->colors();

    int index = mDarkBackgroundGridColorPicker->selectedColorIndex();
    QColor currentColor = gridDarkBackgroundColors.at(index);
    currentColor.setAlpha(opacity);
    gridDarkBackgroundColors.replace(index, currentColor);
    mDarkBackgroundGridColorPicker->setColors(gridDarkBackgroundColors);

    settings->boardGridDarkBackgroundColors->setColor(index, currentColor);

    Settings::settings()->boardCrossColorDarkBackground->set(currentColor.name(QColor::HexArgb));

    if (Application::boardController && Application::boardController->activeScene()) {
        foreach (QGraphicsView *view, Application::boardController->activeScene()->views())
            view->resetCachedContent();
    }
}

void PreferencesController::lightBackgroundCrossOpacityValueChanged(int value) {
    Settings *settings = Settings::settings();
    int opacity = value * 255 / 100;

    QList<QColor> gridLightBackgroundColors = settings->boardGridLightBackgroundColors->colors();

    int index = mLightBackgroundGridColorPicker->selectedColorIndex();
    QColor currentColor = gridLightBackgroundColors.at(index);
    currentColor.setAlpha(opacity);
    gridLightBackgroundColors.replace(index, currentColor);
    mLightBackgroundGridColorPicker->setColors(gridLightBackgroundColors);

    settings->boardGridLightBackgroundColors->setColor(index, currentColor);

    Settings::settings()->boardCrossColorLightBackground->set(currentColor.name(QColor::HexArgb));

    if (Application::boardController && Application::boardController->activeScene()) {
        foreach (QGraphicsView *view, Application::boardController->activeScene()->views())
            view->resetCachedContent();
    }
}

void PreferencesController::penPreviewFromSizeChanged(int value) {
    Settings::settings()->setPenPreviewFromSize(value);
}

void PreferencesController::widthSliderChanged(int value) {
    Settings *settings = Settings::settings();

    qreal width = value / sSliderRatio;

    QObject *slider = sender();
    CircleFrame *display = 0;

    if (slider == mPenProperties->ui().fineSlider) {
        settings->boardPenFineWidth->set(width);
        display = mPenProperties->ui().fineDisplayFrame;
    } else if (slider == mPenProperties->ui().mediumSlider) {
        settings->boardPenMediumWidth->set(width);
        display = mPenProperties->ui().mediumDisplayFrame;
    } else if (slider == mPenProperties->ui().strongSlider) {
        settings->boardPenStrongWidth->set(width);
        display = mPenProperties->ui().strongDisplayFrame;
    } else if (slider == mMarkerProperties->ui().fineSlider) {
        settings->boardMarkerFineWidth->set(width);
        display = mMarkerProperties->ui().fineDisplayFrame;
    } else if (slider == mMarkerProperties->ui().mediumSlider) {
        settings->boardMarkerMediumWidth->set(width);
        display = mMarkerProperties->ui().mediumDisplayFrame;
    } else if (slider == mMarkerProperties->ui().strongSlider) {
        settings->boardMarkerStrongWidth->set(width);
        display = mMarkerProperties->ui().strongDisplayFrame;
    } else {
        qDebug() << "widthSliderChanged : unknown sender ... ignoring event";
    }

    if (display) {
        display->setCurrentPenWidth(width);
        display->setMaxPenWidth(sMaxPenWidth);
        display->repaint();
    }
}


void PreferencesController::opacitySliderChanged(int value) {
    qreal opacity = ((qreal)value) / 100;

    QObject *slider = sender();

    if (slider == mMarkerProperties->ui().opacitySlider) {
        DrawingController::drawingController()->setMarkerAlpha(opacity);
    }
}


void PreferencesController::colorSelected(const QColor &color) {
    ColorPicker *colorPicker = qobject_cast<ColorPicker *>(sender());

    int index = mPenProperties->lightBackgroundColorPickers().indexOf(colorPicker);

    if (index >= 0) {
        DrawingController::drawingController()->setPenColor(false, color, index);
        return;
    }

    index = mPenProperties->darkBackgroundColorPickers().indexOf(colorPicker);

    if (index >= 0) {
        DrawingController::drawingController()->setPenColor(true, color, index);
    }

    index = mMarkerProperties->lightBackgroundColorPickers().indexOf(colorPicker);

    if (index >= 0) {
        DrawingController::drawingController()->setMarkerColor(false, color, index);
    }

    index = mMarkerProperties->darkBackgroundColorPickers().indexOf(colorPicker);

    if (index >= 0) {
        DrawingController::drawingController()->setMarkerColor(true, color, index);
    }
}

void PreferencesController::setCrossColorOnDarkBackground(const QColor &color) {
    Settings::settings()->boardCrossColorDarkBackground->set(color.name(QColor::HexArgb));

    mPreferencesUI->darkBackgroundOpacitySlider->setValue(color.alpha() * 100 / 255);

    if (Application::boardController && Application::boardController->activeScene()) {
        foreach (QGraphicsView *view, Application::boardController->activeScene()->views())
            view->resetCachedContent();
    }
}

void PreferencesController::setCrossColorOnLightBackground(const QColor &color) {
    Settings::settings()->boardCrossColorLightBackground->set(color.name(QColor::HexArgb));

    mPreferencesUI->lightBackgroundOpacitySlider->setValue(color.alpha() * 100 / 255);

    if (Application::boardController && Application::boardController->activeScene()) {
        foreach (QGraphicsView *view, Application::boardController->activeScene()->views())
            view->resetCachedContent();
    }
}

void PreferencesController::toolbarPositionChanged(bool checked) {
    Q_UNUSED(checked);

    Settings *settings = Settings::settings();

    settings->appToolBarPositionedAtTop->set(mPreferencesUI->toolbarAtTopRadioButton->isChecked());
}

void PreferencesController::toolbarOrientationVertical(bool checked) {
    Settings *settings = Settings::settings();
    settings->appToolBarOrientationVertical->set(checked);
}

void PreferencesController::toolbarOrientationHorizontal(bool checked) {
    Settings *settings = Settings::settings();
    settings->appToolBarOrientationVertical->set(!checked);
}

void PreferencesController::systemOSKCheckBoxToggled(bool checked) {
    mPreferencesUI->keyboardPaletteKeyButtonSize->setVisible(!checked);
    mPreferencesUI->keyboardPaletteKeyButtonSize_Label->setVisible(!checked);
}

BrushPropertiesFrame::BrushPropertiesFrame(QFrame *owner, const QList<QColor> &lightBackgroundColors,
                                           const QList<QColor> &darkBackgroundColors,
                                           const QList<QColor> &lightBackgroundSelectedColors,
                                           const QList<QColor> &darkBackgroundSelectedColors,
                                           PreferencesController *controller)
    : d(new Private) {
    d->ui.setupUi(owner);

    Ui &ui = d->ui;

    QPalette lightBackgroundPalette = QApplication::palette();
    lightBackgroundPalette.setColor(QPalette::Window, Qt::white);

    ui.lightBackgroundFrame->setAutoFillBackground(true);
    ui.lightBackgroundFrame->setPalette(lightBackgroundPalette);

    QPalette darkBackgroundPalette = QApplication::palette();
    darkBackgroundPalette.setColor(QPalette::Window, Qt::black);
    darkBackgroundPalette.setColor(QPalette::ButtonText, Qt::white);
    darkBackgroundPalette.setColor(QPalette::WindowText, Qt::white);

    ui.darkBackgroundFrame->setAutoFillBackground(true);
    ui.darkBackgroundFrame->setPalette(darkBackgroundPalette);
    ui.darkBackgroundLabel->setPalette(darkBackgroundPalette);

    QList<QColor> firstLightBackgroundColor;
    firstLightBackgroundColor.append(lightBackgroundColors[0]);

    ui.lightBackgroundColorPicker0->setColors(firstLightBackgroundColor);
    ui.lightBackgroundColorPicker0->setSelectedColorIndex(0);
    d->lightBackgroundColorPickers.append(ui.lightBackgroundColorPicker0);

    for (int i = 1; i < Settings::settings()->colorPaletteSize; i++) {
        ColorPicker *picker = new ColorPicker(ui.lightBackgroundFrame);
        picker->setObjectName(QString::fromUtf8("penLightBackgroundColor") + QString::number(i));
        picker->setMinimumSize(QSize(32, 32));
        picker->setFrameShape(QFrame::StyledPanel);
        picker->setFrameShadow(QFrame::Raised);

        ui.lightBackgroundLayout->addWidget(picker);

        picker->setColors(lightBackgroundColors);

        picker->setSelectedColorIndex(lightBackgroundColors.indexOf(lightBackgroundSelectedColors.at(i)));

        d->lightBackgroundColorPickers.append(picker);

        QObject::connect(picker, &ColorPicker::colorSelected, controller,
                         &PreferencesController::colorSelected);
    }

    QList<QColor> firstDarkBackgroundColor;
    firstDarkBackgroundColor.append(darkBackgroundColors[0]);

    ui.darkBackgroundColorPicker0->setColors(firstDarkBackgroundColor);
    ui.darkBackgroundColorPicker0->setSelectedColorIndex(0);
    d->darkBackgroundColorPickers.append(ui.darkBackgroundColorPicker0);

    for (int i = 1; i < Settings::settings()->colorPaletteSize; i++) {
        ColorPicker *picker = new ColorPicker(ui.darkBackgroundFrame);
        picker->setObjectName(QString::fromUtf8("penDarkBackgroundColor") + QString::number(i));
        picker->setMinimumSize(QSize(32, 32));
        picker->setFrameShape(QFrame::StyledPanel);
        picker->setFrameShadow(QFrame::Raised);

        ui.darkBackgroundLayout->addWidget(picker);

        picker->setColors(darkBackgroundColors);
        picker->setSelectedColorIndex(darkBackgroundColors.indexOf(darkBackgroundSelectedColors.at(i)));

        d->darkBackgroundColorPickers.append(picker);

        QObject::connect(picker, &ColorPicker::colorSelected, controller,
                         &PreferencesController::colorSelected);
    }
}

BrushPropertiesFrame::~BrushPropertiesFrame() = default;

BrushPropertiesFrame::Ui &BrushPropertiesFrame::ui() {
    return d->ui;
}

QList<ColorPicker *> &BrushPropertiesFrame::lightBackgroundColorPickers() {
    return d->lightBackgroundColorPickers;
}

QList<ColorPicker *> &BrushPropertiesFrame::darkBackgroundColorPickers() {
    return d->darkBackgroundColorPickers;
}
