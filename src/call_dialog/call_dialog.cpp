#include "call_dialog.h"
#include "ui_call_dialog.h"

call_dialog::call_dialog(
    std::shared_ptr<P2PConnection> p2p_worker,
    std::shared_ptr<WebSocketClient> ws_woker,
    std::shared_ptr<ScreenStreamer> screen_streamer,
    std::shared_ptr<Audio> audio_worker,
    std::shared_ptr<Video_camera> video_camera_worker,
    QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::call_dialog)
{
    ui->setupUi(this);


    // ui->label_self_share->setScaledContents(true);
    // ui->label_peer_share->setScaledContents(true);
    // ui->label->setScaledContents(true);
    // ui->label_2->setScaledContents(true);

    // ui->label_self_share->setMaximumSize(ui->label_self_share->height(), ui->label_self_share->width());
    // ui->label_peer_share->setMaximumSize(ui->label_peer_share->height(), ui->label_peer_share->width());
    // ui->label->setMaximumSize(ui->label->height(), ui->label->width());
    // ui->label_2->setMaximumSize(ui->label_2->height(), ui->label_2->width());


    this->p2p_worker = p2p_worker;
    this->ws_woker = ws_woker;
    this->screen_streamer = screen_streamer;
    this->audio_worker = audio_worker;
    this->video_camera_worker = video_camera_worker;

    screen_streamer->set_labels(ui->label_self_share, ui->label_peer_share);

    video_camera_worker->set_lables(ui->label, ui->label_2);

    pb_share_screen = new QPushButton("включить демонстрацию");

    pb_share = new QPushButton("включить видео");
    //pb_share->setObjectName("share_button");

    pb_audio_share = new QPushButton("включить звук");
    //pb_share->setObjectName("audio_share_button");

    pb_endCall = new QPushButton("завершить звонок");
    //pb_endCall->setObjectName("endCall_button");

    pb_cancelCall = new QPushButton("отменить");

    // Соединяем сигналы и слоты
    connect(pb_share_screen, &QPushButton::clicked, this, &call_dialog::onShareScreenClicked);
    connect(pb_share, &QPushButton::clicked, this, &call_dialog::onShareClicked);
    connect(pb_endCall, &QPushButton::clicked, this, &call_dialog::onEndCallClicked);
    connect(pb_audio_share, &QPushButton::clicked, this, &call_dialog::onShareAudioClicked);
    connect(pb_cancelCall, &QPushButton::clicked, this, &call_dialog::onCancelclicked);


    ui->gridLayout->addWidget(pb_share_screen);
    ui->gridLayout->addWidget(pb_share);
    ui->gridLayout->addWidget(pb_endCall);
    ui->gridLayout->addWidget(pb_audio_share);
    ui->gridLayout->addWidget(pb_cancelCall);


    pb_endCall->hide();
    pb_audio_share->hide();
    pb_share_screen->hide();
    pb_share->hide();
    pb_cancelCall->hide();
}

call_dialog::~call_dialog()
{
    delete pb_share_screen;
    delete pb_share;
    delete pb_endCall;
    delete pb_audio_share;
    delete pb_cancelCall;
    delete ui;
}



void call_dialog::on_pb_reject_clicked()
{
    this->hide();
}


void call_dialog::on_pb_accept_clicked()
{
    std::string peer_id = msg_offer["sender"];
    std::string candidate = msg_offer["sdp"];
    rtc::Description offer(candidate, rtc::Description::Type::Offer);
    p2p_worker->setRemoteDescription(offer, msg_offer["type"], peer_id);

    rtc::Candidate c_candidate(msg_candidate["candidate"], msg_candidate["sdpMid"]);
    p2p_worker->addRemoteCandidate(msg_candidate["candidate"], msg_candidate["sdpMid"]);

    video_camera_worker->init_decoder();

    // тут сделать ожидание подключения
}

void call_dialog::onCancelclicked(){
    hide_ui_end_call();
    //отправить чуваку то, что мы отменили звонок
}

void call_dialog::onShareScreenClicked()
{
    if( !screen_streamer->is_shared() ){
        qDebug() << "share on";
        screen_streamer->start();
    } else {
        qDebug() << "share off";
        screen_streamer->stop();
    }
}

void call_dialog::on_datachannel_open(){


    if(pb_cancelCall->isVisible()) pb_cancelCall->hide();
    ui->pb_accept->hide();
    ui->pb_reject->hide();

    if(!pb_cancelCall->isVisible())ui->label->show();
    pb_endCall->show();
    pb_audio_share->show();
    pb_share->show();
    pb_share_screen->show();


    //мне необходимо создать peer_connection
    //отправить данные, чтобы собеседник мог подключитьться
    //мне необходимо открыть трансляцию
    audio_worker->open_out_stream();

}

void call_dialog::onShareClicked()
{
    // if( !screen_streamer->is_shared() ){
    //     qDebug() << "share on";
    //     screen_streamer->start();
    // } else {
    //     qDebug() << "share off";
    //     screen_streamer->stop();
    // }
    if( !video_camera_worker->is_shared() ){
        qDebug() << "share on";
        video_camera_worker->start();
    } else {
        qDebug() << "share off";
        video_camera_worker->stop();
    }
}

//BUG: аудио начинает работать только если два раза нажать на включить звук

void call_dialog::onShareAudioClicked()
{
    if( !audio_worker->is_open_in()){
        qDebug() << "open share audio";
        audio_worker->open_in_stream();
        audio_worker->open_out_stream();
    }
    if( !audio_worker->is_shared()){
        qDebug() << "shared audio on";
        audio_worker->start_in_stream();
    } else {
        qDebug() << "shared audio off";
        audio_worker->stop_in_stream();
    }
    // qDebug() << "share audio on";
    // audio_worker->open_in_stream();
    // audio_worker->open_out_stream();
}

void call_dialog::onEndCallClicked()
{
    end_call();
}

void call_dialog::end_call()
{
    // если datachannel даже не открыт то будет ошибка
    //qDebug() << "end call";
    p2p_worker->close_p2p(true);
    if(screen_streamer->is_shared()){
        qDebug() << "screen stop";
        screen_streamer->stop();
    }

    hide_ui_end_call();
}

void call_dialog::on_remote_call_ended()
{
    p2p_worker->close_p2p(false);
    if(screen_streamer->is_shared()){
        qDebug() << "screen stop";
        screen_streamer->stop();
    }
    //раскоментрирую когда сделаю этот функционал

    if(audio_worker->is_shared()){
        qDebug() << "auido stop";
        //audio_worker->stop();
    }

    hide_ui_end_call();
}

void call_dialog::hide_ui_end_call()
{
    pb_endCall->hide();
    pb_audio_share->hide();
    pb_share->hide();
    pb_share_screen->hide();

    ui->pb_accept->show();
    ui->pb_reject->show();

    this->hide();
}

void call_dialog::start_call()
{
    ui->label->show();

    ui->pb_accept->hide();
    ui->pb_reject->hide();

    pb_cancelCall->show();

    //pb_endCall->show();
    //pb_audio_share->show();
    //pb_share->show();

    // тут тоже нужно будет сделать ожидание
}

