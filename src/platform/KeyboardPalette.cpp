#include <QtWidgets>
#include <QList>
#include <QSize>

#include "platform/KeyboardPalette.h"
#include "settings/Settings.h"

#include "core/Application.h"
#include "ui/MainWindow.h"


/*

            KeyboardPalette

*/

struct KeyboardPalette::Private
{
    BTNImages* currBtnImages{nullptr};

    bool m_isVisible{false};
    QPoint m_pos;

    bool capsLock{false};
    bool shift{false};
    int nCurrentLocale{0};
    int nLocalesCount{0};
    KeyboardLocale** locales{nullptr};

    int nSpecialModifierIndex{0};
    KEYCODE specialModifier{0};

    QString strSize;
    int btnWidth{0};
    int btnHeight{0};

    bool languagePopupActive{false};
    bool keyboardActive{false};

    QRect originalRect;

    KeyButton** buttons{nullptr};
    KeyboardButton** ctrlButtons{nullptr};

    // Storage for platform-dependent objects (linux)
    void* storage{nullptr};
    // Linux-related parameters
    int min_keycodes{0};
    int max_keycodes{0};
    int byte_per_code{0};
};

struct KeyboardButton::Private
{
    KeyboardPalette* m_parent{nullptr};
    ContentImage* imgContent{nullptr};
    QString m_contentImagePath;
    KeyboardPalette* keyboard{nullptr};
    bool bFocused{false};
    bool bPressed{false};
};

struct KeyButton::Private
{
    const KEYBT* keybt{nullptr};
};

struct CntrlButton::Private
{
    QString label;
    int code{0};
};

struct LocaleButton::Private
{
    QMenu* localeMenu{nullptr};
};

KeyboardPalette::KeyboardPalette(QWidget *parent)
        : ActionPalette(Qt::TopRightCorner, parent)
        , d(new Private)
{

  //  setWindowFlags(/*Qt::CustomizeWindowHint|*/Qt::WindowStaysOnTopHint|Qt::FramelessWindowHint);

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

    d->buttons = new KeyButton*[47];
    for (int i=0; i<47; i++)
    {
        d->buttons[i] = new KeyButton(this);
    }

    d->locales = PlatformUtils::getKeyboardLayouts(d->nLocalesCount);

    createCtrlButtons();

    d->nCurrentLocale = Settings::settings()->KeyboardLocale->get().toInt();
    if (d->nCurrentLocale < 0 || d->nCurrentLocale >= d->nLocalesCount)
        d->nCurrentLocale = 0;
    if (d->locales!=NULL)
        setInput(d->locales[d->nCurrentLocale]);

    setContentsMargins( 22, 22, 22, 22 );

    init();
}

KeyboardButton** KeyboardPalette::createCtrlButtonArray(int count)
{
    d->ctrlButtons = new KeyboardButton*[count];
    return d->ctrlButtons;
}

//QList<KeyboardPalette*> KeyboardPalette::instances;
void KeyboardPalette::init()
{
    d->m_isVisible = false;
    setVisible(false);

    setKeyButtonSize(Settings::settings()->boardKeyboardPaletteKeyBtnSize->get().toString());

    connect(this, &KeyboardPalette::keyboardActivated, this, &KeyboardPalette::onActivated);
    connect(Settings::settings()->boardKeyboardPaletteKeyBtnSize, &Setting::changed, this, &KeyboardPalette::keyboardPaletteButtonSizeChanged);
    connect(Application::mainWindow->ui().actionVirtualKeyboard, &QAction::triggered, this, &KeyboardPalette::showKeyboard);
    connect(this, &ActionPalette::closed, this, &KeyboardPalette::hideKeyboard);

    //------------------------------//

    PlatformUtils::setWindowNonActivableFlag(this, true);
}

void KeyboardPalette::showKeyboard(bool show)
{
    d->m_isVisible = show;
}

bool KeyboardPalette::keyboardVisible() const
{
    return d->m_isVisible;
}


void KeyboardPalette::hideKeyboard()
{
    Application::mainWindow->ui().actionVirtualKeyboard->activate(QAction::Trigger);
}

