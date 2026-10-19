#pragma once

#include <QtWidgets>
#include <QUndoCommand>
#include "utils/Enums.h"

class UndoCommand : public QUndoCommand {
public:
    UndoCommand(QUndoCommand *parent = 0);
    ~UndoCommand();

    virtual int getType() const { return UndoType::undotype_UNKNOWN; }
};
