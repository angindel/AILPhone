#include "login.h"
#include "ui_login.h"

#include <QMessageBox>

Login::Login(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::Login)
{
    ui->setupUi(this);
    this->setStyleSheet("background-color: rgb(85, 170, 255);"
                        "border-color: rgb(0, 0, 0);");
}

Login::~Login()
{
    delete ui;
}

void Login::on_pushButton_clicked()
{
    QString username, password;
    username = ui->lineEdit->text();
    password = ui->lineEdit_2->text();

    if(username == "admin" && password == "admin")
    {
        MainWindow *w = new MainWindow(this);
        this->hide();
        w->show();
    }
    else
    {
        QMessageBox msg;
        msg.setText("Username atau Password salah");
        msg.exec();
    }

}