void KeyboardPalette::syncPosition(const QPoint & pos)
{
    d->m_pos = pos;
    move(pos);
}

void KeyboardPalette::syncLocale(int nLocale)
{
    d->nCurrentLocale = nLocale;
    setInput(d->locales[d->nCurrentLocale]);
}

void KeyboardPalette::keyboardPaletteButtonSizeChanged(QVariant size)
{
    setKeyButtonSize(size.toString());
}

void KeyboardPalette::setInput(const KeyboardLocale* locale)
{
    if (locale!=NULL)
    {
        for (int i=0; i<47; i++)
            d->buttons[i]->setKeyBt((*locale)[i]);
    }
    else
    {
        this->hide();
    }
}

KeyboardPalette::~KeyboardPalette()
{
    //for (int i=0; i<47; i++)
    //    delete buttons[i];
    delete [] d->buttons;

    //for (int i=0; i<8; i++)
    //    delete ctrlButtons[i];
    delete [] d->ctrlButtons;

    //if (locales!=NULL)
    //{
    //    for (int i=0; i<nLocalesCount; i++)
    //        delete locales[i];
    //    delete [] locales;
    //}

    if(d->currBtnImages != NULL)
    {
        delete d->currBtnImages;
        d->currBtnImages = NULL;
    }

    onActivated(false);
}

bool KeyboardPalette::isEnabled()
{
    return d->locales!= NULL;
}

QSize  KeyboardPalette::sizeHint () const
{
    int w = contentsMargins().left() + 15 * d->btnWidth + contentsMargins().right();
    int h = contentsMargins().top() + 5 * d->btnHeight + contentsMargins().bottom();
    return QSize(w, h);
}

const QString* KeyboardPalette::getLocaleName()
{
    return d->locales == NULL ? NULL : &(d->locales[d->nCurrentLocale]->name);
}

void KeyboardPalette::setLocale(int nLocale)
{
    if (d->locales != NULL)
    {
        d->nCurrentLocale = nLocale;

        setInput(d->locales[d->nCurrentLocale]);
        onLocaleChanged(d->locales[d->nCurrentLocale]);
        update();

        Settings::settings()->KeyboardLocale->set(d->nCurrentLocale);
    }
    emit localeChanged(nLocale);
}

QString KeyboardPalette::getKeyButtonSize() const
{
    return QString("%1x%2").arg(d->btnWidth).arg(d->btnHeight);
}

void KeyboardPalette::setKeyButtonSize(const QString& _strSize)
{
    QStringList strs = _strSize.split('x');

    if (strs.size()==2)
    {
        d->strSize = _strSize;
        d->btnWidth = strs[0].toInt();
        d->btnHeight = strs[1].toInt();

        if(d->currBtnImages != NULL)
            delete d->currBtnImages;
        d->currBtnImages = new BTNImages(strs[1], d->btnWidth, d->btnHeight);

        adjustSizeAndPosition();
    }
}

void KeyboardPalette::enterEvent ( QEvent * )
{
    if (d->keyboardActive)
        return;

    d->keyboardActive = true;

    adjustSizeAndPosition();

    emit keyboardActivated(true);
}

void KeyboardPalette::leaveEvent ( QEvent * )
{
    if (d->languagePopupActive || !d->keyboardActive || isMoving())
        return;

    d->keyboardActive = false;

    adjustSizeAndPosition();

    emit keyboardActivated(false);
}

void  KeyboardPalette::moveEvent ( QMoveEvent * event )
{
    ActionPalette::moveEvent(event);
    emit moved(event->pos());
}

void KeyboardPalette::adjustSizeAndPosition(bool pUp)
{
    QSize rSize = sizeHint();
    if (rSize != size())
    {
        int dx = (rSize.width() - size().width()) /2;
        int dy = rSize.height() - size().height();

        this->move(x()-dx, y() - dy);
        this->resize(rSize.width(), rSize.height());
    }
    ActionPalette::adjustSizeAndPosition(pUp);
}

