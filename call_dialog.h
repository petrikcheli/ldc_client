#ifndef CALL_DIALOG_H
#define CALL_DIALOG_H

#include <QDialog>
#include <QLabel>

#include <iostream>
#include <string>
#include <thread>

#include "p2p_connection.h"
#include "websocket_client.h"
#include "screen_streamer.h"

namespace Ui {
class call_dialog;
}

class call_dialog : public QDialog
{
    Q_OBJECT

public:
    //explicit call_dialog(QWidget *parent = nullptr);
    explicit call_dialog(
        std::shared_ptr<P2PConnection> p2p_worker,
        std::shared_ptr<WebSocketClient> ws_woker,
        std::shared_ptr<ScreenStreamer> screen_streamer,
        QWidget *parent = nullptr);
    ~call_dialog();
    json msg_candidate;
    json msg_offer;
    //QLabel* get_screen_label(){return this->ui->label;}
    //std::string peer_id;
    //rtc::Description offer;
    void end_call();

private slots:
    void on_pb_reject_clicked();

    void on_pb_accept_clicked();

private:
    Ui::call_dialog *ui;
    std::shared_ptr<P2PConnection> p2p_worker;
    std::shared_ptr<WebSocketClient> ws_woker;
    std::shared_ptr<ScreenStreamer> screen_streamer;


};

#endif // CALL_DIALOG_H
