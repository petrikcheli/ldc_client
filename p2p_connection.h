#ifndef P2P_CONNECTION_H
#define P2P_CONNECTION_H

#include <memory>
#include <rtc/rtc.hpp>
#include <string>
#include <nlohmann/json.hpp>
#include <QByteArray>

using json = nlohmann::json;

class P2PConnection
{
public:
    // инициализирует ссылки на функции
    P2PConnection(std::function<void(const nlohmann::json &)> send_p2p_data_on_server,
                  std::function<void(const QByteArray &)> on_share_data);

    // инициализирует peer_connection_ data_channel_ self и peer id
    // и начинает процесс отправки offer
    void create_offer( const std::string &self_id_, const std::string &peer_id_ );

    // устанавливает sdp от собеседника
    void setRemoteDescription(const std::string &sdp,
                              const std::string &type,
                              const std::string &peer_id);

    // устанавливает Answer::adp от собесендника
    void setRemoteDescription(const std::string &sdp,
                                             const std::string &type);

    // устанавливает candidate от собеседника
    void addRemoteCandidate(const std::string &candidate_msg, const std::string &sdpMid);

    //ссылка на функцию которая отправляет данные на сервер
    //будет использоваться метод из websocket_client (send_message(const nlohmann::json &))
    std::function<void(const nlohmann::json &)> send_p2p_data_on_server;

    //ссылка на функцию которая отправляет данные для трансляции экрана
    //будет использоваться метод из screen_streamer (update_frame())
    std::function<void(const QByteArray &)> on_share_data;

    //std::function<void(const rtc::Description &)> on_local_description;

    //std::function<void(const rtc::Candidate &)> on_ice_candidate;

    //отправка различных данных через p2p
    void send_data_p2p(std::string msg);
    void send_data_p2p(rtc::message_variant data);
    void send_data_p2p(const char* data);
    void send_data_p2p(int data);

    //отправка трансляции экрана собеседнику
    //этот метод пойдет в screen_streamer
    void send_frame_p2p(const QByteArray &data);

    void send_video_frame_p2p(const QByteArray &data);

    void close_p2p(bool send_end_call);

    void set_peer_id(const std::string &peer_id) {this->peer_id_ = peer_id;}

private:
    //id пользователя
    std::string self_id_;

    //id для собеседника
    std::string peer_id_;

    //объект rtc::PeerConnection
    std::shared_ptr<rtc::PeerConnection> peer_connection_;

    //объект rtc::PeerConnection
    std::shared_ptr<rtc::DataChannel> data_channel_;

    //объект rtc::Track
    std::shared_ptr<rtc::Track> track_;

    std::shared_ptr<rtc::Track> video_track_;

    //объект который хранит конфиг от stun
    rtc::Configuration config_;

    //инициализирует peer_connection_
    void init(const std::string &candidate_type);

    //инициализирует data_channel
    void init_data_channel();

    void init_video_track();

};

#endif // P2P_CONNECTION_H