void  KeyboardPalette::paintEvent( QPaintEvent* event)
{
    checkLayout();

    ActionPalette::paintEvent(event);

    QRect r = this->geometry();

    const QMargins paletteMargins = contentsMargins();
    int lleft = paletteMargins.left(), ltop = paletteMargins.top(),
        lright = paletteMargins.right(), lbottom = paletteMargins.bottom();

    //------------------------------------------------
    // calculate start offset from left, and from top

    int ctrlButtonsId = 0;
    lleft = ( r.width() - d->btnWidth * 15 ) / 2;
    ltop = ( r.height() - d->btnHeight * 5 ) / 2;

    //------------------------------------------------
    // set geometry (position) for row 1

    int offX = lleft;
    int offY = ltop;

    //-------------------

    // buttons [`]..[+]
    for (int i = 0; i<13; i++)
    {
        d->buttons[i]->setGeometry(offX, offY, d->btnWidth, d->btnHeight);
        offX += d->btnWidth;
    }

    // button Backspace
    d->ctrlButtons[ctrlButtonsId++]->setGeometry(offX, offY, d->btnWidth * 2, d->btnHeight);
    offX += d->btnWidth * 2;

    //------------------------------------------------
    // set geometry (position) for row 2

    offX = lleft;
    offY += d->btnHeight;
    offX += d->btnWidth / 2;

    //-------------------

    // button Tab
    d->ctrlButtons[ctrlButtonsId++]->setGeometry(offX, offY, d->btnWidth * 2, d->btnHeight);
    offX += d->btnWidth * 2;

    // buttons [q]..[]]
    for (int i = 0; i<12; i++)
    {
        d->buttons[i + 13]->setGeometry(offX, offY, d->btnWidth, d->btnHeight);
        offX += d->btnWidth;
    }

//     // Row 2 Stub
//     ctrlButtons[ctrlButtonsId++]->setGeometry(offX, offY, btnWidth * 1.5, btnHeight);
//     offX += btnWidth * 1.5;

    //------------------------------------------------
    // set geometry (position) for row 3

    offX = lleft;
    offY += d->btnHeight;

    //-------------------

//     // Row 3 Stub

    // button Enter
    d->ctrlButtons[ctrlButtonsId++]->setGeometry(offX, offY, d->btnWidth * 1, d->btnHeight);
    offX += d->btnWidth*1;

    // buttons [a]..[\]
    for (int i = 0; i < 12; i++)
    {
        d->buttons[i + 12 + 13]->setGeometry(offX, offY, d->btnWidth, d->btnHeight);
        offX += d->btnWidth;
    }

    // button Enter
    d->ctrlButtons[ctrlButtonsId++]->setGeometry(offX, offY, d->btnWidth * 2, d->btnHeight);
    offX += d->btnWidth*2;

    //------------------------------------------------
    // set geometry (position) for row 4

    offX = lleft;
    offY += d->btnHeight;

    //-------------------

    // button LCapsLock
    d->ctrlButtons[ctrlButtonsId++]->setGeometry(offX, offY, d->btnWidth*2.5, d->btnHeight);
    offX += d->btnWidth*2.5;

    for (int i = 0; i < 10; i++)
    {
        d->buttons[i + 12 + 12 + 13]->setGeometry(offX, offY, d->btnWidth, d->btnHeight);
        offX += d->btnWidth;
    }

    // button RCapsLock
    d->ctrlButtons[ctrlButtonsId++]->setGeometry(offX, offY, d->btnWidth*2.5, d->btnHeight);
    offX += d->btnWidth*2.5;

    //------------------------------------------------
    // set geometry (position) for row 5

    offX = lleft;
    offY += d->btnHeight;

    //-------------------

    d->ctrlButtons[ctrlButtonsId++]->setGeometry(offX + d->btnWidth * 1 , offY, d->btnWidth * 2, d->btnHeight);
    d->ctrlButtons[ctrlButtonsId++]->setGeometry(offX + d->btnWidth * 3 , offY, d->btnWidth * 9, d->btnHeight);
    d->ctrlButtons[ctrlButtonsId++]->setGeometry(offX + d->btnWidth * 12, offY, d->btnWidth * 2, d->btnHeight);

    //------------------------------------------------
}

void  KeyboardPalette::onDeactivated()
{
    onActivated(false);
}


//-----------------------------------------------------------------------//
// BTNImages Class
//-----------------------------------------------------------------------//

