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
                p2p_worker_->send_video_frame_p2p(data);
            }
        },
        this
    );

    audio_worker_ = make_shared<Audio>();

    audio_worker_->signalAudioCaptured.connect(
        [this](std::queue<std::shared_ptr<std::vector<unsigned char>>>& q_voice) {
            p2p_worker_->send_audio_frame_p2p(q_voice);
        }
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
        },
        [this](std::shared_ptr<std::vector<unsigned char>> frame) {
            if(audio_worker_) {
                audio_worker_->decoded_voice(frame);
            }
        }
    );

    call_offer = new call_dialog(p2p_worker_, ws_client_, screen_streamer_, audio_worker_, this);

    connect(this, &MainWindow::show_call_offer, this, &MainWindow::on_showCallOffer);
    connect(this, &MainWindow::show_call_answer, this, &MainWindow::on_showCallAnswer);

}

MainWindow::~MainWindow()
{
    delete ui;
}


void MainWindow::on_pb_reg_clicked()
{
    set_self_id(ui->self_id->text().toStdString());

    json register_msg = {
        {"type", WebSocketClient::type_str[ews_type::REGISTER]},
        {"sender", self_id}
    };
    ws_client_->send_message( register_msg );
    qDebug() << "Send to reg server";
}

void MainWindow::on_pd_offer_clicked()
{
    set_peer_id(ui->peer_id->text().toStdString());
    p2p_worker_->create_offer(self_id, peer_id);
    emit show_call_answer();
    call_offer->start_call();
    //emit show_call_offer(QString::fromStdString(msg.dump()));
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

void MainWindow::on_showCallAnswer()
{
    call_offer->show();
}

void MainWindow::handle_message(const nlohmann::json &msg)
{
    //std::string received = buffers_to_string(buffer.data());
    //json msg = json::parse(data);
    //qDebug() << "messege receive";
    std::string sender = msg["sender"];
    std::string type = msg["type"];

    if( type == WebSocketClient::type_str[ews_type::ANSWER]) {
        //peer_connection->setRemoteDescription(rtc::Description(msg["sdp"], "answer"));
        call_offer->msg_answer = msg;
        std::string candidate = msg["sdp"];
        rtc::Description offer(candidate, rtc::Description::Type::Answer);

        p2p_worker_->setRemoteDescription(offer, msg["type"]);
        std::cout << "remote sdp: " << candidate << std::endl;
        qDebug() << "SDP Answer down established";

        if(flag_candidate_send == true){
            p2p_worker_->addRemoteCandidate(this->candidate, sdpMid);
            //std::cout << "remote sdp: " << msg["candidate"] << std::endl;
            qDebug() << "ice-candidate added";
            flag_candidate_send = false;
        }
        flag_description_send = true;
    }
    else if( type == WebSocketClient::type_str[ews_type::CANDIDATE_ANSWER]){
        if(flag_description_send == false){
            candidate = msg["candidate"];
            sdpMid = msg["sdpMid"];
            flag_candidate_send = true;
            return;
        }
        p2p_worker_->addRemoteCandidate(msg["candidate"], msg["sdpMid"]);
        std::cout << "remote sdp: " << msg["candidate"] << std::endl;
        qDebug() << "ice-candidate added";
        flag_description_send = false;
    }
    else if( type == WebSocketClient::type_str[ews_type::CANDIDATE] ) {
        call_offer->msg_candidate = msg;
        qDebug() << "ice-candidate added";
    }
    else if( type == WebSocketClient::type_str[ews_type::OFFER] ) {
        qDebug() << "offer added";
        emit show_call_offer(QString::fromStdString(msg.dump()));
    }
    else if( type == WebSocketClient::type_str[ews_type::END_CALL]){
        qDebug() << "end call";
        call_offer->on_remote_call_ended();
    }
}

void MainWindow::handle_p2p_signal(es_p2p signal)
{
    switch (signal) {
    case es_p2p::OPEN_DATACHANNEL:
        call_offer->on_datachannel_open();
        break;
    default:
        break;
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

