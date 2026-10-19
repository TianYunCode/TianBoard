#include <QtWidgets>
#include <QtSvg>
#include <QtSvgWidgets>

#include "core/Application.h"
#include "whiteboard/GraphicsGroupContainerItem.h"
#include "whiteboard/GraphicsTextItemDelegate.h"
#include "whiteboard/GraphicsScene.h"
#include "ui/Resources.h"

#include "whiteboard/GraphicsTextItem.h"
#include "whiteboard/GraphicsDelegateFrame.h"
#include "settings/Settings.h"

#include "whiteboard/BoardController.h"
#include "whiteboard/BoardView.h"


const int GraphicsTextItemDelegate::sMinPixelSize = 8;
const int GraphicsTextItemDelegate::sMinPointSize = 8;


AlignTextButton::AlignTextButton(const QString &fileName, QGraphicsItem *pDelegated, QGraphicsItem *parent,
                                 Qt::WindowFrameSection section)
    : DelegateButton(fileName, pDelegated, parent, section),
      lft(new QSvgRenderer(QString(":/images/leftAligned.svg"))),
      cntr(new QSvgRenderer(QString(":/images/centerAligned.svg"))),
      rght(new QSvgRenderer(QString(":/images/rightAligned.svg"))),
      mxd(new QSvgRenderer(QString(":/images/notAligned.svg"))),
      mHideMixed(true) {
    setKind(k_left);
}

AlignTextButton::~AlignTextButton() {
    if (lft)
        delete lft;
    if (cntr)
        delete cntr;
    if (rght)
        delete rght;
    if (mxd)
        delete mxd;
}

void AlignTextButton::setKind(int pKind) {
    if (mHideMixed && pKind == k_mixed) {
        qDebug() << "Mixed button is hidden, can't process it";
        return;
    }
    mKind = pKind;
    QSvgRenderer *rndrer = rndFromKind(pKind);
    Q_ASSERT(rndrer);
    setSharedRenderer(rndrer);
}

void AlignTextButton::setNextKind() {
    int mxKind = MAX_KIND;
    if (mHideMixed) {
        mxKind--;
    }
    setKind(mKind == mxKind ? 0 : ++mKind);
}

int AlignTextButton::nextKind() const {
    int mxKind = MAX_KIND;
    if (mHideMixed) {
        mxKind--;
    }
    int result = mKind;
    return mKind == mxKind ? 0 : ++result;
}

GraphicsTextItemDelegate::GraphicsTextItemDelegate(GraphicsTextItem *pDelegated, QObject *)
    : GraphicsItemDelegate(pDelegated, 0, GF_COMMON | GF_REVOLVABLE | GF_TITLE_BAR_USED),
      mFontButton(0),
      mColorButton(0),
      mDecreaseSizeButton(0),
      mIncreaseSizeButton(0),
      mAlignButton(0),
      mLastFontPixelSize(-1),
      delta(5) {
    delegated()->setData(GraphicsItemData::ItemEditable, QVariant(true));
    delegated()->setPlainText("");

    QTextCursor curCursor = delegated()->textCursor();
    QTextCharFormat format;
    QFont font(createDefaultFont());

    font.setPointSize(Settings::settings()->fontPointSize());
    format.setFont(font);
    if (Settings::settings()->isDarkBackground()) {
        if (GraphicsTextItem::lastUsedTextColor == Qt::black)
            GraphicsTextItem::lastUsedTextColor = Qt::white;
    } else {
        if (GraphicsTextItem::lastUsedTextColor == Qt::white)
            GraphicsTextItem::lastUsedTextColor = Qt::black;
    }
    delegated()->setDefaultTextColor(GraphicsTextItem::lastUsedTextColor);
    format.setForeground(QBrush(GraphicsTextItem::lastUsedTextColor));
    curCursor.mergeCharFormat(format);
    delegated()->setTextCursor(curCursor);
    delegated()->setFont(font);

    delegated()->adjustSize();
    delegated()->contentsChanged();

    connect(delegated()->document(), &QTextDocument::cursorPositionChanged, this,
            &GraphicsTextItemDelegate::onCursorPositionChanged);
    connect(delegated()->document(), &QTextDocument::modificationChanged, this,
            &GraphicsTextItemDelegate::onModificationChanged);
}

