#pragma once

#include <QtWidgets>
#include <QtSvg>
#include <QtSvgWidgets>
#include <QElapsedTimer>
#include <QGraphicsSceneHoverEvent>

#include "core/Application.h"
#include "whiteboard/BoardController.h"
#include "utils/CoreGraphicsScene.h"
#include "core/Settings.h"
#include "whiteboard/Item.h"
#include "ui/ThumbnailView.h"
#include "model/DocumentProxy.h"

#include <QLabel>
#include <QHBoxLayout>

#include <memory>

#define STARTDRAGTIME   1000000
#define BUTTONSIZE      96
#define BUTTONSPACING 10

class DocumentProxy;
class ThumbnailTextItem;
class Thumbnail;

class ThumbnailWidget : public QGraphicsView
{
    Q_OBJECT

public:
    ThumbnailWidget(QWidget* parent);
    virtual ~ThumbnailWidget();

    QList<QGraphicsItem*> selectedItems();
    void selectItemAt(int pIndex, bool extend = false);
    void unselectItemAt(int pIndex);

    qreal thumbnailWidth();

    void setBackgroundBrush(const QBrush& brush);

public slots:
    void setThumbnailWidth(qreal pThumbnailWidth);
    void setSpacing(qreal pSpacing);
    virtual void setGraphicsItems(const QList<QGraphicsItem*>& pGraphicsItems, const QList<QUrl>& pItemPaths, const QStringList pLabels = QStringList(), const QString& pMimeType = QString(""));
    void refreshScene();
    void sceneSelectionChanged();

signals:
    void resized();
    void selectionChanged();
    void mouseDoubleClick(QGraphicsItem* item, int index);
    void mouseClick(QGraphicsItem* item, int index);


protected:
    virtual void mousePressEvent(QMouseEvent *event);
    virtual void mouseMoveEvent(QMouseEvent *event);
    virtual void mouseReleaseEvent(QMouseEvent *event);
    virtual void resizeEvent(QResizeEvent * event);
    void mouseDoubleClickEvent(QMouseEvent * event);

    virtual void keyPressEvent(QKeyEvent *event);
    virtual void focusInEvent(QFocusEvent *event);

    const QList<QGraphicsItem*>& graphicItems() const;
    const QList<ThumbnailTextItem*>& labelsItems() const;
    const QPoint& mousePressPos() const;

    qreal spacing();
    void setCanDrag(bool canDrag);

private:
    void selectAll();
    void selectItems(int startIndex, int count);
    int rowCount() const;
    int columnCount() const;

    static bool thumbnailLessThan(QGraphicsItem* item1, QGraphicsItem* item2);

    void deleteLasso();

    struct Private;
    std::unique_ptr<Private> d;
};

class Thumbnail
{
public:
    Thumbnail();

    virtual ~Thumbnail();

    QStyleOptionGraphicsItem muteStyleOption(const QStyleOptionGraphicsItem *option)
    {
        QStyleOptionGraphicsItem styleOption = QStyleOptionGraphicsItem(*option);
        styleOption.state &= ~QStyle::State_Selected;

        return styleOption;
    }

    virtual void itemChange(QGraphicsItem *item, QGraphicsItem::GraphicsItemChange change, const QVariant &value);

    int column();
    void setColumn(int column);
    int row();
    void setRow(int row);
    ThumbnailTextItem *label();
    void setLabel(ThumbnailTextItem *label);

private:
    struct Private;
    std::unique_ptr<Private> d;
};

class ThumbnailTextItem : public QGraphicsTextItem
{
    Q_OBJECT
public:
    ThumbnailTextItem(int index);

    ThumbnailTextItem(const QString& text);

    virtual ~ThumbnailTextItem();

    QRectF boundingRect() const;

    void setWidth(qreal pWidth);

    qreal width();

    void highlight();

    void computeText();

private:
    struct Private;
    std::unique_ptr<Private> d;
};

class ThumbnailPixmap : public QGraphicsPixmapItem, public Thumbnail
{
public:
    ThumbnailPixmap(const QPixmap& pix)
        : QGraphicsPixmapItem(pix)
    {
        setTransformationMode(Qt::SmoothTransformation);
        setShapeMode(QGraphicsPixmapItem::BoundingRectShape);
        setFlag(QGraphicsItem::ItemSendsGeometryChanges, true);
    }

    virtual ~ThumbnailPixmap()
    {
        // NOOP
    }

    virtual QPainterPath shape () const
    {
        QPainterPath path;
        path.addRect(boundingRect());
        return path;
    }


    virtual void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
    {
        QStyleOptionGraphicsItem styleOption = Thumbnail::muteStyleOption(option);
        QGraphicsPixmapItem::paint(painter, &styleOption, widget);
    }

    virtual QVariant itemChange(GraphicsItemChange change, const QVariant &value)
    {
        Thumbnail::itemChange(this, change, value);
        return QGraphicsPixmapItem::itemChange(change, value);
    }
};

class SceneThumbnailPixmap : public ThumbnailPixmap
{
public:
    SceneThumbnailPixmap(const QPixmap& pix, DocumentProxy* proxy, int pSceneIndex);

