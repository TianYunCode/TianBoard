#include "ui/ProxyDialog.h"

#include <QtWidgets>

struct ProxyDialog::Private
{
    QGridLayout      *gridLayout{nullptr};
    QLabel           *iconLabel{nullptr};
    QLabel           *introLabel{nullptr};
    QLabel           *usernameLabel{nullptr};
    QLineEdit        *userNameLineEdit{nullptr};
    QLabel           *passwordLabel{nullptr};
    QLineEdit        *passwordLineEdit{nullptr};
    QDialogButtonBox *buttonBox{nullptr};
    QCheckBox        *saveCredentials{nullptr};
};

ProxyDialog::ProxyDialog(QWidget *parent) : QDialog(parent), d(new Private)
{
    if (objectName().isEmpty())
    {
        setObjectName("ProxyDialog");
    }
    resize(369, 161);

    d->gridLayout = new QGridLayout(this);
    d->gridLayout->setObjectName("gridLayout");

    d->iconLabel = new QLabel(this);
    d->iconLabel->setObjectName("iconLabel");
    d->iconLabel->setText(QString::fromUtf8("ICON"));
    d->gridLayout->addWidget(d->iconLabel, 0, 0, 1, 1);

    d->introLabel = new QLabel(this);
    d->introLabel->setObjectName("introLabel");
    d->introLabel->setWordWrap(true);
    d->introLabel->setText(tr("Connect to Proxy"));
    d->gridLayout->addWidget(d->introLabel, 0, 1, 1, 2);

    d->usernameLabel = new QLabel(this);
    d->usernameLabel->setObjectName("usernameLabel");
    d->usernameLabel->setText(tr("Username:"));
    d->gridLayout->addWidget(d->usernameLabel, 1, 0, 1, 2);

    d->userNameLineEdit = new QLineEdit(this);
    d->userNameLineEdit->setObjectName("userNameLineEdit");
    d->gridLayout->addWidget(d->userNameLineEdit, 1, 2, 1, 1);

    d->passwordLabel = new QLabel(this);
    d->passwordLabel->setObjectName("passwordLabel");
    d->passwordLabel->setText(tr("Password:"));
    d->gridLayout->addWidget(d->passwordLabel, 2, 0, 1, 2);

    d->passwordLineEdit = new QLineEdit(this);
    d->passwordLineEdit->setObjectName("passwordLineEdit");
    d->passwordLineEdit->setEchoMode(QLineEdit::Password);
    d->gridLayout->addWidget(d->passwordLineEdit, 2, 2, 1, 1);

    d->buttonBox = new QDialogButtonBox(this);
    d->buttonBox->setObjectName("buttonBox");
    d->buttonBox->setOrientation(Qt::Horizontal);
    d->buttonBox->setStandardButtons(QDialogButtonBox::Cancel | QDialogButtonBox::Ok);
    d->gridLayout->addWidget(d->buttonBox, 6, 0, 1, 3);

    d->saveCredentials = new QCheckBox(this);
    d->saveCredentials->setObjectName("saveCredentials");
    d->saveCredentials->setText(tr("Save username and password for future use"));
    d->gridLayout->addWidget(d->saveCredentials, 3, 2, 1, 1);

    // 移植自 Designer 表单的占位文字 "ICON" 不适合作为提示图形，
    // 这里换成主题提供的标准问号图标。
    d->iconLabel->setText(QString());
    d->iconLabel->setPixmap(style()->standardIcon(QStyle::SP_MessageBoxQuestion, nullptr, this).pixmap(32, 32));

    connect(d->buttonBox, &QDialogButtonBox::accepted, this, qOverload<>(&QDialog::accept));
    connect(d->buttonBox, &QDialogButtonBox::rejected, this, qOverload<>(&QDialog::reject));
}

ProxyDialog::~ProxyDialog() = default;

void ProxyDialog::setIntroText(const QString &text)
{
    d->introLabel->setText(text);
}

void ProxyDialog::setUserName(const QString &userName)
{
    d->userNameLineEdit->setText(userName);
}

void ProxyDialog::setPassword(const QString &password)
{
    d->passwordLineEdit->setText(password);
}

QString ProxyDialog::userName() const
{
    return d->userNameLineEdit->text();
}

QString ProxyDialog::password() const
{
    return d->passwordLineEdit->text();
}

bool ProxyDialog::shouldSaveCredentials() const
{
    return d->saveCredentials->isChecked();
}
