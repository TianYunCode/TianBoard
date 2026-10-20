#include <QtWidgets>
#include <QList>
#include <QSize>

#include "ui/KeyboardPalette.h"
#include "settings/Settings.h"

#include "platform/KeyboardSender.h"

#include "core/Application.h"
#include "ui/MainWindow.h"


struct KeyboardPalette::Private {
    BTNImages *currBtnImages{nullptr};

    bool m_isVisible{false};
    QPoint m_pos;

    bool capsLock{false};
    bool shift{false};
    int nCurrentLocale{0};
    int nLocalesCount{0};
    KeyboardLocale **locales{nullptr};

    int nSpecialModifierIndex{0};
    KEYCODE specialModifier{0};

    QString strSize;
    int btnWidth{0};
    int btnHeight{0};

    bool languagePopupActive{false};
    bool keyboardActive{false};

    QRect originalRect;

    KeyButton **buttons{nullptr};
    KeyboardButton **ctrlButtons{nullptr};

    // 平台相关对象的存储位置（Linux 等平台会用到）
    void *storage{nullptr};
    // 平台按键码范围与编码宽度
    int min_keycodes{0};
    int max_keycodes{0};
    int byte_per_code{0};
};

struct KeyboardButton::Private {
    KeyboardPalette *m_parent{nullptr};
    ContentImage *imgContent{nullptr};
    QString m_contentImagePath;
    KeyboardPalette *keyboard{nullptr};
    bool bFocused{false};
    bool bPressed{false};
};

struct KeyButton::Private {
    const KEYBT *keybt{nullptr};
};

struct CntrlButton::Private {
    QString label;
    int code{0};
};

struct LocaleButton::Private {
    QMenu *localeMenu{nullptr};
};

KeyboardPalette::KeyboardPalette(QWidget *parent)
    : ActionPalette(Qt::TopRightCorner, parent), d(new Private) {
    // 关闭动作交给外部：显隐始终跟随“虚拟键盘”QAction 的勾选状态。
    setCustomCloseProcessing(true);
    setCustomPosition(true);
    setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
    setFocusPolicy(Qt::NoFocus);
    setClosable(true);
    setGrip(false);

    d->capsLock = false;
    d->shift = false;
    d->languagePopupActive = false;
    d->keyboardActive = false;
    d->nSpecialModifierIndex = 0;
    d->specialModifier = 0;
    d->btnWidth = d->btnHeight = 16;
    d->strSize = "16x16";
    d->currBtnImages = new BTNImages("16", d->btnWidth, d->btnHeight);
    d->storage = NULL;
    d->nLocalesCount = 0;

    // 标准 101 键盘的主键区固定为 47 个字符键，先全部创建，布局数据稍后绑定。
    d->buttons = new KeyButton *[47];
    for (int i = 0; i < 47; i++) {
        d->buttons[i] = new KeyButton(this);
    }

    d->locales = PlatformUtils::getKeyboardLayouts(d->nLocalesCount);

    createCtrlButtons();

    // 恢复上次使用的输入法；越界时退回第一个布局。
    d->nCurrentLocale = Settings::settings()->KeyboardLocale->get().toInt();
    if (d->nCurrentLocale < 0 || d->nCurrentLocale >= d->nLocalesCount)
        d->nCurrentLocale = 0;
    if (d->locales != NULL)
        setInput(d->locales[d->nCurrentLocale]);

    setContentsMargins(22, 22, 22, 22);

    init();
}

void KeyboardPalette::createCtrlButtons() {
    d->ctrlButtons = new KeyboardButton *[9];

    // 顺序必须与 paintEvent() 中的布局序号严格一致。
    int ctrlID = 0;
    d->ctrlButtons[ctrlID++] = new CntrlButton(this, KeyboardSender::Backspace, "backspace");
    d->ctrlButtons[ctrlID++] = new CntrlButton(this, KeyboardSender::Tab, "tab");
    d->ctrlButtons[ctrlID++] = new CapsLockButton(this, "capslock"); // Caps Lock
    d->ctrlButtons[ctrlID++] = new CntrlButton(this, tr("Enter"), KeyboardSender::Enter);
    d->ctrlButtons[ctrlID++] = new ShiftButton(this, "shift");
    d->ctrlButtons[ctrlID++] = new ShiftButton(this, "shift");
    d->ctrlButtons[ctrlID++] = new LocaleButton(this); // 切换输入法
    d->ctrlButtons[ctrlID++] = new CntrlButton(this, "", KeyboardSender::Space);
    d->ctrlButtons[ctrlID++] = new LocaleButton(this);
}

