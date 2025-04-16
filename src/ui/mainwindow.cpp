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

    api_worker_ = new User_api();
    login = new Login(api_worker_);

    // Инициализация ws_client_
    ws_client_ = std::make_shared<WebSocketClient>(
        ioc,
        std::bind(&MainWindow::handle_message, this, std::placeholders::_1)
        );

    connect(login, &Login::loginSuccessful, this, &MainWindow::onLoginSuccessful);
    login->show();


}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::onLoginSuccessful()
{
    this->set_username();
    // Инициализация screen_streamer_
    screen_streamer_ = std::make_shared<ScreenStreamer>(
        [this](const QByteArray &data) {
            if (p2p_worker_) {
                p2p_worker_->send_video_screen_p2p(data);
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

    video_camera_worker_ = make_shared<Video_camera>();

    encoderThread = new QThread;

    video_camera_worker_->moveToThread(encoderThread);

    QObject::connect(encoderThread, &QThread::started, video_camera_worker_.get(), &Video_camera::start);
    //QObject::connect(encoderThread, &QThread::finished, video_camera_worker_.get(), &QObject::deleteLater);

    video_camera_worker_->signalVideoCaptured.connect(
        [this](std::queue<std::shared_ptr<std::vector<uint8_t>>>& q_video) {
            p2p_worker_->send_video_camera_frame_p2p(q_video);}
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
        },
        [this](const std::shared_ptr<std::vector<uint8_t>>& buffer) {
            if(video_camera_worker_) {
                video_camera_worker_->handle_received_packet(buffer);
            }
        },
        [this](es_p2p signal){
            this->handle_p2p_signal(signal);
        }
        );

    call_offer = new call_dialog(p2p_worker_, ws_client_, screen_streamer_, audio_worker_, video_camera_worker_, this);

    connect(this, &MainWindow::show_call_offer, this, &MainWindow::on_showCallOffer);
    connect(this, &MainWindow::show_call_answer, this, &MainWindow::on_showCallAnswer);
    this->init_ui_mainwindow();
    this->show();
}

void MainWindow::set_username()
{
    this->self_username = login->username;

    json register_msg = {
        {"type", WebSocketClient::type_str[ews_type::REGISTER]},
        {"sender", self_username}
    };
    ws_client_->send_message( register_msg );
    qDebug() << "Send to reg server";
}

/*
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
*/
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

void MainWindow::init_ui_mainwindow()
{
    // Центральный виджет
    centralWidget = new QWidget(this);
    setCentralWidget(centralWidget);

    // Главный горизонтальный layout
    mainLayout = new QHBoxLayout(centralWidget);

    // ЛЕВАЯ ПАНЕЛЬ
    leftPanel = new QWidget();
    leftLayout = new QVBoxLayout(leftPanel);

    settingsButton = new QPushButton("Настройки");
    settingsButton->setFixedHeight(30);

    friendsList = new QListWidget(); // список друзей

    leftLayout->addWidget(settingsButton);
    leftLayout->addWidget(friendsList);
    leftPanel->setLayout(leftLayout);
    leftPanel->setFixedWidth(200);

    // ПРАВАЯ ПАНЕЛЬ
    rightPanel = new QWidget();
    rightLayout = new QVBoxLayout(rightPanel);

    chatLabel = new QLabel("Выберите друга для чата");
    chatLabel->setAlignment(Qt::AlignCenter);

    chatHistory = new QTextBrowser();
    messageInput = new QLineEdit();
    sendButton = new QPushButton("Отправить");
    callButton = new QPushButton("Позвонить");

    messageLayout = new QHBoxLayout();
    messageLayout->addWidget(messageInput);
    messageLayout->addWidget(sendButton);

    rightLayout->addWidget(chatLabel);
    rightLayout->addWidget(chatHistory);
    rightLayout->addLayout(messageLayout);
    rightLayout->addWidget(callButton);

    // Сборка главного layout
    mainLayout->addWidget(leftPanel);
    mainLayout->addWidget(rightPanel);

    // Пример подключения
    connect(sendButton, &QPushButton::clicked, this, &MainWindow::sendMessage);
    connect(friendsList, &QListWidget::itemClicked, this, &MainWindow::friendSelected);
    connect(callButton, &QPushButton::clicked, this, &MainWindow::startCall);
    connect(settingsButton, &QPushButton::clicked, this, &MainWindow::openSettings);

    this->init_dialogs();

    QString qss = R"(
                QWidget {
                    background-color: #202B38;
                    color: #FFFFFF;
                    font-family: "Segoe UI", sans-serif;
                    font-size: 14px;
                }

                QPushButton {
                    background-color: #2A3749;
                    border: none;
                    border-radius: 8px;
                    padding: 8px 16px;
                    color: white;
                }

                QPushButton:hover {
                    background-color: #3E4E63;
                }

                QPushButton:pressed {
                    background-color: #1B2633;
                }

                QListWidget {
                    background-color: #1B2633;
                    border: none;
                    border-radius: 8px;
                    padding: 4px;
                }

                QListWidget::item {
                    padding: 10px;
                    border-bottom: 1px solid #2E3C4E;
                }

                QListWidget::item:selected {
                    background-color: #324156;
                    border-radius: 4px;
                }

                QLineEdit {
                    background-color: #2A3749;
                    border: 1px solid #3A4A5C;
                    border-radius: 6px;
                    padding: 6px;
                    color: white;
                }

                QTextBrowser {
                    background-color: #1B2633;
                    border: none;
                    border-radius: 8px;
                    padding: 8px;
                    color: white;
                }
                )";
    this->setStyleSheet(qss);
}

void MainWindow::init_dialogs()
{
    auto frends = api_worker_->get_friends();
    for (const auto& user : frends) {
        this->friendsList->addItem(QString::fromStdString(user["username"]));
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

void MainWindow::sendMessage()
{
    QString text = messageInput->text();
    if (text.isEmpty() || peer_username == "")
        return;

    bool success = api_worker_->send_message(text.toStdString(), peer_username);
    if (success) {
        // добавим в интерфейс
        //QListWidgetItem* item = new QListWidgetItem("Вы: " + text);
        chatHistory->append("Вы: " + text);
        messageInput->clear();
    }
    //api_worker_->send_message(, peer_username)
}

void MainWindow::friendSelected(QListWidgetItem *item)
{
    auto friend_name = item->text();
    peer_username = item->text().toStdString();

    chatLabel->setText("Диалог с " + friend_name);

    // Загружаем историю сообщений (пока просто пример)
    chatHistory->clear();
    auto msg_json = api_worker_->get_messages_with_user(peer_username);
    for (const auto& msg : msg_json) {
        QString sender = QString::fromStdString(msg["sender_username"]);
        QString content = QString::fromStdString(msg["content"]);
        QString fullText = sender + ": " + content;

        chatHistory->append(fullText);
    }
    //chatHistory->append("<b>" + friend_name + ":</b> Привет!");
    //chatHistory->append("<b>Вы:</b> Привет, как дела?");
}

void MainWindow::startCall()
{
    //set_peer_id(ui->peer_id->text().toStdString());
    p2p_worker_->create_offer(self_username, peer_username);
    emit show_call_answer();
    call_offer->start_call();
}

void MainWindow::openSettings()
{

}

