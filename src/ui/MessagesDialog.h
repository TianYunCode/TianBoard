#pragma once

#include <QtWidgets>
#include <QWidget>

#include <memory>

class MessagesDialog : public QWidget {
    Q_OBJECT

public:
    MessagesDialog(QString windowTitle, QWidget *parent = NULL);
    ~MessagesDialog() override;

    void setMessages(const QList<QString> messages);

private slots:
    void dispose();

private:
    struct Private;
    std::unique_ptr<Private> d;
};