BTNImages::BTNImages(QString strHeight, int width, int height)
{
    m_strHeight = strHeight;
    m_width = width;
    m_height = height;

    m_strLeftPassive    = ":/images/virtual.keyboard/" + strHeight + "/left-passive.png";
    m_strCenterPassive  = ":/images/virtual.keyboard/" + strHeight + "/centre-passive.png";
    m_strRightPassive   = ":/images/virtual.keyboard/" + strHeight + "/right-passive.png";
    m_strLeftActive     = ":/images/virtual.keyboard/" + strHeight + "/left-active.png";
    m_strCenterActive   = ":/images/virtual.keyboard/" + strHeight + "/centre-active.png";
    m_strRightActive    = ":/images/virtual.keyboard/" + strHeight + "/right-active.png";

    m_btnLeftPassive    = QImage(m_strLeftPassive);
    m_btnCenterPassive  = QImage(m_strCenterPassive);
    m_btnRightPassive   = QImage(m_strRightPassive);
    m_btnLeftActive     = QImage(m_strLeftActive);
    m_btnCenterActive   = QImage(m_strCenterActive);
    m_btnRightActive    = QImage(m_strRightActive);
}

ContentImage::ContentImage(QString strHeight, int height, QString strContentName)
{
    m_strHeight = strHeight;
    m_height = height;

    m_strContent = ":/images/virtual.keyboard/" + strHeight + "/" + strContentName + ".png";
    m_btnContent = QImage(m_strContent);
}

//-----------------------------------------------------------------------//
// KeyboardButton Class
//-----------------------------------------------------------------------//

KeyboardButton::KeyboardButton(KeyboardPalette* parent, QString contentImagePath = "")
        :QWidget(parent),
        d(new Private)
{
    d->keyboard = parent;
    d->bFocused = false;
    d->bPressed = false;

    d->m_parent = parent;

    d->m_contentImagePath = contentImagePath;
    d->imgContent = NULL;

    setCursor(Qt::PointingHandCursor);
}

KeyboardButton::~KeyboardButton()
{
    if(d->imgContent != NULL)
    {
        delete d->imgContent;
        d->imgContent = NULL;
    }
}

bool KeyboardButton::isPressed()
{
    return d->bPressed;
}

KeyboardPalette* KeyboardButton::keyboardPalette()
{
    return d->keyboard;
}

ContentImage* KeyboardButton::contentImage() const
{
    return d->imgContent;
}

void KeyboardButton::paintEvent(QPaintEvent*)
{

    QPainter painter(this);

    //--------------------------

    if(d->imgContent != NULL)
    {
        if(d->imgContent->m_height != d->m_parent->d->currBtnImages->m_height)
        {
            delete d->imgContent;
            if(!d->m_contentImagePath.isEmpty())
                d->imgContent = new ContentImage(d->m_parent->d->currBtnImages->m_strHeight, d->m_parent->d->currBtnImages->m_height, d->m_contentImagePath);
        }
    }
    else
    if(!d->m_contentImagePath.isEmpty())
        d->imgContent = new ContentImage(d->m_parent->d->currBtnImages->m_strHeight, d->m_parent->d->currBtnImages->m_height, d->m_contentImagePath);

    //--------------------------

    if (isPressed())
    {
        painter.drawImage( 0,0, d->m_parent->d->currBtnImages->m_btnLeftActive, 0,0, d->m_parent->d->currBtnImages->m_btnLeftActive.width(), d->m_parent->d->currBtnImages->m_btnLeftActive.height() );
        painter.drawImage( QRect(d->m_parent->d->currBtnImages->m_btnLeftActive.width(), 0, width() - d->m_parent->d->currBtnImages->m_btnLeftActive.width() - d->m_parent->d->currBtnImages->m_btnRightActive.width(), height()), d->m_parent->d->currBtnImages->m_btnCenterActive );
        painter.drawImage( width() - d->m_parent->d->currBtnImages->m_btnRightActive.width(), 0, d->m_parent->d->currBtnImages->m_btnRightActive, 0,0, d->m_parent->d->currBtnImages->m_btnRightActive.width(), d->m_parent->d->currBtnImages->m_btnRightActive.height() );
    }
    else
    {
        painter.drawImage( 0,0, d->m_parent->d->currBtnImages->m_btnLeftPassive, 0,0, d->m_parent->d->currBtnImages->m_btnLeftPassive.width(), d->m_parent->d->currBtnImages->m_btnLeftPassive.height() );
        painter.drawImage( QRect(d->m_parent->d->currBtnImages->m_btnLeftPassive.width(), 0, width() - d->m_parent->d->currBtnImages->m_btnLeftPassive.width() - d->m_parent->d->currBtnImages->m_btnRightPassive.width(), height()), d->m_parent->d->currBtnImages->m_btnCenterPassive );
        painter.drawImage( width() - d->m_parent->d->currBtnImages->m_btnRightPassive.width(), 0, d->m_parent->d->currBtnImages->m_btnRightPassive, 0,0, d->m_parent->d->currBtnImages->m_btnRightPassive.width(), d->m_parent->d->currBtnImages->m_btnRightPassive.height() );
    }

    //--------------------------

    this->paintContent(painter);

    //--------------------------
}

