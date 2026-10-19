#pragma once

#include "ui/ThumbnailWidget.h"

#include <memory>

class GraphicsScene;

class DocumentThumbnailWidget: public ThumbnailWidget
{
    Q_OBJECT

    public:
        DocumentThumbnailWidget(QWidget* parent);
        virtual ~DocumentThumbnailWidget();

        void setDragEnabled(bool enabled);
        bool dragEnabled() const;

        void hightlightItem(int index);

    public slots:
        virtual void setGraphicsItems(const QList<QGraphicsItem*>& pGraphicsItems,
            const QList<QUrl>& pItemPaths, const QStringList pLabels = QStringList(),
            const QString& pMimeType = QString(""));

	signals:
        void sceneDropped(DocumentProxy* proxy, int source, int target);

    private slots:
        void autoScroll();

    protected:
        virtual void mouseMoveEvent(QMouseEvent *event);

        virtual void dragEnterEvent(QDragEnterEvent *event);
        virtual void dragLeaveEvent(QDragLeaveEvent *event);
        virtual void dragMoveEvent(QDragMoveEvent *event);
        virtual void dropEvent(QDropEvent *event);

    private:
        void deleteDropCaret();

        struct Private;
        std::unique_ptr<Private> d;
};

