#ifndef MAINWINDOW_H
#define MAINWINDOW_H

//#include "p2p.h"

#include <QMainWindow>
#include <QApplication>
#include <QScreen>
#include <QPixmap>
#include <QLabel>
#include <QTimer>
#include <QBuffer>
#include <QListWidget>
#include <QTextBrowser>
#include <QLineEdit>

#include <iostream>
#include <string>
#include <thread>
//#include <memory>

#include <boost/asio.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>

#include <nlohmann/json.hpp>
//#include <rtc/rtc.hpp

#include "../client/p2p_connection.h"
#include "../websocket_client/websocket_client.h"
#include "../screen_share/screen_streamer.h"
#include "../audio/audio.h"
#include "../audio/Audio_parametrs.h"
#include "../call_dialog/call_dialog.h"
#include "../enums/es_p2p.h"
#include "../video_camera/video_camera.h"
#include "../login_ui/login.h"
#include "../user_api/user_api.h"
//#include "p2p_connection.h"
//#include "websocket_client.h"
//#include "screen_streamer.h"

//#include "audio/audio.h"


using namespace boost::asio;
using namespace boost::beast;
using namespace std;
using namespace enums;
using json = nlohmann::json;
using ews_type = WebSocketClient::Etype_message;

namespace websocket = boost::beast::websocket;

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(io_context &ioc, ip::tcp::resolver &resolver, QWidget *parent = nullptr);
    ~MainWindow();

    //void connection_to_the_server(io_context &ioc, ip::tcp::resolver &resolver);
    //std::shared_ptr<p2p> p2p_worker;
    //void update_screen(const QByteArray &image_data);

private slots:
    void onLoginSuccessful();

    //void on_pb_reg_clicked();
    //void on_pd_offer_clicked();
    //void on_pb_share_clicked();
    void on_showCallOffer(const QString msg);
    void on_showCallAnswer();
    void on_pb_showCallOffer_clicked();

    void send_message();
    void friend_selected(QListWidgetItem *item);
    void start_call();
    void open_settings();
    void show_friends();
    void show_channels();

signals:
    void show_call_offer(const QString msg); // Сигнал для показа окна
    void show_call_answer();
    void send_candidate_signal(const QString msg);


private:
    void handle_message(const nlohmann::json &msg);

    void handle_p2p_signal(es_p2p signal);

    //void set_self_id(const std::string &self_id){this->self_id = self_id;}
    //void set_peer_id(const std::string &peer_id){this->peer_id = peer_id;}
    void set_username();

    void init_ui_mainwindow();

    void init_dialogs();

    void add_message_dialog(const std::string &sender_username, const std::string &content);


private:
    Ui::MainWindow *ui;

    std::shared_ptr<WebSocketClient> ws_client_;
    std::shared_ptr<P2PConnection> p2p_worker_;
    std::shared_ptr<ScreenStreamer> screen_streamer_;
    std::shared_ptr<Audio> audio_worker_;
    std::shared_ptr<Video_camera> video_camera_worker_;

    call_dialog *call_offer;
    Login *login;
    User_api *api_worker_;
    std::map<std::string, QStringList> message_histories;
    std::shared_ptr<std::thread> thread_call_offer;

    //std::string self_id;
    //std::string peer_id;

    std::string self_username;
    std::string peer_username;

    bool flag_description_send = false;
    bool flag_candidate_send = false;

    std::string candidate ;
    std::string sdpMid;

    QThread *encoderThread;

private:
    QWidget *central_widget;
    QHBoxLayout *main_layout;
    QWidget *left_panel;
    QVBoxLayout *left_layout;
    QPushButton *settings_button;
    QListWidget *friends_list;
    QWidget *right_panel;
    QVBoxLayout *right_layout;
    QLabel *chat_label;
    QTextBrowser *chat_history;
    QLineEdit *message_lnput;
    QPushButton *send_button;
    QPushButton *call_button;

    QHBoxLayout *message_layout;

    QPushButton *friends_button;
    QPushButton *channels_button;

    enum class Mode { Friends, Channels };
    Mode current_mode;

    int selectedChannelId;

};
#endif // MAINWINDOW_H

/*
public slots:
    void captureScreen() {
        QScreen *screen = QApplication::primaryScreen();
        if (screen) {
            QPixmap pixmap = screen->grabWindow(0);
            //setPixmap(pixmap);
            sendImage(pixmap);
        }
    }

    void sendImage(const QPixmap &pixmap) {
        QImage image = pixmap.toImage();
        QByteArray byteArray;
        QBuffer buffer(&byteArray);
        buffer.open(QIODevice::WriteOnly);
        image.save(&buffer, "JPEG", 10); // Сжатие до 50% качества

        int size = byteArray.size();
        p2p_worker->send_data_p2p(size);
        p2p_worker->send_data_p2p(byteArray); // Отправляем данные
        //socket->flush();
    }
*/
