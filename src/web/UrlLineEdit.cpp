#include "web/UrlLineEdit.h"

#include "web/SearchLineEdit.h"
#include "web/WebView.h"

#include "utils/Globals.h"

#include <QtWidgets>
#include <QCompleter>
#include <QLineEdit>
#include <QLabel>
#include <QApplication>


struct ExLineEdit::Private
{
    QWidget* mLeftWidget{nullptr};
    QLineEdit* mLineEdit{nullptr};
    ClearButton* mClearButton{nullptr};
};

struct UrlLineEdit::Private
{
    WebView* mWebView{nullptr};
};

ExLineEdit::ExLineEdit(QWidget *parent)
    : QWidget(parent)
    , d(new Private)
{
    d->mLineEdit = new QLineEdit(this);

    setFocusPolicy(d->mLineEdit->focusPolicy());
    setAttribute(Qt::WA_InputMethodEnabled);
    setSizePolicy(d->mLineEdit->sizePolicy());
    setBackgroundRole(d->mLineEdit->backgroundRole());
    setMouseTracking(true);
    setAcceptDrops(true);
    setAttribute(Qt::WA_MacShowFocusRect, true);
    QPalette p = d->mLineEdit->palette();
    setPalette(p);

    // line edit
    d->mLineEdit->setFrame(false);
    d->mLineEdit->setObjectName("webBrowserLineEdit");
    d->mLineEdit->setFocusProxy(this);
    d->mLineEdit->setAttribute(Qt::WA_MacShowFocusRect, false);
    QPalette clearPalette = d->mLineEdit->palette();
    clearPalette.setBrush(QPalette::Base, QBrush(Qt::transparent));
    d->mLineEdit->setPalette(clearPalette);

    // clearButton
    d->mClearButton = new ClearButton(this);
    connect(d->mClearButton, &QAbstractButton::clicked, d->mLineEdit,
            [this] { d->mLineEdit->clear(); });
    connect(d->mLineEdit, &QLineEdit::textChanged, d->mClearButton, &ClearButton::textChanged);

    d->mClearButton->hide();
}

ExLineEdit::~ExLineEdit() = default;

void ExLineEdit::setLeftWidget(QWidget *widget)
{
    delete d->mLeftWidget;
    d->mLeftWidget = widget;

    updateGeometries();
}

QWidget *ExLineEdit::leftWidget() const
{
    return d->mLeftWidget;
}

QLineEdit *ExLineEdit::lineEdit() const
{
    return d->mLineEdit;
}

void ExLineEdit::resizeEvent(QResizeEvent *event)
{
    Q_ASSERT(d->mLeftWidget);
    updateGeometries();
    QWidget::resizeEvent(event);
}

void ExLineEdit::updateGeometries()
{
    QStyleOptionFrame panel;
    initStyleOption(&panel);

    QRect rect = style()->subElementRect(QStyle::SE_LineEditContents, &panel, this);

    int width = rect.width();

    d->mLeftWidget->setGeometry(0, -4, 32 ,32);

    d->mLineEdit->setGeometry(32, 0,
                            width - this->height() - 32, this->height());

    d->mClearButton->setGeometry(this->width() - this->height() - 2 , 0,
            this->height(), this->height());

}

void ExLineEdit::initStyleOption(QStyleOptionFrame *option) const
{
    option->initFrom(this);
    option->rect = contentsRect();
    option->lineWidth = style()->pixelMetric(QStyle::PM_DefaultFrameWidth, option, this);
    option->midLineWidth = 0;
    option->state |= QStyle::State_Sunken;
    if (d->mLineEdit->isReadOnly())
        option->state |= QStyle::State_ReadOnly;
#ifdef QT_KEYPAD_NAVIGATION
    if (hasEditFocus())
        option->state |= QStyle::State_HasEditFocus;
#endif
    option->features = QStyleOptionFrame::None;
}

QSize ExLineEdit::sizeHint() const
{
    d->mLineEdit->setFrame(true);
    QSize size = d->mLineEdit->sizeHint();
    d->mLineEdit->setFrame(false);
    return size;
}

