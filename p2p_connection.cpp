#include "p2p_connection.h"
#include "websocket_client.h"
#include <QDebug>

using ews_type = WebSocketClient::Etype_message;

//P2PConnection::P2PConnection() {}

QByteArray vectorToQByteArray(const rtc::binary& vec)
{
    QByteArray byteArray;
    byteArray.resize(vec.size());
    for(size_t i = 0; i < vec.size(); ++i){
        byteArray[i] = static_cast<char>(vec[i]);
    }

    return byteArray;
}

// void P2PConnection::register_on_server()
// {
//     json register_msg = {
//         {"type", "register"},
//         {"sender", self_id}
//     };
//     write_to_websocket( register_msg );
//     qDebug() << "Send to reg server";
// }

P2PConnection::P2PConnection(std::function<void (const nlohmann::json &)> send_p2p_data_on_server,
                             std::function<void (const QByteArray &)> on_share_data)
{
    this->send_p2p_data_on_server = send_p2p_data_on_server;
    this->on_share_data = on_share_data;
}

void P2PConnection::create_offer(const std::string &self_id_, const std::string &peer_id_)
{
    this->self_id_ = self_id_;
    this->peer_id_ = peer_id_;
    init(WebSocketClient::type_str[ews_type::CANDIDATE]);
    //сюда нужно добавить data_channel_->open()
    //data_channel_.reset();
    data_channel_ = peer_connection_->createDataChannel("chat");
    video_track_ = peer_connection_->addTrack(rtc::Description::Video{"VP8"});
    init_data_channel();

}

void P2PConnection::setRemoteDescription(const std::string &sdp,
                                         const std::string &type)
{
    rtc::Description offer(sdp,
                           type == "offer" ? rtc::Description::Type::Offer : rtc::Description::Type::Answer);
    peer_connection_->setRemoteDescription(offer);
    qDebug() << "answer added";
}

void P2PConnection::setRemoteDescription(const std::string &sdp,
                                         const std::string &type,
                                         const std::string &peer_id)
{
    this->peer_id_ = peer_id;
    init(WebSocketClient::type_str[ews_type::CANDIDATE_ANSWER]);
    rtc::Description offer(sdp,
                           type == "offer" ? rtc::Description::Type::Offer : rtc::Description::Type::Answer);
    peer_connection_->setRemoteDescription(offer);
    qDebug() << "offer added";
}

void P2PConnection::addRemoteCandidate(const std::string &candidate_msg, const std::string &sdpMid)
{
    rtc::Candidate candidate( candidate_msg , sdpMid );
    peer_connection_->addRemoteCandidate(candidate);
    qDebug() << "ice-candidate added";
}

void P2PConnection::init(const std::string &candidate_type)
{
    config_.iceServers.push_back( {"stun:stun.l.google.com:19302"} );

    // Создаем PeerConnection
    peer_connection_ = std::make_shared<rtc::PeerConnection>( config_ );

    // Обработчик локального SDP (это будет Answer)
    peer_connection_->onLocalDescription( [this]( rtc::Description desc ) {

        json sdp_msg = {
            {"type", desc.typeString() == "offer" ?
                    WebSocketClient::type_str[ews_type::OFFER] : WebSocketClient::type_str[ews_type::ANSWER]},
            {"sender", self_id_},
            {"target", peer_id_},
            {"sdp", std::string( desc )}
        };
        send_p2p_data_on_server(sdp_msg);
        qDebug() << "send " << desc.typeString() << " SDP";
    });

    // Обработчик ICE-кандидатов
    peer_connection_->onLocalCandidate( [this, candidate_type]( rtc::Candidate candidate ) {
        if( candidate.type() == rtc::Candidate::Type::ServerReflexive ){
            json candidate_msg = {
                //{"type", WebSocketClient::type_str[ews_type::CANDIDATE]},
                {"type", candidate_type},
                {"sender", self_id_},
                {"target", peer_id_},
                {"candidate", candidate.candidate()},
                {"sdpMid", candidate.mid()}
            };
            send_p2p_data_on_server( candidate_msg );
            qDebug() << "send ice " << std::string( candidate );
        }
    });

    peer_connection_->onStateChange( [](rtc::PeerConnection::State state ) {
        if (state == rtc::PeerConnection::State::Connected) {
            qDebug() << "PeerConnection accept!\n";
        }
    });

    peer_connection_->onDataChannel( [this](std::shared_ptr<rtc::DataChannel> dc ) {
        qDebug() << "DataChannel receive!\n";
        // Сохраняем в глобальную переменную
        data_channel_ = dc;

        // Обрабатываем получение сообщений
        data_channel_->onMessage( [this](rtc::message_variant msg ) {
            if (std::holds_alternative<std::string>(msg)) {
                //pass
            } else {
                //on_share_data(vectorToQByteArray(std::get<rtc::binary>(msg)));
                //main_window->update_screen(vectorToQByteArray(std::get<rtc::binary>(msg)));
            }
        });
    });


    peer_connection_->onTrack([this](std::shared_ptr<rtc::Track> track) {
        qDebug() << "Track receive!";

        video_track_ = track;

        // Подключаем обработку фреймов
        video_track_->onFrame([this](rtc::binary data, rtc::FrameInfo frame) {
            //std::cout << "Получен фрейм размером " << data.size() << " байт" << std::endl;
            on_share_data(vectorToQByteArray(data));
            // Здесь можно отправить фрейм в декодер или обработчик
        });
    });
}

