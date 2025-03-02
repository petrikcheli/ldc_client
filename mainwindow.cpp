#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include "p2p.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::connection_to_the_server(io_context &ioc, ip::tcp::resolver &resolver)
{
    p2p_worker = std::make_shared<p2p>(ioc, resolver);
}

void MainWindow::on_pb_reg_clicked()
{
    p2p_worker->set_self_id(ui->self_id->text().toStdString());
    //p2p_worker->set_peer_id(ui->peer_id->text().toStdString());
    p2p_worker->register_on_server();
}

void MainWindow::on_pd_offer_clicked()
{
    //p2p_worker->set_self_id(ui->self_id->text().toStdString());
    p2p_worker->set_peer_id(ui->peer_id->text().toStdString());
    p2p_worker->send_offer();
    //p2p_worker->register_on_server();
}