void KeyboardPalette::init() {
    d->m_isVisible = false;
    setVisible(false);

    setKeyButtonSize(Settings::settings()->boardKeyboardPaletteKeyBtnSize->get().toString());

    connect(Settings::settings()->boardKeyboardPaletteKeyBtnSize, &Setting::changed, this,
            &KeyboardPalette::keyboardPaletteButtonSizeChanged);
    connect(Application::mainWindow->ui().actionVirtualKeyboard, &QAction::triggered, this,
            &KeyboardPalette::showKeyboard);
    connect(this, &ActionPalette::closed, this, &KeyboardPalette::hideKeyboard);

    // 键盘不能抢走前台应用的焦点，否则发送的按键会落回键盘自身。
    PlatformUtils::setWindowNonActivableFlag(this, true);
}

void KeyboardPalette::showKeyboard(bool show) {
    d->m_isVisible = show;
}

bool KeyboardPalette::keyboardVisible() const {
    return d->m_isVisible;
}


void KeyboardPalette::hideKeyboard() {
    Application::mainWindow->ui().actionVirtualKeyboard->activate(QAction::Trigger);
}


void KeyboardPalette::keyboardPaletteButtonSizeChanged(QVariant size) {
    setKeyButtonSize(size.toString());
}

void KeyboardPalette::setInput(const KeyboardLocale *locale) {
    if (locale != NULL) {
        for (int i = 0; i < 47; i++)
            d->buttons[i]->setKeyBt((*locale)[i]);
    } else {
        this->hide();
    }
}

KeyboardPalette::~KeyboardPalette() {
    // 各按键控件由 Qt 父控件机制销毁，这里只释放裸指针数组本身。
    delete[] d->buttons;

    delete[] d->ctrlButtons;

    if (d->currBtnImages != NULL) {
        delete d->currBtnImages;
        d->currBtnImages = NULL;
    }
}

bool KeyboardPalette::isEnabled() {
    return d->locales != NULL;
}

QSize KeyboardPalette::sizeHint() const {
    int w = contentsMargins().left() + 15 * d->btnWidth + contentsMargins().right();
    int h = contentsMargins().top() + 5 * d->btnHeight + contentsMargins().bottom();
    return QSize(w, h);
}

const QString *KeyboardPalette::getLocaleName() {
    return d->locales == NULL ? NULL : &(d->locales[d->nCurrentLocale]->name);
}

void KeyboardPalette::setLocale(int nLocale) {
    if (d->locales != NULL) {
        d->nCurrentLocale = nLocale;

        setInput(d->locales[d->nCurrentLocale]);
        update();

        Settings::settings()->KeyboardLocale->set(d->nCurrentLocale);
    }
    emit localeChanged(nLocale);
}

QString KeyboardPalette::getKeyButtonSize() const {
    return QString("%1x%2").arg(d->btnWidth).arg(d->btnHeight);
}

void KeyboardPalette::setKeyButtonSize(const QString &_strSize) {
    QStringList strs = _strSize.split('x');

    if (strs.size() == 2) {
        d->strSize = _strSize;
        d->btnWidth = strs[0].toInt();
        d->btnHeight = strs[1].toInt();

        if (d->currBtnImages != NULL)
            delete d->currBtnImages;
        d->currBtnImages = new BTNImages(strs[1], d->btnWidth, d->btnHeight);

        adjustSizeAndPosition();
    }
}

void KeyboardPalette::enterEvent(QEvent *) {
    if (d->keyboardActive)
        return;

    d->keyboardActive = true;

    adjustSizeAndPosition();

    emit keyboardActivated(true);
}

void KeyboardPalette::leaveEvent(QEvent *) {
    if (d->languagePopupActive || !d->keyboardActive || isMoving())
        return;

    d->keyboardActive = false;

    adjustSizeAndPosition();

    emit keyboardActivated(false);
}

void KeyboardPalette::moveEvent(QMoveEvent *event) {
    ActionPalette::moveEvent(event);
    emit moved(event->pos());
}

