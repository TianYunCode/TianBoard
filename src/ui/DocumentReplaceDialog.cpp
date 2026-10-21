#include "ui/DocumentReplaceDialog.h"

#include <QApplication>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QVBoxLayout>

DocumentReplaceDialog::DocumentReplaceDialog(const QString &pIncommingName, const QStringList &pFileList, QWidget *parent, Qt::WindowFlags pFlags)
    : QDialog(parent, pFlags), mFileNameList(pFileList), acceptText(tr("Accept")), replaceText(tr("Replace")), mLabelText(nullptr)
{
    setStyleSheet(QStringLiteral("background:white;"));

    QVBoxLayout *mainLayout  = new QVBoxLayout(this);

    QVBoxLayout *labelLayout = new QVBoxLayout();

    mLabelText               = new QLabel(labelTextWithName(pIncommingName), this);
    mLineEdit                = new QLineEdit(this);
    mLineEdit->setText(pIncommingName);

    labelLayout->addWidget(mLabelText);
    labelLayout->addWidget(mLineEdit);

    QHBoxLayout *buttonLayout = new QHBoxLayout();

    acceptButton              = new QPushButton(acceptText, this);
    QPushButton *cancelButton = new QPushButton(tr("Cancel"), this);
    buttonLayout->addWidget(acceptButton);
    buttonLayout->addWidget(cancelButton);

    mainLayout->addLayout(labelLayout);
    mainLayout->addLayout(buttonLayout);

    acceptButton->setEnabled(false);

    connect(acceptButton, &QPushButton::clicked, this, &DocumentReplaceDialog::accept);
    connect(cancelButton, &QPushButton::clicked, this, &DocumentReplaceDialog::reject);
    connect(mLineEdit, &QLineEdit::textEdited, this, &DocumentReplaceDialog::reactOnTextChanged);

    reactOnTextChanged(pIncommingName);
}

QString DocumentReplaceDialog::labelTextWithName(const QString &documentName) const
{
    return tr("The name %1 is allready used.\nKeeping this name will replace the document.\nProviding a new "
              "name will create a new document.")
        .arg(documentName);
}

void DocumentReplaceDialog::accept()
{
    QDialog::accept();
}

void DocumentReplaceDialog::reject()
{
    mLineEdit->clear();
    QDialog::reject();
}

void DocumentReplaceDialog::reactOnTextChanged(const QString &pStr)
{
    if (!validString(pStr))
    {
        acceptButton->setEnabled(false);
        mLineEdit->setStyleSheet(QStringLiteral("background:#FFB3C8;"));
    }
    else if (mFileNameList.contains(pStr.trimmed(), Qt::CaseSensitive))
    {
        // 同级已有同名文档：允许确认，但语义改为“替换”，用红底提示风险。
        acceptButton->setEnabled(true);
        mLineEdit->setStyleSheet(QStringLiteral("background:#FFB3C8;"));
        acceptButton->setText(replaceText);
    }
    else
    {
        acceptButton->setEnabled(true);
        mLineEdit->setStyleSheet(QStringLiteral("background:white;"));
        acceptButton->setText(acceptText);
    }
}

bool DocumentReplaceDialog::validString(const QString &pStr)
{
    Q_UNUSED(pStr);
    return mLineEdit->hasAcceptableInput();
}

PersistenceManager::DuplicateNameDecision DocumentReplaceDialog::ask(const QString &documentName, const QStringList &siblingNames)
{
    PersistenceManager::DuplicateNameDecision decision;

    // 导入流程外面套着等待光标，模态框得把它临时换成箭头，否则用户对着沙漏点按钮。
    // 重新取一次指针再改回去：exec() 期间别处可能已经弹掉过这层覆盖光标。
    const Qt::CursorShape savedShape = QApplication::overrideCursor() ? QApplication::overrideCursor()->shape() : Qt::ArrowCursor;
    if (QApplication::overrideCursor())
    {
        QApplication::overrideCursor()->setShape(Qt::ArrowCursor);
    }

    // 不带父窗口：对话框由窗口管理器定位，不跟随主窗口所在的那一块屏。
    DocumentReplaceDialog dialog(documentName, siblingNames);
    if (dialog.exec() == QDialog::Accepted && !dialog.lineEditText().isEmpty())
    {
        decision.proceed   = true;
        decision.finalName = dialog.lineEditText();
    }

    if (QApplication::overrideCursor())
    {
        QApplication::overrideCursor()->setShape(savedShape);
    }

    return decision;
}
