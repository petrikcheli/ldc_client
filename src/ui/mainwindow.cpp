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
    else if( type == WebSocketClient::type_str[ews_type::MESSAGE]){
        qDebug() << "recv message";
        add_message_dialog(sender, msg[WebSocketClient::type_str[ews_type::MESSAGE]]);
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
    central_widget = new QWidget(this);
    setCentralWidget(central_widget);

    // Главный горизонтальный layout
    main_layout = new QHBoxLayout(central_widget);

    // ЛЕВАЯ ПАНЕЛЬ
    left_panel = new QWidget();
    left_layout = new QVBoxLayout(left_panel);

    settings_button = new QPushButton("Настройки");
    settings_button->setFixedHeight(30);

    friends_list = new QListWidget(); // список друзей

    // Кнопки Друзья / Каналы
    friends_button = new QPushButton("Друзья");
    channels_button = new QPushButton("Каналы");

    QHBoxLayout *switchLayout = new QHBoxLayout();
    switchLayout->addWidget(friends_button);
    switchLayout->addWidget(channels_button);

    left_layout->addLayout(switchLayout);
    left_layout->addWidget(settings_button);
    left_layout->addWidget(friends_list);

    // Подключаем сигналы
    connect(friends_button, &QPushButton::clicked, this, &MainWindow::show_friends);
    connect(channels_button, &QPushButton::clicked, this, &MainWindow::show_channels);

    // Стартовый режим
    current_mode = Mode::Friends;
    left_panel->setLayout(left_layout);
    left_panel->setFixedWidth(200);

    // ПРАВАЯ ПАНЕЛЬ
    right_panel = new QWidget();
    right_layout = new QVBoxLayout(right_panel);

    chat_label = new QLabel("Выберите друга для чата");
    chat_label->setAlignment(Qt::AlignCenter);

    chat_history = new QTextBrowser();
    message_lnput = new QLineEdit();
    send_button = new QPushButton("Отправить");
    call_button = new QPushButton("Позвонить");

    message_layout = new QHBoxLayout();
    message_layout->addWidget(message_lnput);
    message_layout->addWidget(send_button);

    right_layout->addWidget(chat_label);
    right_layout->addWidget(chat_history);
    right_layout->addLayout(message_layout);
    right_layout->addWidget(call_button);

    // Сборка главного layout
    main_layout->addWidget(left_panel);
    main_layout->addWidget(right_panel);

    // Пример подключения
    connect(send_button, &QPushButton::clicked, this, &MainWindow::send_message);
    connect(friends_list, &QListWidget::itemClicked, this, &MainWindow::friend_selected);
    connect(call_button, &QPushButton::clicked, this, &MainWindow::start_call);
    connect(settings_button, &QPushButton::clicked, this, &MainWindow::open_settings);

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
        this->friends_list->addItem(QString::fromStdString(user["username"]));
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

void MainWindow::send_message()
{
    QString text = message_lnput->text();
    if (text.isEmpty() || peer_username == "")
        return;

    bool success = api_worker_->send_message(text.toStdString(), peer_username);
    if (success) {
        // добавим в интерфейс
        //QListWidgetItem* item = new QListWidgetItem("Вы: " + text);
        ws_client_->send_rt_message(self_username, peer_username, text.toStdString());
        chat_history->append("Вы: " + text);
        message_lnput->clear();
    }
    //api_worker_->send_message(, peer_username)
}

void MainWindow::add_message_dialog(const std::string &sender_username, const std::string &content)
{

    // QString q_sender = QString::fromStdString(sender_username);
    // QString q_content = QString::fromStdString(content);
    // QString fullText = q_sender + ": " + q_content;

    // chatHistory->append(fullText);
    if (peer_username == sender_username) {
        QString msgText = QString::fromStdString(sender_username + ": " + content);
        message_histories[sender_username].append(msgText);

        chat_history->append(msgText);
    } else if(self_username == sender_username){
        QString msgText = QString::fromStdString("Вы: " + content);
        chat_history->append(msgText);
    }
    else {
        // Уведомление: можешь добавить иконку, подсветку, popup и т.п.
        QList<QListWidgetItem*> items = friends_list->findItems(QString::fromStdString(sender_username), Qt::MatchExactly);
        if (!items.isEmpty()) {
            items[0]->setBackground(Qt::yellow); // Простой визуальный сигнал
        }
    }
}

void MainWindow::friend_selected(QListWidgetItem *item)
{
    // auto friend_name = item->text();
    // peer_username = item->text().toStdString();

    // chat_label->setText("Диалог с " + friend_name);

    // chat_history->clear();
    // auto msg_json = api_worker_->get_messages_with_user(peer_username);
    // for (const auto& msg : msg_json) {
    //     add_message_dialog(msg["sender_username"], msg["content"]);
    // }
    // item->setBackground(Qt::white);
    //chatHistory->append("<b>" + friend_name + ":</b> Привет!");
    //chatHistory->append("<b>Вы:</b> Привет, как дела?");

    if (current_mode == Mode::Friends) {
        auto friend_name = item->text();
        peer_username = friend_name.toStdString();

        chat_label->setText("Диалог с " + friend_name);
        chat_history->clear();

        auto msg_json = api_worker_->get_messages_with_user(peer_username);
        for (const auto& msg : msg_json) {
            add_message_dialog(msg["sender_username"], msg["content"]);
        }

        item->setBackground(Qt::white);
    }
    else if (current_mode == Mode::Channels) {
        QString channelName = item->text();
        chat_label->setText("Канал: " + channelName);
        chat_history->clear();

        // Найти ID канала по названию
        auto channels = api_worker_->get_channels();
        for (const auto& channel : channels) {
            if (QString::fromStdString(channel["name"]) == channelName) {
                selectedChannelId = channel["id"];
                break;
            }
        }

        // Теперь загружаем подканалы
        friends_list->clear();
        auto subchannels = api_worker_->get_subchannels(selectedChannelId);
        for (const auto& sub : subchannels) {
            QString type = QString::fromStdString(sub["type"]);
            QString name = QString::fromStdString(sub["name"]);

            if (type == "text") {
                friends_list->addItem("💬 " + name);
            } else if (type == "voice") {
                friends_list->addItem("🎤 " + name);
            }
        }

        // Подписать что это подканалы:
        chat_label->setText("Выберите подканал в " + channelName);
    }

}
//TODO: Test sending RTP packets in track
void MainWindow::start_call()
{
    //set_peer_id(ui->peer_id->text().toStdString());
    p2p_worker_->create_offer(self_username, peer_username);
    emit show_call_answer();
    call_offer->start_call();
}

void MainWindow::open_settings()
{

}

void MainWindow::show_friends()
{
    current_mode = Mode::Friends;
    friends_list->clear();

    auto friends = api_worker_->get_friends();
    for (const auto& user : friends) {
        friends_list->addItem(QString::fromStdString(user["username"]));
    }
    chat_label->setText("Выберите друга для чата");
}

void MainWindow::show_channels()
{
    current_mode = Mode::Channels;
    friends_list->clear();

    auto channels = api_worker_->get_channels(); // <--- тебе нужно будет сделать метод в api_worker_
    for (const auto& channel : channels) {
        friends_list->addItem(QString::fromStdString(channel["name"]));
    }
    chat_label->setText("Выберите канал");
}