void KeyboardPalette::adjustSizeAndPosition(bool pUp) {
    QSize rSize = sizeHint();
    if (rSize != size()) {
        int dx = (rSize.width() - size().width()) / 2;
        int dy = rSize.height() - size().height();

        this->move(x() - dx, y() - dy);
        this->resize(rSize.width(), rSize.height());
    }
    ActionPalette::adjustSizeAndPosition(pUp);
}

void KeyboardPalette::paintEvent(QPaintEvent *event) {
    ActionPalette::paintEvent(event);

    QRect r = this->geometry();

    const QMargins paletteMargins = contentsMargins();
    int lleft = paletteMargins.left(), ltop = paletteMargins.top(), lright = paletteMargins.right(),
        lbottom = paletteMargins.bottom();

    // 计算 15x5 键区在面板内的居中起点
    int ctrlButtonsId = 0;
    lleft = (r.width() - d->btnWidth * 15) / 2;
    ltop = (r.height() - d->btnHeight * 5) / 2;

    // 第 1 行：13 个字符键加退格
    int offX = lleft;
    int offY = ltop;

    // 字符键 [`]..[+]
    for (int i = 0; i < 13; i++) {
        d->buttons[i]->setGeometry(offX, offY, d->btnWidth, d->btnHeight);
        offX += d->btnWidth;
    }

    // 退格键占两格宽
    d->ctrlButtons[ctrlButtonsId++]->setGeometry(offX, offY, d->btnWidth * 2, d->btnHeight);
    offX += d->btnWidth * 2;

    // 第 2 行：Tab 后接 12 个字符键
    offX = lleft;
    offY += d->btnHeight;
    offX += d->btnWidth / 2;

    // Tab 键占两格宽
    d->ctrlButtons[ctrlButtonsId++]->setGeometry(offX, offY, d->btnWidth * 2, d->btnHeight);
    offX += d->btnWidth * 2;

    // 字符键 [q]..[]]
    for (int i = 0; i < 12; i++) {
        d->buttons[i + 13]->setGeometry(offX, offY, d->btnWidth, d->btnHeight);
        offX += d->btnWidth;
    }

    // 第 3 行：CapsLock 风格的左 Enter（一格宽）后接 12 个字符键，再接右 Enter（两格宽）
    offX = lleft;
    offY += d->btnHeight;

    // 左 Enter
    d->ctrlButtons[ctrlButtonsId++]->setGeometry(offX, offY, d->btnWidth * 1, d->btnHeight);
    offX += d->btnWidth * 1;

    // 字符键 [a]..[\]
    for (int i = 0; i < 12; i++) {
        d->buttons[i + 12 + 13]->setGeometry(offX, offY, d->btnWidth, d->btnHeight);
        offX += d->btnWidth;
    }

    // 右 Enter
    d->ctrlButtons[ctrlButtonsId++]->setGeometry(offX, offY, d->btnWidth * 2, d->btnHeight);
    offX += d->btnWidth * 2;

    // 第 4 行：左右两个 CapsLock 夹住 10 个字符键
    offX = lleft;
    offY += d->btnHeight;

    // 左 CapsLock
    d->ctrlButtons[ctrlButtonsId++]->setGeometry(offX, offY, d->btnWidth * 2.5, d->btnHeight);
    offX += d->btnWidth * 2.5;

    for (int i = 0; i < 10; i++) {
        d->buttons[i + 12 + 12 + 13]->setGeometry(offX, offY, d->btnWidth, d->btnHeight);
        offX += d->btnWidth;
    }

    // 右 CapsLock
    d->ctrlButtons[ctrlButtonsId++]->setGeometry(offX, offY, d->btnWidth * 2.5, d->btnHeight);
    offX += d->btnWidth * 2.5;

    // 第 5 行：左 Shift、空格、右 Shift
    offX = lleft;
    offY += d->btnHeight;

    d->ctrlButtons[ctrlButtonsId++]->setGeometry(offX + d->btnWidth * 1, offY, d->btnWidth * 2, d->btnHeight);
    d->ctrlButtons[ctrlButtonsId++]->setGeometry(offX + d->btnWidth * 3, offY, d->btnWidth * 9, d->btnHeight);
    d->ctrlButtons[ctrlButtonsId++]->setGeometry(offX + d->btnWidth * 12, offY, d->btnWidth * 2,
                                                 d->btnHeight);
}