void  KeyboardButton::enterEvent ( QEvent*)
{
    d->bFocused = true;
    update();
}

void  KeyboardButton::leaveEvent ( QEvent*)
{
    d->bFocused = false;
    update();
}

void  KeyboardButton::mousePressEvent ( QMouseEvent * event)
{
    event->accept(); 
    d->bPressed = true;
    update();
    this->onPress();
}

void  KeyboardButton::mouseReleaseEvent ( QMouseEvent * )
{
    d->bPressed = false;
    update();
    this->onRelease();
}

KeyButton::KeyButton(KeyboardPalette* parent)
        :KeyboardButton(parent)
        , d(new Private)
{
    d->keybt = 0;
}

KeyButton::~KeyButton()
{}

bool KeyButton::shifted()
{
    bool b = keyboardPalette()->d->shift;
    if (d->keybt->capsLockSwitch && keyboardPalette()->d->capsLock)
        b = !b;
    return b;
}

void KeyButton::onPress()
{
    if (d->keybt!=NULL)
    {
        int codeIndex = keyboardPalette()->d->nSpecialModifierIndex * 2 + shifted();

        if (keyboardPalette()->d->nSpecialModifierIndex)
        {
            if (d->keybt->codes[codeIndex].empty())
            {
                sendUnicodeSymbol(keyboardPalette()->d->specialModifier);
                sendUnicodeSymbol(d->keybt->codes[shifted()]);
            }
            else
            {
                sendUnicodeSymbol(d->keybt->codes[codeIndex]);
            }

            keyboardPalette()->d->nSpecialModifierIndex = 0;
        }
        else
        {
            int nSpecialModifierIndex = shifted()? d->keybt->modifier2 : d->keybt->modifier1;

            if (nSpecialModifierIndex)
            {
                keyboardPalette()->d->nSpecialModifierIndex = nSpecialModifierIndex;
                keyboardPalette()->d->specialModifier = d->keybt->codes[codeIndex];
            }
            else
            {
                sendUnicodeSymbol(d->keybt->codes[codeIndex]);            
            }
        }
    }

    if (keyboardPalette()->d->shift)
    {
        keyboardPalette()->d->shift = false;
        keyboardPalette()->update();
    }
}

void KeyButton::onRelease()
{}

void KeyButton::setKeyBt(const KEYBT* keybt)
{
    d->keybt = keybt;
}

void KeyButton::paintContent(QPainter& painter)
{
    if (d->keybt)
    {
        QString text(QChar(shifted() ? d->keybt->symbol2 : d->keybt->symbol1));
        QRect textRect(rect().x()+2, rect().y()+2, rect().width()-4, rect().height()-4);
        painter.drawText(textRect, Qt::AlignCenter, text);
    }
}

CntrlButton::CntrlButton(KeyboardPalette* parent, int _code, const QString& _contentImagePath )
        :KeyboardButton(parent, _contentImagePath)
        , d(new Private)
{
    d->label = "";
    d->code = _code;
}


