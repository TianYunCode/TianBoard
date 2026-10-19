#include <QLayout>
#include <QTextEdit>
#include <QPushButton>

#include "ui/MessagesDialog.h"

struct MessagesDialog::Private
{
    QList<QString> mMessages;
    int mMessagesFontSize{0};
};

MessagesDialog::MessagesDialog(QString windowTitle, QWidget *parent)
: QWidget(parent), d(new Private)
{
    resize(400, 0);

    setWindowTitle(windowTitle);
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
        connect(closeButton, SIGNAL(clicked(bool)), this, SLOT(dispose()));

        messagesLayout->addWidget(closeButton);

        setLayout(messagesLayout);
    }
}

void MessagesDialog::dispose()
{
    delete this;
}
