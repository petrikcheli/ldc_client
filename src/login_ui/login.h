#ifndef LOGIN_H
#define LOGIN_H

#include <QApplication>
#include <QWidget>
#include <QVBoxLayout>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include <QMessageBox>

#include "../user_api/user_api.h"

namespace Ui {
class login;
}

class Login : public QWidget
{
    Q_OBJECT

public:
    explicit Login(User_api *api_worker_, QWidget *parent = nullptr);
   // ~Login();


public:
    std::string token;
    std::string username;

signals:
    void loginSuccessful();

private slots:
    void attemptLogin();

private:
    QLineEdit *usernameEdit;
    QLineEdit *passwordEdit;
    QPushButton *loginButton;
    QLabel *messageLabel;
    User_api *api_worker_;
    Ui::login *ui;
};

#endif // LOGIN_H
