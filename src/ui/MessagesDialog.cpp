#include <QLayout>
#include <QTextEdit>
#include <QPushButton>

#include "ui/MessagesDialog.h"

struct MessagesDialog::Private
{
    QList<QString> mMessages;
    int            mMessagesFontSize{0};
};

MessagesDialog::MessagesDialog(QString windowTitle, QWidget *parent) : QWidget(parent), d(new Private)
{
    resize(400, 0);

    setWindowTitle(windowTitle);
    // 用工具窗口而不是独立主窗口：不进任务栏，且始终压在其它窗口之上。
    setWindowFlags(Qt::Tool | Qt::WindowStaysOnTopHint);
}

MessagesDialog::~MessagesDialog() = default;

void MessagesDialog::setMessages(const QList<QString> messages)
{
    d->mMessages = messages;

    if (d->mMessages.count())
    {
        QVBoxLayout *messagesLayout = new QVBoxLayout(this);
        foreach (QString message, d->mMessages)
        {
            QTextEdit *messageBox = new QTextEdit(this);
            messageBox->setMinimumHeight(55);
            messageBox->setReadOnly(true);
            messageBox->setFocusPolicy(Qt::NoFocus);
            messageBox->setText(message);
            messagesLayout->addWidget(messageBox);
        }
        QPushButton *closeButton = new QPushButton(tr("Close"), this);
        connect(closeButton, &QPushButton::clicked, this, &MessagesDialog::dispose);

        messagesLayout->addWidget(closeButton);

        setLayout(messagesLayout);
    }
}

void MessagesDialog::dispose()
{
    // 调用方创建后不再持有指针，因此关闭时由自己销毁。
    delete this;
}
