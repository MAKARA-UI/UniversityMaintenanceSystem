#include "LoginDialog.h"

#include "data/AccountRepository.h"

#include <QFormLayout>
#include <QComboBox>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QSizePolicy>
#include <QStringList>
#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>

LoginDialog::LoginDialog(AccountRepository &accounts, QWidget *parent)
    : QDialog(parent), m_accounts(accounts)
{
    setWindowTitle("University Maintenance | Sign in");
    setObjectName("authDialog");
    setMinimumSize(820, 640);
    resize(1100, 720);
    setModal(true);

    auto *outerLayout = new QHBoxLayout(this);
    outerLayout->setContentsMargins(28, 28, 28, 28);
    outerLayout->setSpacing(24);

    auto *heroPanel = new QWidget(this);
    heroPanel->setObjectName("authHero");
    heroPanel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    auto *heroLayout = new QVBoxLayout(heroPanel);
    heroLayout->setContentsMargins(52, 56, 52, 56);
    heroLayout->setSpacing(20);

    auto *heroBrand = new QLabel("CAMPUS CARE", heroPanel);
    heroBrand->setObjectName("heroBrand");
    heroLayout->addWidget(heroBrand);
    heroLayout->addStretch(1);
    auto *heroTitle = new QLabel("Better campus spaces start with a report.", heroPanel);
    heroTitle->setObjectName("heroTitle");
    heroTitle->setWordWrap(true);
    heroLayout->addWidget(heroTitle);
    auto *heroDescription = new QLabel(
        "A simple place to report facilities problems and support timely maintenance across campus.",
        heroPanel);
    heroDescription->setObjectName("heroDescription");
    heroDescription->setWordWrap(true);
    heroLayout->addWidget(heroDescription);
    heroLayout->addSpacing(20);
    for (const QString &feature : QStringList{
             QStringLiteral("01   Submit a facilities fault"),
             QStringLiteral("02   Keep reports in one place"),
             QStringLiteral("03   Support a safer campus")}) {
        auto *featureLabel = new QLabel(feature, heroPanel);
        featureLabel->setObjectName("heroFeature");
        heroLayout->addWidget(featureLabel);
    }
    heroLayout->addStretch(1);
    outerLayout->addWidget(heroPanel, 1);

    auto *card = new QWidget(this);
    card->setObjectName("authCard");
    card->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    auto *layout = new QVBoxLayout(card);
    layout->setContentsMargins(30, 28, 30, 26);
    layout->setSpacing(14);

    auto *brand = new QLabel("CAMPUS CARE", card);
    brand->setObjectName("brandLabel");
    layout->addWidget(brand);
    layout->addStretch(1);
    auto *title = new QLabel("Welcome back", card);
    title->setObjectName("authTitle");
    layout->addWidget(title);
    auto *subtitle = new QLabel("Sign in to report and follow up on campus facilities faults.", card);
    subtitle->setWordWrap(true);
    subtitle->setObjectName("mutedText");
    layout->addWidget(subtitle);

    auto *form = new QFormLayout;
    form->setVerticalSpacing(12);
    m_username = new QLineEdit(card);
    m_username->setPlaceholderText("Your username");
    m_username->setMaxLength(40);
    m_password = new QLineEdit(card);
    m_password->setPlaceholderText("Your password");
    m_password->setEchoMode(QLineEdit::Password);
    m_confirmPassword = new QLineEdit(card);
    m_confirmPassword->setPlaceholderText("Enter the password again");
    m_confirmPassword->setEchoMode(QLineEdit::Password);
    m_roleCombo = new QComboBox(card);
    m_roleCombo->addItems(AccountRepository::availableRoles());
    m_roleCombo->setCurrentText("Reporter");
    form->addRow("Username", m_username);
    form->addRow("Password", m_password);
    m_confirmLabel = new QLabel("Confirm password", card);
    form->addRow(m_confirmLabel, m_confirmPassword);
    m_roleLabel = new QLabel("Account type", card);
    form->addRow(m_roleLabel, m_roleCombo);
    layout->addLayout(form);
    layout->addStretch(1);

    m_message = new QLabel(card);
    m_message->setObjectName("formMessage");
    m_message->setWordWrap(true);
    layout->addWidget(m_message);

    auto *action = new QPushButton(card);
    action->setObjectName("primaryButton");
    action->setMinimumHeight(42);
    layout->addWidget(action);
    auto *toggle = new QPushButton(card);
    toggle->setObjectName("linkButton");
    layout->addWidget(toggle, 0, Qt::AlignHCenter);

    outerLayout->addWidget(card, 1);

    connect(action, &QPushButton::clicked, this, &LoginDialog::submit);
    connect(toggle, &QPushButton::clicked, this, [this] {
        setSignUpMode(!m_signUpMode);
    });
    connect(m_password, &QLineEdit::returnPressed, action, &QPushButton::click);
    setSignUpMode(false);
}

QString LoginDialog::username() const
{
    return m_username->text().trimmed();
}

QString LoginDialog::role() const
{
    return m_role;
}

void LoginDialog::setSignUpMode(bool enabled)
{
    m_signUpMode = enabled;
    m_confirmPassword->setVisible(enabled);
    m_confirmLabel->setVisible(enabled);
    m_roleCombo->setVisible(enabled);
    m_roleLabel->setVisible(enabled);
    auto *action = findChild<QPushButton *>("primaryButton");
    auto *toggle = findChild<QPushButton *>("linkButton");
    findChild<QLabel *>("authTitle")->setText(enabled ? "Create your account" : "Welcome back");
    action->setText(enabled ? "Create account" : "Sign in");
    toggle->setText(enabled ? "Already registered? Sign in" : "New here? Create an account");
    m_message->clear();
}

void LoginDialog::submit()
{
    const QString user = username();
    const QString password = m_password->text();
    if (user.trimmed().size() < 3) {
        m_message->setText("Enter a username with at least 3 characters.");
        return;
    }
    if (password.size() < 10) {
        m_message->setText("Use a password with at least 10 characters.");
        return;
    }

    QString error;
    if (m_signUpMode) {
        if (password != m_confirmPassword->text()) {
            m_message->setText("The passwords do not match.");
            return;
        }
        m_role = m_roleCombo->currentText();
        if (!m_accounts.createAccount(user, password, m_role, &error)) {
            m_message->setText(error);
            return;
        }
        accept();
        return;
    }

    if (!m_accounts.authenticate(user, password, &m_role, &error)) {
        m_message->setText(error);
        return;
    }
    accept();
}