BTNImages::BTNImages(QString strHeight, int width, int height) {
    m_strHeight = strHeight;
    m_width = width;
    m_height = height;

    m_strLeftPassive = ":/images/virtual.keyboard/" + strHeight + "/left-passive.png";
    m_strCenterPassive = ":/images/virtual.keyboard/" + strHeight + "/centre-passive.png";
    m_strRightPassive = ":/images/virtual.keyboard/" + strHeight + "/right-passive.png";
    m_strLeftActive = ":/images/virtual.keyboard/" + strHeight + "/left-active.png";
    m_strCenterActive = ":/images/virtual.keyboard/" + strHeight + "/centre-active.png";
    m_strRightActive = ":/images/virtual.keyboard/" + strHeight + "/right-active.png";

    m_btnLeftPassive = QImage(m_strLeftPassive);
    m_btnCenterPassive = QImage(m_strCenterPassive);
    m_btnRightPassive = QImage(m_strRightPassive);
    m_btnLeftActive = QImage(m_strLeftActive);
    m_btnCenterActive = QImage(m_strCenterActive);
    m_btnRightActive = QImage(m_strRightActive);
}

ContentImage::ContentImage(QString strHeight, int height, QString strContentName) {
    m_strHeight = strHeight;
    m_height = height;

    // 内容图标与按键贴图放在同一尺寸目录下。
    m_strContent = ":/images/virtual.keyboard/" + strHeight + "/" + strContentName + ".png";
    m_btnContent = QImage(m_strContent);
}

KeyboardButton::KeyboardButton(KeyboardPalette *parent, QString contentImagePath = "")
    : QWidget(parent), d(new Private) {
    d->keyboard = parent;
    d->bFocused = false;
    d->bPressed = false;

    d->m_parent = parent;

    d->m_contentImagePath = contentImagePath;
    d->imgContent = NULL;

    setCursor(Qt::PointingHandCursor);
}

KeyboardButton::~KeyboardButton() {
    if (d->imgContent != NULL) {
        delete d->imgContent;
        d->imgContent = NULL;
    }
}

bool KeyboardButton::isPressed() {
    return d->bPressed;
}

KeyboardPalette *KeyboardButton::keyboardPalette() {
    return d->keyboard;
}

ContentImage *KeyboardButton::contentImage() const {
    return d->imgContent;
}

void KeyboardButton::paintEvent(QPaintEvent *) {
    QPainter painter(this);

    // 键帽尺寸变化后，内容图标要按新尺寸目录重新加载。
    if (d->imgContent != NULL) {
        if (d->imgContent->m_height != d->m_parent->d->currBtnImages->m_height) {
            delete d->imgContent;
            if (!d->m_contentImagePath.isEmpty())
                d->imgContent =
                    new ContentImage(d->m_parent->d->currBtnImages->m_strHeight,
                                     d->m_parent->d->currBtnImages->m_height, d->m_contentImagePath);
        }
    } else if (!d->m_contentImagePath.isEmpty())
        d->imgContent = new ContentImage(d->m_parent->d->currBtnImages->m_strHeight,
                                         d->m_parent->d->currBtnImages->m_height, d->m_contentImagePath);

    // 左右端图保持原宽，中间部分拉伸填满，拼出完整键帽。
    if (isPressed()) {
        painter.drawImage(0, 0, d->m_parent->d->currBtnImages->m_btnLeftActive, 0, 0,
                          d->m_parent->d->currBtnImages->m_btnLeftActive.width(),
                          d->m_parent->d->currBtnImages->m_btnLeftActive.height());
        painter.drawImage(QRect(d->m_parent->d->currBtnImages->m_btnLeftActive.width(), 0,
                                width() - d->m_parent->d->currBtnImages->m_btnLeftActive.width() -
                                    d->m_parent->d->currBtnImages->m_btnRightActive.width(),
                                height()),
                          d->m_parent->d->currBtnImages->m_btnCenterActive);
        painter.drawImage(width() - d->m_parent->d->currBtnImages->m_btnRightActive.width(), 0,
                          d->m_parent->d->currBtnImages->m_btnRightActive, 0, 0,
                          d->m_parent->d->currBtnImages->m_btnRightActive.width(),
                          d->m_parent->d->currBtnImages->m_btnRightActive.height());
    } else {
        painter.drawImage(0, 0, d->m_parent->d->currBtnImages->m_btnLeftPassive, 0, 0,
                          d->m_parent->d->currBtnImages->m_btnLeftPassive.width(),
                          d->m_parent->d->currBtnImages->m_btnLeftPassive.height());
        painter.drawImage(QRect(d->m_parent->d->currBtnImages->m_btnLeftPassive.width(), 0,
                                width() - d->m_parent->d->currBtnImages->m_btnLeftPassive.width() -
                                    d->m_parent->d->currBtnImages->m_btnRightPassive.width(),
                                height()),
                          d->m_parent->d->currBtnImages->m_btnCenterPassive);
        painter.drawImage(width() - d->m_parent->d->currBtnImages->m_btnRightPassive.width(), 0,
                          d->m_parent->d->currBtnImages->m_btnRightPassive, 0, 0,
                          d->m_parent->d->currBtnImages->m_btnRightPassive.width(),
                          d->m_parent->d->currBtnImages->m_btnRightPassive.height());
    }

    // 最后叠加键面文字或图标。
    this->paintContent(painter);
}

