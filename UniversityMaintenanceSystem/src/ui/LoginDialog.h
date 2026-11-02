#pragma once

#include <QDialog>

class AccountRepository;
class QComboBox;
class QLineEdit;
class QLabel;
class QWidget;

class LoginDialog final : public QDialog
{
public:
    explicit LoginDialog(AccountRepository &accounts, QWidget *parent = nullptr);
    QString username() const;
    QString role() const;

private:
    void setSignUpMode(bool enabled);
    void submit();

    AccountRepository &m_accounts;
    QLineEdit *m_username = nullptr;
    QLineEdit *m_password = nullptr;
    QLineEdit *m_confirmPassword = nullptr;
    QComboBox *m_roleCombo = nullptr;
    QLabel *m_confirmLabel = nullptr;
    QLabel *m_roleLabel = nullptr;
    QLabel *m_message = nullptr;
    bool m_signUpMode = false;
    QString m_role;
};