GraphicsTextItemDelegate::~GraphicsTextItemDelegate() {
    // NOOP
}

QFont GraphicsTextItemDelegate::createDefaultFont() {
    QTextCharFormat textFormat;

    QString fFamily = Settings::settings()->fontFamily();
    if (!fFamily.isEmpty())
        textFormat.setFontFamily(fFamily);

    bool bold = Settings::settings()->isBoldFont();
    if (bold)
        textFormat.setFontWeight(QFont::Bold);

    bool italic = Settings::settings()->isItalicFont();
    if (italic)
        textFormat.setFontItalic(true);

    QFont font(fFamily, -1, bold ? QFont::Bold : -1, italic);
    int pointSize = Settings::settings()->fontPointSize();
    if (pointSize > 0) {
        font.setPointSize(pointSize);
    }

    return font;
}

void GraphicsTextItemDelegate::createControls() {
    GraphicsItemDelegate::createControls();

    if (!mFontButton) {
        mFontButton = new DelegateButton(":/images/font.svg", mDelegated, mFrame, Qt::TitleBarArea);
        connect(mFontButton, &DelegateButton::clicked, this, &GraphicsTextItemDelegate::pickFont);
        mButtons << mFontButton;
    }
    if (!mColorButton) {
        mColorButton = new DelegateButton(":/images/color.svg", mDelegated, mFrame, Qt::TitleBarArea);
        connect(mColorButton, &DelegateButton::clicked, this, &GraphicsTextItemDelegate::pickColor);
        mButtons << mColorButton;
    }

    if (!mDecreaseSizeButton) {
        mDecreaseSizeButton = new DelegateButton(":/images/minus.svg", mDelegated, mFrame, Qt::TitleBarArea);
        connect(mDecreaseSizeButton, &DelegateButton::clicked, this, &GraphicsTextItemDelegate::decreaseSize);
        mButtons << mDecreaseSizeButton;
    }

    if (!mIncreaseSizeButton) {
        mIncreaseSizeButton = new DelegateButton(":/images/plus.svg", mDelegated, mFrame, Qt::TitleBarArea);
        connect(mIncreaseSizeButton, &DelegateButton::clicked, this, &GraphicsTextItemDelegate::increaseSize);
        mButtons << mIncreaseSizeButton;
    }

    if (!mAlignButton) {
        mAlignButton = new AlignTextButton(":/images/plus.svg", mDelegated, mFrame, Qt::TitleBarArea);
        connect(mAlignButton, &DelegateButton::clicked, this, &GraphicsTextItemDelegate::alignButtonProcess);
        mButtons << mAlignButton;
    }

    foreach (DelegateButton *button, mButtons) {
        button->hide();
        button->setFlag(QGraphicsItem::ItemIsSelectable, true);
    }

    // 改 z 值会重建选择框，并把操作模式默认成“缩放”，
    // 文本需要的却是“调整尺寸”，所以在这里补一次调用。
    frame()->setOperationMode(GraphicsDelegateFrame::Resizing);
}

/**
 * @brief Calculate the width of the toolbar containing the text item-related buttons
 * @return The space between the left-most and right-most buttons in pixels
 */
qreal GraphicsTextItemDelegate::titleBarWidth() {
    if (!mFontButton)
        return 0;

    // refresh the frame and buttons' positions
    positionHandles();

    qreal titleBarWidth(0);
    qreal frameLeftCoordinate = mFontButton->pos().x();
    qreal frameRightCoordinate = frameLeftCoordinate;

    foreach (DelegateButton *button, mButtons) {
        if (button->getSection() == Qt::TitleBarArea) {
            frameLeftCoordinate = qMin(button->pos().x(), frameLeftCoordinate);
            frameRightCoordinate =
                qMax(button->pos().x() + button->boundingRect().width(), frameRightCoordinate);
        }
    }

    return frameRightCoordinate - frameLeftCoordinate;
}