void KeyboardButton::enterEvent(QEvent *) {
    d->bFocused = true;
    update();
}

void KeyboardButton::leaveEvent(QEvent *) {
    d->bFocused = false;
    update();
}

void KeyboardButton::mousePressEvent(QMouseEvent *event) {
    event->accept();
    d->bPressed = true;
    update();
    this->onPress();
}

void KeyboardButton::mouseReleaseEvent(QMouseEvent *) {
    d->bPressed = false;
    update();
    this->onRelease();
}

KeyButton::KeyButton(KeyboardPalette *parent) : KeyboardButton(parent), d(new Private) {
    d->keybt = 0;
}

KeyButton::~KeyButton() {}

bool KeyButton::shifted() {
    bool b = keyboardPalette()->d->shift;
    if (d->keybt->capsLockSwitch && keyboardPalette()->d->capsLock)
        b = !b;
    return b;
}

void KeyButton::onPress() {
    if (d->keybt != NULL) {
        // 普通字符用 0/1 区分下档/上档；带特殊修饰符的键按修饰符序号取更高一组。
        int codeIndex = keyboardPalette()->d->nSpecialModifierIndex * 2 + shifted();

        if (keyboardPalette()->d->nSpecialModifierIndex) {
            // 上一个键留下了修饰符：先发修饰符本身，再发当前字符。
            if (d->keybt->codes[codeIndex].empty()) {
                KeyboardSender::sendUnicodeKey(keyboardPalette()->d->specialModifier);
                KeyboardSender::sendUnicodeKey(d->keybt->codes[shifted()]);
            } else {
                KeyboardSender::sendUnicodeKey(d->keybt->codes[codeIndex]);
            }

            keyboardPalette()->d->nSpecialModifierIndex = 0;
        } else {
            // 当前键若声明了修饰符，先记住它，等下一个键一起发送。
            int nSpecialModifierIndex = shifted() ? d->keybt->modifier2 : d->keybt->modifier1;

            if (nSpecialModifierIndex) {
                keyboardPalette()->d->nSpecialModifierIndex = nSpecialModifierIndex;
                keyboardPalette()->d->specialModifier = d->keybt->codes[codeIndex];
            } else {
                KeyboardSender::sendUnicodeKey(d->keybt->codes[codeIndex]);
            }
        }
    }

    // Shift 是一次性上档，输入一个字符后自动释放。
    if (keyboardPalette()->d->shift) {
        keyboardPalette()->d->shift = false;
        keyboardPalette()->update();
    }
}

void KeyButton::onRelease() {}

void KeyButton::setKeyBt(const KEYBT *keybt) {
    d->keybt = keybt;
}

void KeyButton::paintContent(QPainter &painter) {
    if (d->keybt) {
        QString text(QChar(shifted() ? d->keybt->symbol2 : d->keybt->symbol1));
        QRect textRect(rect().x() + 2, rect().y() + 2, rect().width() - 4, rect().height() - 4);
        painter.drawText(textRect, Qt::AlignCenter, text);
    }
}

CntrlButton::CntrlButton(KeyboardPalette *parent, int _code, const QString &_contentImagePath)
    : KeyboardButton(parent, _contentImagePath), d(new Private) {
    d->label = "";
    d->code = _code;
}


