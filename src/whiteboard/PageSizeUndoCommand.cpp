#include "whiteboard/PageSizeUndoCommand.h"

#include <QtWidgets>

#include "core/Application.h"
#include "whiteboard/BoardController.h"
#include "whiteboard/GraphicsScene.h"


PageSizeUndoCommand::PageSizeUndoCommand(GraphicsScene *pScene, const QSize &previousSize,
                                         const QSize &newSize)
    : mScene(pScene), mPreviousSize(previousSize), mNewSize(newSize) {
    mFirstRedo = true;
}

PageSizeUndoCommand::~PageSizeUndoCommand() {
    // 有意为空：无资源需要手动释放
}

void PageSizeUndoCommand::undo() {
    if (!mScene) {
        return;
    }

    Application::boardController->setPageSize(mPreviousSize);
    // 强制刷新：Qt 触发重绘偏慢（耗时疑与项数量的平方成正比？）
    mScene->update(mScene->sceneRect());
}

void PageSizeUndoCommand::redo() {
    // 撤销框架在压入命令时会先调用一次 redo；
    // 此时新页面尺寸已经生效，不能重复执行
    if (!mFirstRedo) {
        if (!mScene) {
            return;
        }

        Application::boardController->setPageSize(mNewSize);
        // 强制刷新：Qt 触发重绘偏慢（耗时疑与项数量的平方成正比）
        mScene->update(mScene->sceneRect());
    } else {
        mFirstRedo = false;
    }
}