void GraphicsTextItemDelegate::freeButtons() {
    mButtons.removeOne(mFontButton);
    delete mFontButton;
    mFontButton = 0;

    mButtons.removeOne(mColorButton);
    delete mColorButton;
    mColorButton = 0;

    mButtons.removeOne(mDecreaseSizeButton);
    delete mDecreaseSizeButton;
    mDecreaseSizeButton = 0;

    mButtons.removeOne(mIncreaseSizeButton);
    delete mIncreaseSizeButton;
    mIncreaseSizeButton = 0;

    mButtons.removeOne(mAlignButton);
    delete mAlignButton;
    mAlignButton = 0;

    GraphicsItemDelegate::freeButtons();
}


void GraphicsTextItemDelegate::contentsChanged() {
    delegated()->contentsChanged();
}

// This method is used to filter the available fonts. Only the web-compliant fonts
// will remain in the font list.
void GraphicsTextItemDelegate::customize(QFontDialog &fontDialog) {
    if (Settings::settings()->isDarkBackground()) {
        fontDialog.setStyleSheet("background-color: white;");
    }

    QListView *fontNameListView = NULL;
    QList<QListView *> listViews = fontDialog.findChildren<QListView *>();
    if (listViews.count() > 0) {
        fontNameListView = listViews.at(0);
        foreach (QListView *listView, listViews) {
            if (listView->pos().x() < fontNameListView->pos().x())
                fontNameListView = listView;
        }
    }
    if (fontNameListView) {
        QStringListModel *stringListModel = dynamic_cast<QStringListModel *>(fontNameListView->model());
        if (stringListModel) {
            QStringList dialogFontNames = stringListModel->stringList();
            QStringList safeWebFontNames;
            safeWebFontNames.append("Arial");
            safeWebFontNames.append("Arial Black");
            safeWebFontNames.append("Comic Sans MS");
            safeWebFontNames.append("Courier New");
            safeWebFontNames.append("Georgia");
            safeWebFontNames.append("Impact");
            safeWebFontNames.append("Times New Roman");
            safeWebFontNames.append("Trebuchet MS");
            safeWebFontNames.append("Verdana");

            QStringList customFontList = Resources::resources()->customFontList();
            int index = 0;
            foreach (QString dialogFontName, dialogFontNames) {
                if (safeWebFontNames.contains(dialogFontName, Qt::CaseInsensitive) ||
                    customFontList.contains(dialogFontName, Qt::CaseSensitive))
                    index++;
                else
                    stringListModel->removeRow(index);
            }
        }
    }
    QList<QComboBox *> comboBoxes = fontDialog.findChildren<QComboBox *>();
    if (comboBoxes.count() > 0)
        comboBoxes.at(0)->setEnabled(false);
}


void GraphicsTextItemDelegate::pickFont() {
    if (mDelegated && mDelegated->scene() && mDelegated->scene()->views().size() > 0) {
        // https://bugreports.qt.io/browse/QTBUG-79637
        QFontDialog fontDialog(static_cast<QGraphicsView *>(Application::boardController->controlView()));

        fontDialog.setOption(QFontDialog::DontUseNativeDialog);
        fontDialog.setCurrentFont(delegated()->textCursor().charFormat().font());
        customize(fontDialog);

        if (fontDialog.exec()) {
            QFont selectedFont = fontDialog.selectedFont();
            Settings::settings()->setFontFamily(selectedFont.family());
            Settings::settings()->setBoldFont(selectedFont.bold());
            Settings::settings()->setItalicFont(selectedFont.italic());
            Settings::settings()->setFontPointSize(selectedFont.pointSize());

            //setting format for selected item
            QTextCursor curCursor = delegated()->textCursor();
            QTextCharFormat format;
            format.setFont(selectedFont);
            curCursor.mergeCharFormat(format);

            delegated()->setTextCursor(curCursor);

            delegated()->setSelected(true);
            delegated()->document()->adjustSize();
            delegated()->contentsChanged();
        }
    }
}

