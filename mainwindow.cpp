#include "mainwindow.h"
#include "./ui_mainwindow.h"

//#include "p2p.h"
#include <QApplication>
#include <QImage>
#include <QPixmap>
#include <QLabel>
#include <QBuffer>
#include <QByteArray>
#include <QThread>

//std::bind(&MainWindow::send_p2p_data_on_server, this, std::placeholders::_1)

MainWindow::MainWindow( io_context &ioc, ip::tcp::resolver &resolver, QWidget *parent )
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    //this->regi

    ui->label->setScaledContents(true);
    ui->label->resize(800, 450);
    //ui->label->setMaximumSize(800, 450);

    // Инициализация ws_client_
    ws_client_ = std::make_shared<WebSocketClient>(
        ioc,
        std::bind(&MainWindow::handle_message, this, std::placeholders::_1)
    );

    // Инициализация screen_streamer_
    screen_streamer_ = std::make_shared<ScreenStreamer>(
        ui->label,
        [this](const QByteArray &data) {
            if (p2p_worker_) {
                p2p_worker_->send_frame_p2p(data);
            }
        },
        this
    );

    // Инициализация p2p_worker_
    p2p_worker_ = std::make_shared<P2PConnection>(
        [this](const json &msg) {
            if (ws_client_) {
                ws_client_->send_message(msg);
            }
        },
        [this](const QByteArray &frame) {
            if (screen_streamer_) {
                screen_streamer_->update_frame(frame);
            }
        }
    );

    call_offer = new call_dialog(p2p_worker_, ws_client_, screen_streamer_, this);

    connect(this, &MainWindow::show_call_offer, this, &MainWindow::on_showCallOffer);

}

MainWindow::~MainWindow()
{
    delete ui;
}


void MainWindow::on_pb_reg_clicked()
{
    //p2p_worker->set_self_id(ui->self_id->text().toStdString());
    // //p2p_worker->set_peer_id(ui->peer_id->text().toStdString());
    // p2p_worker->register_on_server();
}

void MainWindow::on_pd_offer_clicked()
{
    // //p2p_worker->set_self_id(ui->self_id->text().toStdString());
    // p2p_worker->set_peer_id(ui->peer_id->text().toStdString());
    // p2p_worker->send_offer();
    // //p2p_worker->register_on_server();
}

void MainWindow::on_pb_share_clicked()
{
    // QTimer *timer = new QTimer(this);
    // connect(timer, &QTimer::timeout, this, &MainWindow::captureScreen);
    // timer->start(1000); // Обновление каждые 100 мс
}

void MainWindow::on_showCallOffer(const QString msg)
{
    if (call_offer) {
        qDebug() << "open call_offer";
        json received_offer = nlohmann::json::parse(msg.toStdString());
        //json received_candidate = nlohmann::json::parse(msg_candidate.toStdString());
        call_offer->msg_offer = received_offer;
        //call_offer->msg_candidate = received_candidate;
        call_offer->show(); // Показать окно
    }
}

void MainWindow::handle_message(const nlohmann::json &msg)
{
    //std::string received = buffers_to_string(buffer.data());
    //json msg = json::parse(data);
    //qDebug() << "messege receive";
    std::string sender = msg["sender"];
    std::string type = msg["type"];

    if( type == "answer" ) {
        //peer_connection->setRemoteDescription(rtc::Description(msg["sdp"], "answer"));
        qDebug() << "SDP Answer down established";
    }
    else if( type == "candidate" ) {
        rtc::Candidate candidate(msg["candidate"], msg["sdpMid"]);
        call_offer->msg_candidate = msg;
        //peer_connection->addRemoteCandidate(candidate);
        qDebug() << "ice-candidate added";
    }
    else if( type == "offer" ) {
        qDebug() << "offer added";
        emit show_call_offer(QString::fromStdString(msg.dump()));
    }
    else if( type == "end_call"){
        qDebug() << "end call";
        call_offer->end_call();
    }
}



/*
void MainWindow::connection_to_the_server(io_context &ioc, ip::tcp::resolver &resolver)
{
    p2p_worker = std::make_shared<p2p>(ioc, resolver, this);
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
*/



void MainWindow::on_pb_showCallOffer_clicked()
{
    call_offer->show();
}