CntrlButton::CntrlButton(KeyboardPalette* parent, const QString& _label, int _code )
        :KeyboardButton(parent)
        , d(new Private)
{
    d->label = _label;
    d->code = _code;
}

CntrlButton::~CntrlButton()
{}

void CntrlButton::onPress()
{
     sendControlSymbol(d->code);
}

void CntrlButton::onRelease()
{}

void CntrlButton::paintContent(QPainter& painter)
{
    if(!d->label.isEmpty())
    {
        painter.drawText(rect(), Qt::AlignCenter, d->label);
    }
    else
    if(contentImage() != NULL)
    {
        painter.drawImage(( rect().width() - contentImage()->m_btnContent.width() ) / 2, ( rect().height() - contentImage()->m_btnContent.height() ) / 2,
            contentImage()->m_btnContent, 0,0, contentImage()->m_btnContent.width(), contentImage()->m_btnContent.height());
    }
}

CapsLockButton::CapsLockButton(KeyboardPalette* parent, const QString _contentImagePath)
        :KeyboardButton(parent, _contentImagePath)
{}

CapsLockButton::~CapsLockButton()
{}

void CapsLockButton::onPress()
{
    keyboardPalette()->d->capsLock = !keyboardPalette()->d->capsLock;
    keyboardPalette()->update();
}

void CapsLockButton::onRelease()
{}

bool CapsLockButton::isPressed()
{
    return keyboardPalette()->d->capsLock;
}

void CapsLockButton::paintContent(QPainter& painter)
{
    if(contentImage() != NULL)
    {
        painter.drawImage(( rect().width() - contentImage()->m_btnContent.width() ) / 2, ( rect().height() - contentImage()->m_btnContent.height() ) / 2,
            contentImage()->m_btnContent, 0,0, contentImage()->m_btnContent.width(), contentImage()->m_btnContent.height());
    }
    else
        painter.drawText(rect(), Qt::AlignCenter, "^");
}

ShiftButton::ShiftButton(KeyboardPalette* parent, const QString _contentImagePath)
    :KeyboardButton(parent, _contentImagePath)
{}

ShiftButton::~ShiftButton()
{}

void ShiftButton::onPress()
{
    keyboardPalette()->d->shift = !keyboardPalette()->d->shift;
    keyboardPalette()->update();
}


void ShiftButton::onRelease()
{}

bool ShiftButton::isPressed()
{
    return keyboardPalette()->d->shift;
}

void ShiftButton::paintContent(QPainter& painter)
{
    if(contentImage() != NULL)
    {
        painter.drawImage(( rect().width() - contentImage()->m_btnContent.width() ) / 2, ( rect().height() - contentImage()->m_btnContent.height() ) / 2,
            contentImage()->m_btnContent, 0,0, contentImage()->m_btnContent.width(), contentImage()->m_btnContent.height());
    }
    else
        painter.drawText(rect(), Qt::AlignCenter, "^");
}



LocaleButton::LocaleButton(KeyboardPalette* parent)
        :KeyboardButton(parent)
        , d(new Private)
{
    d->localeMenu = new QMenu(this);

    for (int i=0; i<parent->d->nLocalesCount; i++)
    {
        QAction* action = (parent->d->locales[i]->icon!=NULL) ?
                          d->localeMenu->addAction(*parent->d->locales[i]->icon, parent->d->locales[i]->fullName)
                          : d->localeMenu->addAction(parent->d->locales[i]->fullName);
        action->setData(QVariant(i));
    }
}

LocaleButton::~LocaleButton()
{
    delete d->localeMenu;
}

void LocaleButton::onPress()
{
}

void LocaleButton::onRelease()
{
    keyboardPalette()->d->languagePopupActive = true;
    QAction* action = d->localeMenu->exec(mapToGlobal(QPoint(0,0)));
    keyboardPalette()->d->languagePopupActive = false;
    if (action!=NULL)
    {
        int nLocale = action->data().toInt();
        keyboardPalette()->setLocale(nLocale);
    }
}

void LocaleButton::paintContent(QPainter& painter)
{
    const QString* localeName = keyboardPalette()->getLocaleName();
    if (localeName!=NULL)
        painter.drawText(rect(), Qt::AlignCenter, *localeName);
}
