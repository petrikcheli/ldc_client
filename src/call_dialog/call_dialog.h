#ifndef CALL_DIALOG_H
#define CALL_DIALOG_H

#include <QDialog>
#include <QLabel>

#include <iostream>
#include <string>
#include <thread>

//#include "../ui/mainwindow.h"
#include "../client/p2p_connection.h"
#include "../websocket_client/websocket_client.h"
#include "../screen_share/screen_streamer.h"
#include "../audio/audio.h"
#include "../video_camera/video_camera.h"

//TODO: правильно завершать все переменные, а также правильно их обновлять и инициализировать
//TODO: реализовать логику многопользовательского звонка: как минимум красиво выводить экраны

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
        std::shared_ptr<Audio> audio_worker,
        std::shared_ptr<Video_camera> video_camera_worker,
        QWidget *parent = nullptr);

    ~call_dialog();

    //json файлы для p2p соединения
    json msg_candidate;
    json msg_offer;
    json msg_answer;

    //заканчивает звонок
    void end_call();

    void on_remote_call_ended();

    void hide_ui_end_call();

    //начинает звонок
    void start_call();

    void on_datachannel_open();

private slots:
    //еще не реализовал
    void on_pb_reject_clicked();

    //метод который принимает звонок, убирает кнопки, ставит новые
    void on_pb_accept_clicked();

    //метод который отменяет звонок если человек перехотел звонить
    void onCancelclicked();

    void onShareScreenClicked();

    //включает и выключает видео
    void onShareClicked();

    //включает и выключает звук
    void onShareAudioClicked();

    //заканчивает звонок звонок
    void onEndCallClicked();

private:
    Ui::call_dialog *ui;
    std::shared_ptr<P2PConnection> p2p_worker;
    std::shared_ptr<WebSocketClient> ws_woker;
    std::shared_ptr<ScreenStreamer> screen_streamer;
    std::shared_ptr<Audio> audio_worker;
    std::shared_ptr<Video_camera> video_camera_worker;


    QPushButton *pb_share_screen;

    QPushButton *pb_share;

    QPushButton *pb_audio_share;

    QPushButton *pb_endCall;

    QPushButton *pb_cancelCall;

};

#endif // CALL_DIALOG_H
