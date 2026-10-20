#include "whiteboard/UndoCommand.h"


UndoCommand::UndoCommand(QUndoCommand *parent) : QUndoCommand(parent) {
    // 刻意留空
}

UndoCommand::~UndoCommand() {
    // 刻意留空
}
