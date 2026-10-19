#pragma once

#define CONTROLVIEW_OBJ_NAME "ControlView"

#include <QtWidgets>
#include <QGraphicsView>
#include <QRubberBand>

#include <memory>

#include "utils/Enums.h"
#include "whiteboard/GraphicsDelegateFrame.h"

class BoardController;
class GraphicsScene;
class GraphicsWidgetItem;
class RubberBand;

class BoardView : public QGraphicsView
{
    Q_OBJECT

	public:
		BoardView(BoardController* pController, QWidget* pParent = 0, bool isControl = false, bool isDesktop = false);
		BoardView(BoardController* pController, int pStartLayer, int pEndLayer, QWidget* pParent = 0, bool isControl = false, bool isDesktop = false);
		virtual ~BoardView();

		GraphicsScene* scene();

		void forcedTabletRelease();

		void setToolCursor(int tool);

		void rubberItems();
		void moveRubberedItems(QPointF movingVector);

		void setMultiselection(bool enable);
		bool isMultipleSelectionEnabled();

	#if defined(Q_OS_OSX)
		bool directTabletEvent(QEvent *event);
		QWidget *widgetForTabletEvent(QWidget *w, const QPoint &pos);
	#endif
	signals:
		void resized(QResizeEvent* event);
		void shown();
		void mouseReleased();

	protected:
		bool itemIsLocked(QGraphicsItem *item);
		bool isBoardItem(QGraphicsItem *item);
		bool isCppTool(QGraphicsItem *item);
		void handleItemsSelection(QGraphicsItem *item);
		bool itemShouldReceiveMousePressEvent(QGraphicsItem *item);
		bool itemShouldReceiveSuspendedMousePressEvent(QGraphicsItem *item);
		bool itemHaveParentWithType(QGraphicsItem *item, int type);
		bool itemShouldBeMoved(QGraphicsItem *item);
		QGraphicsItem* determineItemToPress(QGraphicsItem *item);
		QGraphicsItem* determineItemToMove(QGraphicsItem *item);
		void handleItemMousePress(QMouseEvent *event);
		void handleItemMouseMove(QMouseEvent *event);

		virtual bool event (QEvent * e);

		virtual void keyPressEvent(QKeyEvent *event);
		virtual void keyReleaseEvent(QKeyEvent *event);
		virtual void tabletEvent(QTabletEvent * event);
		virtual void mouseDoubleClickEvent(QMouseEvent *event);
		virtual void mousePressEvent(QMouseEvent *event);
		virtual void mouseMoveEvent(QMouseEvent *event);
		virtual void mouseReleaseEvent(QMouseEvent *event);
		virtual void wheelEvent(QWheelEvent *event);
		virtual void leaveEvent ( QEvent * event);

		virtual void focusOutEvent ( QFocusEvent * event );

		virtual void drawItems(QPainter *painter, int numItems,
							   QGraphicsItem *items[],
							   const QStyleOptionGraphicsItem options[]);

		virtual void dropEvent(QDropEvent *event);
		virtual void dragMoveEvent(QDragMoveEvent *event);

		virtual void resizeEvent(QResizeEvent * event);

		virtual void drawBackground(QPainter *painter, const QRectF &rect);

	private:
		void init();

		bool shouldDisplayItem(QGraphicsItem *item);

		QList<QUrl> processMimeData(const QMimeData* pMimeData);

		bool isAbsurdPoint(QPoint point);

		static bool hasSelectedParents(QGraphicsItem * item);

		struct Private;
		std::unique_ptr<Private> d;

	private slots:
		void settingChanged(QVariant newValue);

	public slots:
		void virtualKeyboardActivated(bool b);
		void longPressEvent();

};