void P2PConnection::init_data_channel()
{
    // 5. Обработчик открытия DataChannel
    data_channel_->onOpen([this] {
        qDebug() << "DataChannel open!";
        data_channel_->send("Hello ot " + self_id_);
    });

    // 6. Обработчик получения сообщений
    data_channel_->onMessage([this](rtc::message_variant msg) {
        if (std::holds_alternative<std::string>(msg)) {
            //pass
        } else {
            on_share_data(vectorToQByteArray(std::get<rtc::binary>(msg)));
        }
    });
}

void P2PConnection::init_video_track()
{
    video_track_->onOpen([this]{
        qDebug() << "VideoTrack open!";
    });

    // Подключаем обработку фреймов
    video_track_->onFrame([this](rtc::binary data, rtc::FrameInfo frame) {
        //std::cout << "Получен фрейм размером " << data.size() << " байт" << std::endl;
        on_share_data(vectorToQByteArray(data));
        // Здесь можно отправить фрейм в декодер или обработчик
    });
}
void P2PConnection::send_data_p2p( std::string msg )
{
    if(data_channel_->isClosed()) return;
    data_channel_->send( msg );
}

void P2PConnection::send_data_p2p(rtc::message_variant data)
{
    if(data_channel_->isClosed()) return;
    data_channel_->send( data );
}

void P2PConnection::send_data_p2p(const char* data)
{
    if(data_channel_->isClosed()) return;
    data_channel_->send( data );
}

void P2PConnection::send_frame_p2p(const QByteArray &data)
{
    if(data_channel_->isClosed()) return;
    rtc::byte* charArray = new rtc::byte[data.size()];
    memcpy(charArray, data.constData(), data.size());
    data_channel_->send( charArray, data.size() );
    delete[] charArray;
}

void P2PConnection::send_video_frame_p2p(const QByteArray &data)
{
    if(video_track_->isClosed()) return;
    rtc::byte* charArray = new rtc::byte[data.size()];
    memcpy(charArray, data.constData(), data.size());
    video_track_->send( charArray, data.size() );
    delete[] charArray;
}

void P2PConnection::close_p2p(bool send_end_call)
{
    if( data_channel_ != nullptr){
        data_channel_->close();
    }
    peer_connection_->close();
    qDebug() << "send end_call";

    json end_msg = {
                    {"type", WebSocketClient::type_str[ews_type::END_CALL]},
                    {"sender", self_id_},
                    {"target", peer_id_},
                    };
    if( send_end_call ){
        send_p2p_data_on_server( end_msg );
    }
    // return;
}


void P2PConnection::send_data_p2p(int data)
{
    if(data_channel_->isClosed()) return;
    char byteArray[sizeof(int)];
    memcpy(byteArray, &data, sizeof(int));
    data_channel_->send( byteArray );
}
