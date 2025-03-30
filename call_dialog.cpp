#include "call_dialog.h"
#include "ui_call_dialog.h"

call_dialog::call_dialog(
    std::shared_ptr<P2PConnection> p2p_worker,
    std::shared_ptr<WebSocketClient> ws_woker,
    std::shared_ptr<ScreenStreamer> screen_streamer,
    QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::call_dialog)
{
    ui->setupUi(this);

    ui->label->setScaledContents(true);
    ui->label->setMaximumSize(800, 450);
    //ui->label->resize(800, 450);

    this->p2p_worker = p2p_worker;
    this->ws_woker = ws_woker;
    this->screen_streamer = screen_streamer;
    screen_streamer->set_label(ui->label);
}

call_dialog::~call_dialog()
{
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

    ui->label->show();

    ui->pb_accept->hide();
    ui->pb_reject->hide();


    QPushButton *share = new QPushButton("включить демонстрацию");
    share->setObjectName("share_button");

    QPushButton *endCall = new QPushButton("завершить звонок");
    endCall->setObjectName("endCall_button");

    ui->gridLayout->addWidget(share);
    ui->gridLayout->addWidget(endCall);

    // Соединяем сигналы и слоты
    connect(share, &QPushButton::clicked, this, &call_dialog::onShareClicked);
    connect(endCall, &QPushButton::clicked, this, &call_dialog::onEndCallClicked);

    //мне необходимо создать peer_connection
    //отправить данные, чтобы собеседник мог подключитьться
    //мне необходимо открыть трансляцию
}

void call_dialog::onShareClicked()
{
    if( !screen_streamer->is_shared() ){
        qDebug() << "share on";
        screen_streamer->start();
    } else {
        qDebug() << "share off";
        screen_streamer->stop();
    }
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
    hide_ui_end_call();
}

void call_dialog::hide_ui_end_call()
{
    QPushButton *share = findChild<QPushButton *>("share_button");
    QPushButton *endCall = findChild<QPushButton *>("endCall_button");

    if (share) {
        ui->gridLayout->removeWidget(share);
        delete share;
    }

    if (endCall) {
        ui->gridLayout->removeWidget(endCall);
        delete endCall;
    }

    //ui->label->hide();

    ui->pb_accept->show();
    ui->pb_reject->show();

    this->hide();
}

void call_dialog::start_call()
{
    // //this->show();

    // ui->label->hide();

    // ui->pb_accept->hide();
    // ui->pb_reject->hide();

    // QPushButton *cancel = new QPushButton("отменить звонок");
    // cancel->setObjectName("cancel");

    // ui->gridLayout->addWidget(cancel);

    ui->label->show();

    ui->pb_accept->hide();
    ui->pb_reject->hide();


    QPushButton *share = new QPushButton("включить демонстрацию");
    share->setObjectName("share_button");

    QPushButton *endCall = new QPushButton("завершить звонок");
    endCall->setObjectName("endCall_button");

    ui->gridLayout->addWidget(share);
    ui->gridLayout->addWidget(endCall);

    // Соединяем сигналы и слоты
    connect(share, &QPushButton::clicked, this, &call_dialog::onShareClicked);
    connect(endCall, &QPushButton::clicked, this, &call_dialog::onEndCallClicked);
}

