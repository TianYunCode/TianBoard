#include "web/SearchLineEdit.h"

#include <QtWidgets>
#include <QMenu>


ClearButton::ClearButton(QWidget *parent) : QAbstractButton(parent) {
    setCursor(Qt::ArrowCursor);
    setToolTip(tr("Clear"));
    setVisible(false);
    setFocusPolicy(Qt::NoFocus);
}

void ClearButton::paintEvent(QPaintEvent *event) {
    Q_UNUSED(event);
    QPainter painter(this);
    int height = this->height();

    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setBrush(isDown() ? palette().color(QPalette::Dark) : palette().color(QPalette::Mid));
    painter.setPen(painter.brush().color());
    int size = width();
    int offset = size / 5;
    int radius = size - offset * 2;
    painter.drawEllipse(offset, offset, radius, radius);

    painter.setPen(palette().color(QPalette::Base));
    int border = offset * 2;
    painter.drawLine(border, border, width() - border, height - border);
    painter.drawLine(border, height - border, width() - border, border);
}

void ClearButton::textChanged(const QString &text) {
    setVisible(!text.isEmpty());
}

/*
    搜索框左侧的搜索按钮：点击图标可弹出菜单（若设置了菜单）。
    这里只在本文件内部使用，随 SearchLineEdit 一起创建。
 */
class SearchButton : public QAbstractButton {
public:
    SearchButton(QWidget *parent = 0);
    void paintEvent(QPaintEvent *event);
    QMenu *m_menu;

protected:
    void mousePressEvent(QMouseEvent *event);
};

SearchButton::SearchButton(QWidget *parent) : QAbstractButton(parent), m_menu(0) {
    setObjectName(QLatin1String("SearchButton"));
    setCursor(Qt::ArrowCursor);
    setFocusPolicy(Qt::NoFocus);
}

void SearchButton::mousePressEvent(QMouseEvent *event) {
    if (m_menu && event->button() == Qt::LeftButton) {
        QWidget *p = parentWidget();
        if (p) {
            QPoint r = p->mapToGlobal(QPoint(0, p->height()));
            m_menu->exec(QPoint(r.x() + height() / 2, r.y()));
        }
        event->accept();
    }
    QAbstractButton::mousePressEvent(event);
}

void SearchButton::paintEvent(QPaintEvent *event) {
    Q_UNUSED(event);
    QPainterPath myPath;

    int radius = (height() / 5) * 2;
    QRect circle(height() / 3 - 1, height() / 4, radius, radius);
    myPath.addEllipse(circle);

    myPath.arcMoveTo(circle, 300);
    QPointF c = myPath.currentPosition();
    int diff = height() / 7;
    myPath.lineTo(qMin(width() - 2, (int)c.x() + diff), c.y() + diff);

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setPen(QPen(Qt::darkGray, 2));
    painter.drawPath(myPath);

    if (m_menu) {
        QPainterPath dropPath;
        dropPath.arcMoveTo(circle, 320);
        QPointF c = dropPath.currentPosition();
        c = QPointF(c.x() + 3.5, c.y() + 0.5);
        dropPath.moveTo(c);
        dropPath.lineTo(c.x() + 4, c.y());
        dropPath.lineTo(c.x() + 2, c.y() + 2);
        dropPath.closeSubpath();
        painter.setPen(Qt::darkGray);
        painter.setBrush(Qt::darkGray);
        painter.setRenderHint(QPainter::Antialiasing, false);
        painter.drawPath(dropPath);
    }
    painter.end();
}

/*
    SearchLineEdit 是增强版行编辑框：
    - 左侧有搜索图标，可挂下拉菜单；
    - 无文本且未获得焦点时显示“占位提示”；
    - 有文本时右侧显示清除按钮。
 */
SearchLineEdit::SearchLineEdit(QWidget *parent) : ExLineEdit(parent), mSearchButton(new SearchButton(this)) {
    connect(lineEdit(), &QLineEdit::textChanged, this, &SearchLineEdit::textChanged);
    setLeftWidget(mSearchButton);
    mInactiveText = tr("Search");

    setMinimumWidth(150);

    QSizePolicy policy = sizePolicy();
    setSizePolicy(QSizePolicy::Maximum, policy.verticalPolicy());
}

void SearchLineEdit::paintEvent(QPaintEvent *event) {
    ExLineEdit::paintEvent(event);

    if (lineEdit()->text().isEmpty() && !hasFocus() && !mInactiveText.isEmpty()) {
        QStyleOptionFrame panel;
        initStyleOption(&panel);
        QRect r = style()->subElementRect(QStyle::SE_LineEditContents, &panel, this);
        QFontMetrics fm = fontMetrics();
        int horizontalMargin = lineEdit()->x();
        QRect lineRect(horizontalMargin + r.x(), r.y() + (r.height() - fm.height() + 1) / 2,
                       r.width() - 2 * horizontalMargin, fm.height());
        QPainter painter(this);
        painter.setPen(palette().brush(QPalette::Disabled, QPalette::Text).color());
        painter.drawText(lineRect, Qt::AlignLeft | Qt::AlignVCenter, mInactiveText);
    }
}

void SearchLineEdit::resizeEvent(QResizeEvent *event) {
    updateGeometries();
    ExLineEdit::resizeEvent(event);
}

void SearchLineEdit::updateGeometries() {
    int menuHeight = height();
    int menuWidth = menuHeight + 1;
    if (!mSearchButton->m_menu)
        menuWidth = (menuHeight / 5) * 4;
    mSearchButton->resize(QSize(menuWidth, menuHeight));
}

QString SearchLineEdit::inactiveText() const {
    return mInactiveText;
}

void SearchLineEdit::setInactiveText(const QString &text) {
    mInactiveText = text;
}

void SearchLineEdit::setMenu(QMenu *menu) {
    if (mSearchButton->m_menu)
        mSearchButton->m_menu->deleteLater();
    mSearchButton->m_menu = menu;
    updateGeometries();
}

QMenu *SearchLineEdit::menu() const {
    if (!mSearchButton->m_menu) {
        mSearchButton->m_menu = new QMenu(mSearchButton);
        if (isVisible())
            (const_cast<SearchLineEdit *>(this))->updateGeometries();
    }
    return mSearchButton->m_menu;
}


void SearchLineEdit::setVisible(bool pVisible) {
    ExLineEdit::setVisible(pVisible);
    mSearchButton->setVisible(pVisible);
}