void GraphicsTextItemDelegate::pickColor() {
    if (mDelegated && mDelegated->scene() && mDelegated->scene()->views().size() > 0) {
        QColorDialog colorDialog(delegated()->defaultTextColor(),
                                 static_cast<QGraphicsView *>(Application::boardController->controlView()));
        colorDialog.setWindowTitle(tr("Text Color"));
        colorDialog.setOption(QColorDialog::DontUseNativeDialog);
        if (Settings::settings()->isDarkBackground()) {
            colorDialog.setStyleSheet("background-color: white;");
        }

        if (colorDialog.exec()) {
            QColor selectedColor = colorDialog.selectedColor();

            QTextCursor curCursor = delegated()->textCursor();

            QTextCharFormat format;
            format.setForeground(QBrush(selectedColor));
            curCursor.mergeCharFormat(format);
            delegated()->setTextCursor(curCursor);

            if (!curCursor.hasSelection() ||
                (curCursor.selectedText().length() == delegated()->toPlainText().length())) {
                delegated()->setDefaultTextColor(selectedColor);
                GraphicsTextItem::lastUsedTextColor = selectedColor;
            }

            delegated()->setSelected(true);
            delegated()->document()->adjustSize();
            delegated()->contentsChanged();
        }
    }
}

void GraphicsTextItemDelegate::decreaseSize() {
    ChangeTextSize(-delta, changeSize);
}

void GraphicsTextItemDelegate::increaseSize() {
    ChangeTextSize(delta, changeSize);
}

void GraphicsTextItemDelegate::alignButtonProcess() {
    qDebug() << "alignButtonProcess() clicked";
    QObject *sndr = sender();

    if (sndr == mAlignButton) {
        qDebug() << "Align button";
        AlignTextButton *asAlText = static_cast<AlignTextButton *>(mAlignButton);
        if (asAlText->nextKind() == AlignTextButton::k_mixed) {
            restoreTextCursorFormats();
            asAlText->setNextKind();
            return;
        }
        asAlText->setNextKind();

        QTextCursor cur = delegated()->textCursor();
        QTextBlockFormat fmt = cur.blockFormat();
        switch (asAlText->kind()) {
            case AlignTextButton::k_left:
                fmt.setAlignment(Qt::AlignLeft);
                break;
            case AlignTextButton::k_center:
                fmt.setAlignment(Qt::AlignCenter);
                break;
            case AlignTextButton::k_right:
                fmt.setAlignment(Qt::AlignRight);
                break;
            case AlignTextButton::k_mixed:
                break;
        }

        delegated()->setTextCursor(cur);
        cur.setBlockFormat(fmt);
        delegated()->setFocus();
    }

    qDebug() << "sender process" << sndr;
}

void GraphicsTextItemDelegate::onCursorPositionChanged(const QTextCursor &cursor) {
    qDebug() << "cursor position changed";
    qDebug() << "-----------------------";
    qDebug() << "we have a selection!" << cursor.selectionStart();
    qDebug() << "-----------------------";
}

void GraphicsTextItemDelegate::onModificationChanged(bool ch) {
    Q_UNUSED(ch);
    qDebug() << "modification changed";
}

void GraphicsTextItemDelegate::onContentChanged() {
    qDebug() << "onContentChanged";
}

GraphicsTextItem *GraphicsTextItemDelegate::delegated() {
    return static_cast<GraphicsTextItem *>(mDelegated);
}

void GraphicsTextItemDelegate::setEditable(bool editable) {
    if (editable) {
        delegated()->setTextInteractionFlags(Qt::TextEditorInteraction);
        mDelegated->setData(GraphicsItemData::ItemEditable, QVariant(true));
    } else {
        QTextCursor cursor(delegated()->document());
        cursor.clearSelection();
        delegated()->setTextCursor(cursor);

        delegated()->setTextInteractionFlags(Qt::NoTextInteraction);
        mDelegated->setData(GraphicsItemData::ItemEditable, QVariant(false));
    }
}

void GraphicsTextItemDelegate::remove(bool canUndo) {
    GraphicsItemDelegate::remove(canUndo);
}

bool GraphicsTextItemDelegate::isEditable() {
    return mDelegated->data(GraphicsItemData::ItemEditable).toBool();
}