void ExLineEdit::focusInEvent(QFocusEvent *event)
{
    d->mLineEdit->event(event);
    QWidget::focusInEvent(event);
}

void ExLineEdit::focusOutEvent(QFocusEvent *event)
{
    d->mLineEdit->event(event);

    if (d->mLineEdit->completer()) {
        // QLineEdit::setCompleter() 内部已经把 highlighted 连到它自己的补全回写槽，
        // 这里只需要补“回车选中”那一路
        connect(d->mLineEdit->completer(), qOverload<const QString &>(&QCompleter::activated),
                         d->mLineEdit, &QLineEdit::setText);
    }
    QWidget::focusOutEvent(event);
}

void ExLineEdit::keyPressEvent(QKeyEvent *event)
{
    d->mLineEdit->event(event);
    QWidget::keyPressEvent(event);
}

bool ExLineEdit::event(QEvent *event)
{
    if (event->type() == QEvent::ShortcutOverride)
        return d->mLineEdit->event(event);

    return QWidget::event(event);
}


void ExLineEdit::paintEvent(QPaintEvent *)
{
    // No default painting
}


QVariant ExLineEdit::inputMethodQuery(Qt::InputMethodQuery property) const
{
    return d->mLineEdit->inputMethodQuery(property);
}


void ExLineEdit::inputMethodEvent(QInputMethodEvent *e)
{
    d->mLineEdit->event(e);
}


void ExLineEdit::setVisible(bool pVisible)
{
    QWidget::setVisible(pVisible);
    d->mLineEdit->setVisible(pVisible);
    d->mClearButton->setVisible(pVisible);
    d->mLeftWidget->setVisible(pVisible);
}


class UrlIconLabel : public QLabel
{

public:
    UrlIconLabel(QWidget *parent);

    WebView *m_webView;

protected:
    void mousePressEvent(QMouseEvent *event);
    void mouseMoveEvent(QMouseEvent *event);

private:
    QPoint m_dragStartPos;

};

UrlIconLabel::UrlIconLabel(QWidget *parent)
    : QLabel(parent)
    , m_webView(0)
{
    setMinimumWidth(16);
    setMinimumHeight(16);
}

void UrlIconLabel::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton)
        m_dragStartPos = event->pos();
    QLabel::mousePressEvent(event);
}

void UrlIconLabel::mouseMoveEvent(QMouseEvent *event)
{
    if (event->buttons() == Qt::LeftButton
        && (event->pos() - m_dragStartPos).manhattanLength() > QApplication::startDragDistance()
         && m_webView) {
        QDrag *drag = new QDrag(this);
        QMimeData *mimeData = new QMimeData;
        mimeData->setText(QString::fromUtf8(m_webView->url().toEncoded()));
        QList<QUrl> urls;
        urls.append(m_webView->url());
        mimeData->setUrls(urls);
        drag->setMimeData(mimeData);
        drag->exec();
    }
}

UrlLineEdit::UrlLineEdit(QWidget *parent)
    : ExLineEdit(parent)
    , d(new Private)
{
    setLeftWidget(new QWidget(this));
}

UrlLineEdit::~UrlLineEdit() = default;


void UrlLineEdit::setWebView(WebView *pWebView)
{
    d->mWebView = pWebView;

    connect(pWebView, &QWebEngineView::urlChanged,
        this, &UrlLineEdit::webViewUrlChanged);

    connect(pWebView, &QWebEngineView::loadProgress,
        this, [this] { update(); });
}


void UrlLineEdit::webViewUrlChanged(const QUrl &url)
{
    lineEdit()->setText(url.toString());
    lineEdit()->setCursorPosition(0);
}


void UrlLineEdit::focusOutEvent(QFocusEvent *event)
{
    if (lineEdit()->text().isEmpty() && d->mWebView)
        lineEdit()->setText(QString::fromUtf8(d->mWebView->url().toEncoded()));
    ExLineEdit::focusOutEvent(event);
}


