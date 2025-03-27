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

}


void call_dialog::on_pb_accept_clicked()
{
    std::string peer_id = msg_offer["sender"];
    std::string candidate = msg_offer["sdp"];
    rtc::Description offer(candidate, rtc::Description::Type::Offer);
    p2p_worker->setRemoteDescription(offer, msg_offer["type"], peer_id);

    //rtc::Candidate c_candidate(msg_candidate["candidate"], msg_candidate["sdpMid"]);
    p2p_worker->addRemoteCandidate(msg_candidate["candidate"], msg_candidate["sdpMid"]);

    ui->pb_accept->hide();
    ui->pb_reject->hide();
    //мне необходимо создать peer_connection
    //отправить данные, чтобы собеседник мог подключитьться
    //мне необходимо открыть трансляцию
}

void call_dialog::end_call()
{
    p2p_worker->close_p2p();
    this->hide();
}

