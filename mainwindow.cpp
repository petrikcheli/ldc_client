#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include "p2p.h"
#include <QApplication>
#include <QImage>
#include <QPixmap>
#include <QLabel>
#include <QBuffer>
#include <QByteArray>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    ui->label->setScaledContents(true);
    ui->label->resize(800, 450);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::connection_to_the_server(io_context &ioc, ip::tcp::resolver &resolver)
{
    p2p_worker = std::make_shared<p2p>(ioc, resolver, this);
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

void MainWindow::on_pb_share_clicked()
{
    QTimer *timer = new QTimer(this);
    connect(timer, &QTimer::timeout, this, &MainWindow::captureScreen);
    timer->start(1000); // Обновление каждые 100 мс
}

// Метод для обновления изображения из байтов (например, PNG, JPEG)
void MainWindow::update_screen(const QByteArray &image_data)
{
    QImage image;
    if (image.loadFromData(image_data)) {
        // Преобразуем QImage → QPixmap и устанавливаем в QLabel
        ui->label->setPixmap(QPixmap::fromImage(image));
    } else {
        qWarning("Ошибка: не удалось загрузить изображение из данных.");
    }
}
