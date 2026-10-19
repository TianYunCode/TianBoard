#include "whiteboard/PageSizeUndoCommand.h"

#include <QtWidgets>

#include "core/Application.h"
#include "whiteboard/BoardController.h"
#include "whiteboard/GraphicsScene.h"


PageSizeUndoCommand::PageSizeUndoCommand(GraphicsScene* pScene, const QSize& previousSize, const QSize& newSize)
    : mScene(pScene)
        , mPreviousSize(previousSize)
        , mNewSize(newSize)
{
    mFirstRedo = true;
}

PageSizeUndoCommand::~PageSizeUndoCommand()
{
   //NOOP
}

void PageSizeUndoCommand::undo()
{
    if (!mScene){
        return;
    }

    Application::boardController->setPageSize(mPreviousSize);
    // force refresh, QT is a bit lazy and take a lot of time (nb item ^2 ?) to trigger repaint
    mScene->update(mScene->sceneRect());

}

void PageSizeUndoCommand::redo()
{
    // the Undo framework calls a redo while appending the undo command.
    // as we have already plotted the elements, we do not want to do it twice
    if (!mFirstRedo)
    {
        if (!mScene){
            return;
        }

        Application::boardController->setPageSize(mNewSize);
        // force refresh, QT is a bit lazy and take a lot of time (nb item ^2) to trigger repaint
        mScene->update(mScene->sceneRect());
    }
    else
    {
        mFirstRedo = false;
    }
}