    virtual ~SceneThumbnailPixmap();

    DocumentProxy* proxy();

    int sceneIndex();

    void highlight();

private:
    struct Private;
    std::unique_ptr<Private> d;
};

class SceneThumbnailNavigPixmap : public SceneThumbnailPixmap
{
public:
    SceneThumbnailNavigPixmap(const QPixmap& pix, DocumentProxy* proxy, int pSceneIndex);
    ~SceneThumbnailNavigPixmap();

    bool editable();

    bool deletable();

    bool movableUp();

    bool movableDown();

    void showUI();

    void hideUI();

    void setEditable(bool editable);

protected:
    void hoverEnterEvent(QGraphicsSceneHoverEvent *event);
    void hoverLeaveEvent(QGraphicsSceneHoverEvent *event);
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget);
    void mousePressEvent(QGraphicsSceneMouseEvent *event);

private:
    void deletePage();
    void duplicatePage();
    void moveUpPage();
    void moveDownPage();

    struct Private;
    std::unique_ptr<Private> d;
};

class ImgTextThumbnailElement
{
private:
    SceneThumbnailNavigPixmap* thumbnail;
    ThumbnailTextItem* caption;
    int border;

public:
    ImgTextThumbnailElement(SceneThumbnailNavigPixmap* thumb, ThumbnailTextItem* text): border(0)
    {
        this->thumbnail = thumb;
        this->caption = text;
    }

    SceneThumbnailNavigPixmap* getThumbnail() const { return this->thumbnail; }
    void setThumbnail(SceneThumbnailNavigPixmap* newGItem) { this->thumbnail = newGItem; }

    ThumbnailTextItem* getCaption() const { return this->caption; }
    void setCaption(ThumbnailTextItem* newcaption) { this->caption = newcaption; }

    void Place(int row, int col, qreal width, qreal height);

    int getBorder() const { return this->border; }
    void setBorder(int newBorder) { this->border = newBorder; }
};

class ThumbnailProxyWidget : public QGraphicsProxyWidget
{
public:
    ThumbnailProxyWidget(DocumentProxy* proxy, int index);

    virtual ~ThumbnailProxyWidget();

    DocumentProxy* documentProxy();

    void setSceneIndex(int i);

    int sceneIndex();

private:
    struct Private;
    std::unique_ptr<Private> d;
};

class DraggableThumbnail : public ThumbnailProxyWidget
{
    Q_OBJECT
public:
    DraggableThumbnail(DocumentProxy* documentProxy, int index);

    ~DraggableThumbnail();

    bool editable();

    bool deletable();

    bool movableUp();

    bool movableDown();

    void showUI();

    void hideUI();

    void setEditable(bool editable);


    ThumbnailTextItem* pageNumber();

    void setPageNumber(int i);

    virtual void updatePos(qreal w, qreal h);

protected:
    void hoverEnterEvent(QGraphicsSceneHoverEvent *event);
    void hoverLeaveEvent(QGraphicsSceneHoverEvent *event);
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget);
    void mousePressEvent(QGraphicsSceneMouseEvent *event);
    void mouseReleaseEvent(QGraphicsSceneMouseEvent *event);

private:
    void deletePage();
    void duplicatePage();
    void moveUpPage();
    void moveDownPage();

    struct Private;
    std::unique_ptr<Private> d;
};

class DraggableThumbnailPixmap : public DraggableThumbnail
{
    Q_OBJECT
public:
    DraggableThumbnailPixmap(ThumbnailPixmap* thumbnailPixmap, DocumentProxy* documentProxy, int index);

    ~DraggableThumbnailPixmap();

    ThumbnailPixmap* thumbnailPixmap();

    void updatePos(qreal w, qreal h);

private:
    struct Private;
    std::unique_ptr<Private> d;
};

class DraggableThumbnailView : public DraggableThumbnail
{
    Q_OBJECT
public:
    DraggableThumbnailView(ThumbnailView* thumbnailView, DocumentProxy* documentProxy, int index);

    ~DraggableThumbnailView();

    ThumbnailView* thumbnailView();

    ThumbnailTextItem* pageNumber();

    void setPageNumber(int i);

private:
    struct Private;
    std::unique_ptr<Private> d;
};

namespace ThumbnailUI
{
    const int ICONSIZE      = 96;
    const int ICONSPACING   = 10;

    class ThumbnailUIIcon : public QPixmap
    {
        public:
            ThumbnailUIIcon(const QString& filename, int pos);

            ~ThumbnailUIIcon();

            int pos() const;

            bool triggered(qreal x) const;

        private:
            struct Private;
            std::unique_ptr<Private> d;
    };

    namespace _private
    {
        static QMap<QString, ThumbnailUIIcon*> catalog;
        void initCatalog();
    }

    ThumbnailUIIcon* addIcon(const QString& thumbnailIcon, int pos);
    ThumbnailUIIcon* getIcon(const QString& thumbnailIcon);
    void draw(QPainter* painter, const ThumbnailUIIcon& thumbnailIcon);
    bool triggered(qreal y);
}