void GraphicsTextItemDelegate::decorateMenu(QMenu *menu) {
    GraphicsItemDelegate::decorateMenu(menu);

    mEditableAction = menu->addAction(tr("Editable"), this, &GraphicsTextItemDelegate::setEditable);
    mEditableAction->setCheckable(true);
    mEditableAction->setChecked(isEditable());
}

void GraphicsTextItemDelegate::updateMenuActionState() {
    GraphicsItemDelegate::updateMenuActionState();
}

bool GraphicsTextItemDelegate::mousePressEvent(QGraphicsSceneMouseEvent *event) {
    mSelectionData.mButtonIsPressed = true;
    qDebug() << "Reporting selection  of the cursor (mouse press)"
             << delegated()->textCursor().selection().isEmpty();
    qDebug() << QString("Anchor: %1\nposition: %2 (mouse press)")
                    .arg(delegated()->textCursor().anchor())
                    .arg(delegated()->textCursor().position());

    if (!GraphicsItemDelegate::mousePressEvent(event)) {
        return false;
    }

    return true;
}

bool GraphicsTextItemDelegate::mouseMoveEvent(QGraphicsSceneMouseEvent *event) {
    if (mSelectionData.mButtonIsPressed) {
        qDebug() << "Reporting selection of the cursor (mouse move)"
                 << delegated()->textCursor().selection().isEmpty();
        qDebug() << QString("Anchor: %1\nposition: %2 (mouse mouse move)")
                        .arg(delegated()->textCursor().anchor())
                        .arg(delegated()->textCursor().position());
    }

    if (!GraphicsItemDelegate::mouseMoveEvent(event)) {
        return false;
    }

    return true;
}

bool GraphicsTextItemDelegate::mouseReleaseEvent(QGraphicsSceneMouseEvent *event) {
    mSelectionData.mButtonIsPressed = false;
    qDebug() << "Reporting selection of the cursor (mouse release)"
             << delegated()->textCursor().selection().isEmpty();
    qDebug() << QString("Anchor: %1\nposition: %2 (mouse mouse release)")
                    .arg(delegated()->textCursor().anchor())
                    .arg(delegated()->textCursor().position());
    updateAlighButtonState();

    if (!GraphicsItemDelegate::mouseReleaseEvent(event)) {
        return false;
    }

    return true;
}

bool GraphicsTextItemDelegate::keyPressEvent(QKeyEvent *event) {
    Q_UNUSED(event);
    return true;
}

bool GraphicsTextItemDelegate::keyReleaseEvent(QKeyEvent *event) {
    if (!delegated()->hasFocus()) {
        return true;
    }

    switch (event->key()) {
        case Qt::Key_Left:
        case Qt::Key_Right:
        case Qt::Key_Up:
        case Qt::Key_Down:
            updateAlighButtonState();
            break;
    }

    qDebug() << "Key has been released" << QString::number(event->key(), 16);
    return true;
}

