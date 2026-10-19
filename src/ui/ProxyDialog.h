#pragma once

#include <QDialog>

#include <memory>

class ProxyDialog : public QDialog {
    Q_OBJECT

public:
    ProxyDialog(QWidget *parent = nullptr);
    ~ProxyDialog() override;

    void setIntroText(const QString &text);
    void setUserName(const QString &userName);
    void setPassword(const QString &password);

    QString userName() const;
    QString password() const;
    bool shouldSaveCredentials() const;

private:
    struct Private;
    std::unique_ptr<Private> d;
};
