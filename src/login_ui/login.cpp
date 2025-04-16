#include "login.h"
#include "ui_login.h"

Login::Login(User_api *api_worker_, QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::login)
{
    ui->setupUi(this);

    this->api_worker_ = api_worker_;

    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->setAlignment(Qt::AlignCenter);

    // Внутренний layout для формы
    auto *formLayout = new QVBoxLayout();
    formLayout->setSpacing(15); // Расстояние между элементами

    QFont titleFont;
    titleFont.setPointSize(18);
    titleFont.setBold(true);

    auto *title = new QLabel("Вход в систему");
    title->setFont(titleFont);
    title->setAlignment(Qt::AlignCenter);
    formLayout->addWidget(title);

    usernameEdit = new QLineEdit();
    usernameEdit->setPlaceholderText("Имя пользователя");
    usernameEdit->setMinimumWidth(250);
    usernameEdit->setFixedHeight(30);
    formLayout->addWidget(usernameEdit);

    passwordEdit = new QLineEdit();
    passwordEdit->setPlaceholderText("Пароль");
    passwordEdit->setEchoMode(QLineEdit::Password);
    passwordEdit->setFixedHeight(30);
    formLayout->addWidget(passwordEdit);

    loginButton = new QPushButton("Войти");
    loginButton->setFixedHeight(35);
    formLayout->addWidget(loginButton);

    messageLabel = new QLabel();
    messageLabel->setStyleSheet("color: red;");
    messageLabel->setAlignment(Qt::AlignCenter);
    formLayout->addWidget(messageLabel);

    // Добавляем форму по центру
    mainLayout->addStretch();
    mainLayout->addLayout(formLayout);
    mainLayout->addStretch();

    connect(loginButton, &QPushButton::clicked, this, &Login::attemptLogin);
    /*
    auto *layout = new QVBoxLayout(this);

    usernameEdit = new QLineEdit(this);
    usernameEdit->setPlaceholderText("Имя пользователя");

    passwordEdit = new QLineEdit(this);
    passwordEdit->setPlaceholderText("Пароль");
    passwordEdit->setEchoMode(QLineEdit::Password);

    loginButton = new QPushButton("Войти", this);
    messageLabel = new QLabel(this);

    layout->addWidget(usernameEdit);
    layout->addWidget(passwordEdit);
    layout->addWidget(loginButton);
    layout->addWidget(messageLabel);

    connect(loginButton, &QPushButton::clicked, this, &Login::attemptLogin);
    api_worker_ = new User_api();
    */
}

void Login::attemptLogin()
{
    QString username = usernameEdit->text();
    QString password = passwordEdit->text();

    if (username.isEmpty() || password.isEmpty()) {
        messageLabel->setText("Введите имя пользователя и пароль");
        return;
    }

    auto check = api_worker_->login_user(username.toStdString(), password.toStdString());
    if(check){
        QMessageBox::information(this, "Успех", "Вы вошли в систему!");
        this->username = usernameEdit->text().toStdString();
        this->hide();  // скрываем окно
        emit loginSuccessful();
    } else {
        messageLabel->setText("Неверное имя пользователя или пароль");
    }

}