void GraphicsTextItemDelegate::ChangeTextSize(qreal factor, textChangeMode changeMode) {
    // round it to the nearest hundredth
    factor = floor(factor * 100 + 0.5) / 100.;

    if (scaleSize == changeMode) {
        if (1 == factor)
            return;
    } else if (0 == factor)
        return;

    GraphicsTextItem *item = dynamic_cast<GraphicsTextItem *>(delegated());

    if (item && (QString() == item->toPlainText()))
        return;

    QTextCursor cursor = delegated()->textCursor();
    QTextCharFormat textFormat;

    int anchorPos = cursor.anchor();
    int cursorPos = cursor.position();

    if (!cursor.hasSelection()) {
        cursor.select(QTextCursor::Document);
    }

    // Now we got the real start and stop positions
    int startPos = qMin(cursor.anchor(), cursor.position());
    int endPos = qMax(cursor.anchor(), cursor.position());

    QFont curFont;
    QFont nextCharFont;
    bool bEndofTheSameBlock;
    int iBlockLen;
    int iPointSize;
    int iNextPointSize;
    int iCursorPos = startPos;
    QBrush curBrush;
    QBrush nextCharBrush;

    // we search continuous blocks of the text with the same PointSize and allpy new settings for them.
    cursor.setPosition(startPos, QTextCursor::MoveAnchor);
    while (iCursorPos < endPos) {
        bEndofTheSameBlock = false;
        iBlockLen = 0;

        // Here we get the point size of the first character
        cursor.setPosition(iCursorPos + 1, QTextCursor::KeepAnchor);
        curFont = cursor.charFormat().font();
        curBrush = cursor.charFormat().foreground();
        iPointSize = curFont.pointSize();

        // Then we position the end cursor to the start cursor position
        cursor.setPosition(iCursorPos, QTextCursor::KeepAnchor);

        do {
            // Get the next character font size
            cursor.setPosition(iCursorPos + iBlockLen + 1, QTextCursor::KeepAnchor);
            nextCharFont = cursor.charFormat().font();
            nextCharBrush = cursor.charFormat().foreground();
            iNextPointSize = nextCharFont.pointSize();

            if ((iPointSize != iNextPointSize) || (iCursorPos + iBlockLen >= endPos) ||
                (curFont.family().compare(nextCharFont.family()) != 0) ||
                (curFont.italic() != nextCharFont.italic()) || (curFont.bold() != nextCharFont.bold()) ||
                (curFont.underline() != nextCharFont.underline()) || (curBrush != nextCharBrush)) {
                bEndofTheSameBlock = true;
                break;
            }

            iBlockLen++;

        } while (!bEndofTheSameBlock);


        //setting new parameters
        QFont tmpFont = curFont;
        int iNewPointSize = (changeSize == changeMode) ? (iPointSize + factor) : (iPointSize * factor);
        tmpFont.setPointSize((iNewPointSize > 0) ? iNewPointSize : 1);
        textFormat.setFont(tmpFont);
        textFormat.setForeground(curBrush);
        cursor.setPosition(iCursorPos + iBlockLen, QTextCursor::KeepAnchor);
        cursor.mergeCharFormat(textFormat);

        iCursorPos += iBlockLen;
        cursor.setPosition(iCursorPos, QTextCursor::MoveAnchor);

        curFont = nextCharFont;
    }

    delegated()->setFont(curFont);
    Settings::settings()->setFontPointSize(iPointSize);
    //returning initial selection
    cursor.setPosition(anchorPos, QTextCursor::MoveAnchor);
    cursor.setPosition(cursorPos, QTextCursor::KeepAnchor);

    delegated()->setTextCursor(cursor);
}

void GraphicsTextItemDelegate::recolor() {
    QTextCursor cursor = delegated()->textCursor();
    QTextCharFormat textFormat;

    int anchorPos = cursor.anchor();
    int cursorPos = cursor.position();

    if (0 == anchorPos - cursorPos) {
        // If nothing is selected, then we select all the text
        cursor.setPosition(0, QTextCursor::MoveAnchor);
        cursor.setPosition(cursor.document()->characterCount() - 1, QTextCursor::KeepAnchor);
    }

    int startPos = qMin(cursor.anchor(), cursor.position());
    int endPos = qMax(cursor.anchor(), cursor.position());

    QFont curFont;
    QFont nextCharFont;
    bool bEndofTheSameBlock;
    int iBlockLen;
    int iCursorPos = startPos;
    QBrush curBrush;
    QBrush nextCharBrush;

    cursor.setPosition(startPos, QTextCursor::MoveAnchor);
    while (iCursorPos < endPos) {
        bEndofTheSameBlock = false;
        iBlockLen = 0;

        // Here we get the point size of the first character
        cursor.setPosition(iCursorPos + 1, QTextCursor::KeepAnchor);
        curBrush = cursor.charFormat().foreground();

        // Then we position the end cursor to the start cursor position
        cursor.setPosition(iCursorPos, QTextCursor::KeepAnchor);

        do {
            cursor.setPosition(iCursorPos + iBlockLen + 1, QTextCursor::KeepAnchor);
            nextCharBrush = cursor.charFormat().foreground();

            if (curBrush != nextCharBrush || (iCursorPos + iBlockLen >= endPos)) {
                bEndofTheSameBlock = true;
                break;
            }

            iBlockLen++;

        } while (!bEndofTheSameBlock);


        //setting new parameters
        if (delegated()->scene()->isDarkBackground()) {
            if (curBrush.color() == Qt::black) {
                curBrush = QBrush(Qt::white);
            }
        } else {
            if (curBrush.color() == Qt::white) {
                curBrush = QBrush(Qt::black);
            }
        }

        cursor.setPosition(iCursorPos + iBlockLen, QTextCursor::KeepAnchor);
        textFormat.setForeground(curBrush);
        cursor.mergeCharFormat(textFormat);

        iCursorPos += iBlockLen;
        cursor.setPosition(iCursorPos, QTextCursor::MoveAnchor);

        curFont = nextCharFont;
    }

    delegated()->setFont(curFont);
    //returning initial selection
    cursor.setPosition(anchorPos, QTextCursor::MoveAnchor);
    cursor.setPosition(cursorPos, QTextCursor::KeepAnchor);

    delegated()->setTextCursor(cursor);
}