CntrlButton::CntrlButton(KeyboardPalette *parent, const QString &_label, int _code)
    : KeyboardButton(parent), d(new Private) {
    d->label = _label;
    d->code = _code;
}

CntrlButton::~CntrlButton() {}

void CntrlButton::onPress() {
    KeyboardSender::sendVirtualKey(d->code);
}

void CntrlButton::onRelease() {}

void CntrlButton::paintContent(QPainter &painter) {
    if (!d->label.isEmpty()) {
        painter.drawText(rect(), Qt::AlignCenter, d->label);
    } else if (contentImage() != NULL) {
        painter.drawImage((rect().width() - contentImage()->m_btnContent.width()) / 2,
                          (rect().height() - contentImage()->m_btnContent.height()) / 2,
                          contentImage()->m_btnContent, 0, 0, contentImage()->m_btnContent.width(),
                          contentImage()->m_btnContent.height());
    }
}

CapsLockButton::CapsLockButton(KeyboardPalette *parent, const QString _contentImagePath)
    : KeyboardButton(parent, _contentImagePath) {}

CapsLockButton::~CapsLockButton() {}

void CapsLockButton::onPress() {
    keyboardPalette()->d->capsLock = !keyboardPalette()->d->capsLock;
    keyboardPalette()->update();
}

void CapsLockButton::onRelease() {}

bool CapsLockButton::isPressed() {
    return keyboardPalette()->d->capsLock;
}

void CapsLockButton::paintContent(QPainter &painter) {
    if (contentImage() != NULL) {
        painter.drawImage((rect().width() - contentImage()->m_btnContent.width()) / 2,
                          (rect().height() - contentImage()->m_btnContent.height()) / 2,
                          contentImage()->m_btnContent, 0, 0, contentImage()->m_btnContent.width(),
                          contentImage()->m_btnContent.height());
    } else
        painter.drawText(rect(), Qt::AlignCenter, "^");
}

ShiftButton::ShiftButton(KeyboardPalette *parent, const QString _contentImagePath)
    : KeyboardButton(parent, _contentImagePath) {}

ShiftButton::~ShiftButton() {}

void ShiftButton::onPress() {
    keyboardPalette()->d->shift = !keyboardPalette()->d->shift;
    keyboardPalette()->update();
}


void ShiftButton::onRelease() {}

bool ShiftButton::isPressed() {
    return keyboardPalette()->d->shift;
}

void ShiftButton::paintContent(QPainter &painter) {
    if (contentImage() != NULL) {
        painter.drawImage((rect().width() - contentImage()->m_btnContent.width()) / 2,
                          (rect().height() - contentImage()->m_btnContent.height()) / 2,
                          contentImage()->m_btnContent, 0, 0, contentImage()->m_btnContent.width(),
                          contentImage()->m_btnContent.height());
    } else
        painter.drawText(rect(), Qt::AlignCenter, "^");
}


LocaleButton::LocaleButton(KeyboardPalette *parent) : KeyboardButton(parent), d(new Private) {
    d->localeMenu = new QMenu(this);

    for (int i = 0; i < parent->d->nLocalesCount; i++) {
        QAction *action =
            (parent->d->locales[i]->icon != NULL)
                ? d->localeMenu->addAction(*parent->d->locales[i]->icon, parent->d->locales[i]->fullName)
                : d->localeMenu->addAction(parent->d->locales[i]->fullName);
        action->setData(QVariant(i));
    }
}

LocaleButton::~LocaleButton() {
    delete d->localeMenu;
}

void LocaleButton::onPress() {}

void LocaleButton::onRelease() {
    // 菜单以阻塞方式弹出；期间标记语言菜单激活，避免 leaveEvent 把键盘隐藏掉。
    keyboardPalette()->d->languagePopupActive = true;
    QAction *action = d->localeMenu->exec(mapToGlobal(QPoint(0, 0)));
    keyboardPalette()->d->languagePopupActive = false;
    if (action != NULL) {
        int nLocale = action->data().toInt();
        keyboardPalette()->setLocale(nLocale);
    }
}

void LocaleButton::paintContent(QPainter &painter) {
    const QString *localeName = keyboardPalette()->getLocaleName();
    if (localeName != NULL)
        painter.drawText(rect(), Qt::AlignCenter, *localeName);
}