void GraphicsTextItemDelegate::updateAlighButtonState() {
    if (!mAlignButton) {
        return;
    }

    qDebug() << "new cursor position" << delegated()->textCursor().position();
    AlignTextButton *asAlBtn = static_cast<AlignTextButton *>(mAlignButton);

    if (!oneBlockSelection()) {
        asAlBtn->setMixedButtonVisible(true);
        asAlBtn->setKind(AlignTextButton::k_mixed);
        saveTextCursorFormats();
        return;
    }

    asAlBtn->setMixedButtonVisible(false);

    Qt::Alignment cf = delegated()->textCursor().blockFormat().alignment();
    qDebug() << "getting alignment" << cf;

    if (cf & Qt::AlignCenter) {
        asAlBtn->setKind(AlignTextButton::k_center);
    } else if (cf & Qt::AlignRight) {
        asAlBtn->setKind(AlignTextButton::k_right);
    } else {
        asAlBtn->setKind(AlignTextButton::k_left);
    }
}

bool GraphicsTextItemDelegate::oneBlockSelection() {
    const QTextCursor cursor = delegated()->textCursor();
    int pos = cursor.position();
    int anchor = cursor.anchor();

    // no selection
    if (pos == anchor) {
        return true;
    }

    //selecton within one text block
    QTextBlock blck = cursor.block();
    if (blck.contains(pos) && blck.contains(anchor)) {
        return true;
    }

    //otherwise
    return false;
}

void GraphicsTextItemDelegate::saveTextCursorFormats() {
    mSelectionData.anchor = delegated()->textCursor().anchor();
    mSelectionData.position = delegated()->textCursor().position();
    mSelectionData.html = delegated()->document()->toHtml();
}

void GraphicsTextItemDelegate::restoreTextCursorFormats() {
    delegated()->document()->setHtml(mSelectionData.html);

    int min = qMin(mSelectionData.position, mSelectionData.anchor);
    int max = qMax(mSelectionData.position, mSelectionData.anchor);
    int steps = max - min;

    QTextCursor tcrsr = delegated()->textCursor();
    tcrsr.setPosition(mSelectionData.position);
    tcrsr.movePosition(QTextCursor::Left, QTextCursor::KeepAnchor, steps);
    delegated()->setTextCursor(tcrsr);
}

void GraphicsTextItemDelegate::scaleTextSize(qreal multiplyer) {
    ChangeTextSize(multiplyer, scaleSize);
}

QVariant GraphicsTextItemDelegate::itemChange(QGraphicsItem::GraphicsItemChange change,
                                              const QVariant &value) {
    if (change == QGraphicsItem::ItemSelectedChange) {
        if (delegated()->isSelected()) {
            QTextCursor c = delegated()->textCursor();
            if (c.hasSelection()) {
                c.clearSelection();
                delegated()->setTextCursor(c);
            }
        }
    }

    return GraphicsItemDelegate::itemChange(change, value);
}
