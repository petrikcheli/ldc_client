#include "p2p_connection.h"
#include "../websocket_client/websocket_client.h"
#include <QDebug>
#include <thread>
#include <chrono>
#include "../audio/Audio_parametrs.h"



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

std::shared_ptr<std::vector<unsigned char>> vector_to_vec_UnC_ptr(const rtc::binary& vec)
{
    auto vec_UnC_ptr = std::make_shared<std::vector<unsigned char>>();
    vec_UnC_ptr->resize(vec.size());
    for(size_t i = 0; i < vec.size(); ++i){
        vec_UnC_ptr->at(i) = static_cast<unsigned char>(vec[i]);
    }

    return vec_UnC_ptr;
}

std::shared_ptr<std::vector<uint8_t>> convert_binary_to_uint8_ptr(const rtc::binary& binary) {
    auto result = std::make_shared<std::vector<uint8_t>>();
    result->reserve(binary.size());
    std::transform(binary.begin(), binary.end(), std::back_inserter(*result),
                   [](std::byte b) { return static_cast<uint8_t>(b); });
    return result;
}

std::shared_ptr<std::vector<unsigned char>> P2PConnection::sh_ptr_to_vec_UnC_ptr(rtc::message_ptr vec)
{
    auto vec_UnC_ptr = std::make_shared<std::vector<unsigned char>>();
    vec_UnC_ptr->resize(vec->size());
    for(size_t i = 0; i < vec->size(); ++i){
        vec_UnC_ptr->at(i) = static_cast<unsigned char>(vec->at(i));
    }

    return vec_UnC_ptr;
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
                             std::function<void (const QByteArray &)> on_share_data,
                             std::function<void(std::shared_ptr<std::vector<unsigned char>>)> on_share_audio_frame,
                             std::function<void(const std::shared_ptr<std::vector<uint8_t>>)> on_share_video_frame,
                             std::function<void(es_p2p)>send_signal)
{
    rtc::InitLogger(rtc::LogLevel::Debug);
    this->send_p2p_data_on_server = send_p2p_data_on_server;
    this->on_share_data = on_share_data;
    this->on_share_audio_frame = on_share_audio_frame;
    this->on_share_video_frame = on_share_video_frame;
    this->send_signal = send_signal;
}

void P2PConnection::create_offer(const std::string &self_id_, const std::string &peer_id_)
{
    this->self_id_ = self_id_;
    this->peer_id_ = peer_id_;
    init(WebSocketClient::type_str[ews_type::CANDIDATE]);
    //сюда нужно добавить data_channel_->open()
    //data_channel_.reset();
//media.setBitrate(2000000); // сделать const значение для этого

    rtc::Description::Video media("video-stream", rtc::Description::Direction::SendRecv);
    media.addH264Codec(102);

    media.addSSRC(1, "video-stream", "stream1", "video-stream");

    video_track_ = peer_connection_->addTrack(media);
    //pc, payloadType, ssrc, cname         ,  msid,
    //pc, 102        , 1   , "video-stream", "stream1"

    // create RTP configuration
    auto rtpConfigM = std::make_shared<rtc::RtpPacketizationConfig>(1, "video-stream", 102, rtc::H264RtpPacketizer::ClockRate);
    // create packetizer
    auto packetizerM = std::make_shared<rtc::H264RtpPacketizer>(rtc::NalUnit::Separator::LongStartSequence, rtpConfigM, 50000);
    // add RTCP SR handler
    auto srReporterM = std::make_shared<rtc::RtcpSrReporter>(rtpConfigM);
    packetizerM->addToChain(srReporterM);
    // add RTCP NACK handler
    auto nackResponderM = std::make_shared<rtc::RtcpNackResponder>();
    packetizerM->addToChain(nackResponderM);
    // set handler
    video_track_->setMediaHandler(packetizerM);

    init_video_track();


    auto audio = rtc::Description::Audio("audio-stream", rtc::Description::Direction::SendRecv);
    audio.addOpusCodec(111);
    audio.addSSRC(2, "audio-stream", "stream1", "audio-stream");

    audio_track_ = peer_connection_->addTrack(audio);
    // create RTP configuration
    auto rtpConfig = std::make_shared<rtc::RtpPacketizationConfig>(2, "audio-stream",
                                                              111, daupi::SAMPLE_RATE);
    // create packetizer
    auto packetizer = make_shared<rtc::OpusRtpPacketizer>(rtpConfig);
    // add RTCP SR handler
    auto srReporter = make_shared<rtc::RtcpSrReporter>(rtpConfig);
    packetizer->addToChain(srReporter);
    // add RTCP NACK handler
    auto nackResponder = std::make_shared<rtc::RtcpNackResponder>();
    packetizer->addToChain(nackResponder);
    // set handler
    audio_track_->setMediaHandler(packetizer);
    //track->onOpen(onOpen);
    //auto trackData = std::make_shared<rtc::ClientTrackData>(track, srReporter);
    //return trackData;

   //rtc::Description::Audio audio("audio_test", rtc::Description::Direction::SendOnly);
    // audio.addOpusCodec(111);
    // audio.addSSRC(123456, "track1");
    // audio_track_ = peer_connection_->addTrack(audio);
    init_audio_track();

    data_channel_ = peer_connection_->createDataChannel("chat");
    //video_track_ = peer_connection_->addTrack(rtc::Description::Video{"VP8"});


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
    config_.iceServers.push_back( {"stun:stun2.l.google.com:19302"} );

    // Создаем PeerConnection
    peer_connection_ = std::make_shared<rtc::PeerConnection>( config_ );

    // Обработчик локального SDP (это будет Answer)
    peer_connection_->onLocalDescription( [this]( rtc::Description desc ) {

        //desc.addVideo();
        json sdp_msg = {
            {"type", desc.typeString() == "offer" ?
                    WebSocketClient::type_str[ews_type::OFFER] : WebSocketClient::type_str[ews_type::ANSWER]},
            {"sender", self_id_},
            {"target", peer_id_},
            {"sdp", std::string( desc )}
        };
        send_p2p_data_on_server(sdp_msg);
        //std::cout << "Локальный SDP:\n" << std::string(desc) << std::endl;
        qDebug() << "send " << desc.typeString() << " SDP";
    });

    // Обработчик ICE-кандидатов
    peer_connection_->onLocalCandidate( [this, candidate_type]( rtc::Candidate candidate ) {
        //if( candidate.type() == rtc::Candidate::Type::ServerReflexive ){
        if(candidate.type() == rtc::Candidate::Type::Host){
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

    peer_connection_->onStateChange( [this](rtc::PeerConnection::State state ) {
        if (state == rtc::PeerConnection::State::Connected) {
            qDebug() << "PeerConnection accept!";

            send_signal(es_p2p::OPEN_DATACHANNEL);
        }
        qDebug() << "PeerConnection state changed: " << static_cast<int>(state);
    });

    peer_connection_->onGatheringStateChange([](rtc::PeerConnection::GatheringState state) {
        qDebug() << "Gathering state changed: " << static_cast<int>(state);
    });

    peer_connection_->onDataChannel( [this](std::shared_ptr<rtc::DataChannel> dc ) {
        qDebug() << "DataChannel receive!\n";
        // Сохраняем в глобальную переменную
        data_channel_ = dc;

        // Обрабатываем получение сообщений
        data_channel_->onMessage( [this](rtc::message_variant msg ) {
            if (std::holds_alternative<std::string>(msg)) {
                //pass
                //qDebug() << "maybe is frame";
            } else {
                if(std::to_integer<uint8_t>(std::get<rtc::binary>(msg).at(0)) == 0x02){
                    rtc::binary h264_payload(std::get<rtc::binary>(msg).begin() + 1,
                                             std::get<rtc::binary>(msg).end());
                    on_share_video_frame(convert_binary_to_uint8_ptr(h264_payload));
                } else if (std::to_integer<uint8_t>(std::get<rtc::binary>(msg).at(0)) == 0x01){
                    // std::cout << "frame recv" << std::endl;
                    std::cout << "recv screen frame" << std::get<rtc::binary>(msg).size() << " byte" << std::endl;

                    on_share_data(vectorToQByteArray(rtc::binary ( std::get<rtc::binary>(msg).begin() + 1,
                                                                 std::get<rtc::binary>(msg).end())));
                }
                //qDebug() << "maybe is frame";
                //on_share_data(vectorToQByteArray(std::get<rtc::binary>(msg)));
                //main_window->update_screen(vectorToQByteArray(std::get<rtc::binary>(msg)));
            }
        });
    });


    // peer_connection_->onTrack([this](std::shared_ptr<rtc::Track> track) {
    //     qDebug() << "Track receive!";

    //     if(track->mid() == "audio_test") return

    //     video_track_ = track;

    //     // Подключаем обработку фреймов
    //     video_track_->onFrame([this](rtc::binary data, rtc::FrameInfo frame) {
    //         std::cout << "frame recv" << std::endl;
    //         //std::cout << "Получен фрейм размером " << data.size() << " байт" << std::endl;
    //         on_share_data(vectorToQByteArray(data));
    //         // Здесь можно отправить фрейм в декодер или обработчик
    //     });
    // });


    peer_connection_->onTrack([this](std::shared_ptr<rtc::Track> track) {
        qDebug() << "track receive!";
        //std::string test = track->mid();
        if(track->mid() == "audio-stream"){
            audio_track_ = track;

            // create RTP configuration
            auto rtpConfig = std::make_shared<rtc::RtpPacketizationConfig>(2, "audio-stream",
                                                                           111, daupi::SAMPLE_RATE);
            // create packetizer
            auto packetizer = make_shared<rtc::OpusRtpPacketizer>(rtpConfig);
            // add RTCP SR handler
            auto srReporter = make_shared<rtc::RtcpSrReporter>(rtpConfig);
            packetizer->addToChain(srReporter);
            // add RTCP NACK handler
            auto nackResponder = std::make_shared<rtc::RtcpNackResponder>();
            packetizer->addToChain(nackResponder);
            // set handler
            audio_track_->setMediaHandler(packetizer);

            audio_track_->onMessage( [this](rtc::message_variant msg ) {
                if (std::holds_alternative<std::string>(msg)) {
                    //pass
                    //on_share_audio_frame()
                    qDebug() << "maybe is frame str";
                } else {
                    // Создаем depacketizer
                    // auto depacketizer = std::make_shared<rtc::OpusRtpDepacketizer>();

                    // depacketizer->incoming(std::get<rtc::message_vector>(msg), [this](rtc::message_ptr msg){
                    //     on_share_audio_frame(sh_ptr_to_vec_UnC_ptr(msg));
                    // });
                    std::cout << "done frame " << std::get<rtc::binary>(msg).size() << "byte" <<  std::endl;
                    //auto test_vec = vector_to_vec_UnC_ptr(std::get<rtc::binary>(msg));
                    const size_t rtp_header_size = 12;
                    // Извлечем полезную нагрузку (OPUS)
                    rtc::binary opus_payload(std::get<rtc::binary>(msg).begin() + rtp_header_size,
                                             std::get<rtc::binary>(msg).end());

                    on_share_audio_frame(vector_to_vec_UnC_ptr(opus_payload));
                    //qDebug() << "maybe is frame choto drygot";
                    //on_share_data(vectorToQByteArray(std::get<rtc::binary>(msg)));
                    //main_window->update_screen(vectorToQByteArray(std::get<rtc::binary>(msg)));
                }
            });

            // Подключаем обработку фреймов
            audio_track_->onFrame([this](rtc::binary data, rtc::FrameInfo frame) {
                std::cout << "Получен фрейм размером " << data.size() << " байт" << std::endl;
                on_share_data(vectorToQByteArray(data));
                // Здесь можно отправить фрейм в декодер или обработчик
            });

            // audio_track_ = track;



            // // Подписываемся на готовые Opus-фреймы
            // depacketizer->onFrame([this](rtc::binary opusFrame) {
            //     // Тут ты вызываешь свою функцию декодирования Opus
            //     on_share_audio_frame(vector_to_vec_UnC_ptr(opusFrame));
            // });

            // // Назначаем обработчик медиа для трека
            // audio_track_->setMediaHandler(depacketizer);

        } else {
            qDebug() << "video track recive";
            video_track_ = track;

            video_track_->onMessage( [this](rtc::message_variant msg) {
                if (std::holds_alternative<std::string>(msg)) {
                    //pass
                    //on_share_audio_frame()
                    qDebug() << "maybe is frame str";
                } else {
                    // Создаем depacketizer
                    auto depacketizer = std::make_shared<rtc::H264RtpDepacketizer>();

                //     depacketizer->incoming(std::get<rtc::message_vector>(msg), [this, msg](rtc::message_ptr msgsg){
                //         std::cout << "done video frame " << std::get<rtc::binary>(msg).size() << "byte" <<  std::endl;
                //         //auto test_vec = vector_to_vec_UnC_ptr(std::get<rtc::binary>(msg));

                //         const size_t rtp_header_size = 12;
                //         // Извлечем полезную нагрузку (OPUS)
                //         rtc::binary opus_payload(std::get<rtc::binary>(msg).begin() + rtp_header_size,
                //                                  std::get<rtc::binary>(msg).end());
                //         on_share_video_frame(convert_binary_to_uint8_ptr(opus_payload));

                //         qDebug() << "maybe is frame choto drygot";
                //         on_share_data(vectorToQByteArray(std::get<rtc::binary>(msg)));
                //      });
                }
            });

            // Подключаем обработку фреймов
            video_track_->onFrame([this](rtc::binary data, rtc::FrameInfo frame) {
                std::cout << "frame recv" << std::endl;
                //std::cout << "Получен фрейм размером " << data.size() << " байт" << std::endl;
                on_share_data(vectorToQByteArray(data));
                // Здесь можно отправить фрейм в декодер или обработчик
            });
        }
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
            //qDebug() << "maybe is frame";
        } else {
            if(std::to_integer<uint8_t>(std::get<rtc::binary>(msg).at(0)) == 0x02){
                rtc::binary h264_payload(std::get<rtc::binary>(msg).begin() + 1,
                                         std::get<rtc::binary>(msg).end());
                on_share_video_frame(convert_binary_to_uint8_ptr(h264_payload));
            } else if (std::to_integer<uint8_t>(std::get<rtc::binary>(msg).at(0)) == 0x01){
                // std::cout << "frame recv" << std::endl;
                std::cout << "recv screen frame" << std::get<rtc::binary>(msg).size() << " byte" << std::endl;

                on_share_data(vectorToQByteArray(rtc::binary ( std::get<rtc::binary>(msg).begin() + 1,
                                                             std::get<rtc::binary>(msg).end())));
            }
            //qDebug() << "maybe is frame";
            //on_share_data(vectorToQByteArray(std::get<rtc::binary>(msg)));
            //main_window->update_screen(vectorToQByteArray(std::get<rtc::binary>(msg)));
        }
    });
}

void P2PConnection::init_video_track()
{
    video_track_->onOpen([this](){
        if (video_track_->isOpen()) {
            qDebug() << "Track open" ;
        } else {
            qDebug() << "track not open" ;
        }
    });

    // Подключаем обработку фреймов
    video_track_->onFrame([this](rtc::binary data, rtc::FrameInfo frame) {
        std::cout << "frame recv" << std::endl;

        on_share_data(vectorToQByteArray(data));
        //on_share_audio_frame(vector_to_vec_UnC_ptr(data));
        //audio->decoded_voice(ar);
    });
}

void P2PConnection::init_audio_track()
{
    video_track_->onOpen([this](){
        if (video_track_->isOpen()) {
            qDebug() << "Audio track open" ;
        } else {
            qDebug() << "audio track not open" ;
        }
    });

    // Подключаем обработку фреймов
    video_track_->onFrame([this](rtc::binary data, rtc::FrameInfo frame) {
        std::cout << "frame audio recv" << std::endl;

        //on_share_data(vectorToQByteArray(data));
        on_share_audio_frame(vector_to_vec_UnC_ptr(data));
        //audio->decoded_voice(ar);
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

void P2PConnection::send_video_screen_p2p(const QByteArray &data)
{
    if(data_channel_->isClosed()) return;
    rtc::byte* charArray = new rtc::byte[data.size()+1];
    memcpy(charArray+1, data.constData(), data.size());
    charArray[0] = static_cast<std::byte>(0x01);
    data_channel_->send( charArray, data.size() );
    delete[] charArray;
}

void P2PConnection::send_video_camera_frame_p2p(std::queue<std::shared_ptr<std::vector<uint8_t>>>& q_video)
{
    //if(video_track_->isClosed()) return;
    qDebug() << "send video frame";
    try {
        std::vector<unsigned char> data_send = *q_video.front();

        rtc::byte* charArray = new rtc::byte[data_send.size()+1];

        memcpy(charArray+1, data_send.data(), data_send.size());
        charArray[0] = static_cast<std::byte>(0x02);
        data_channel_->send(charArray, data_send.size());
        //video_track_->sendFrame(charArray, data_send.size(), rtc::FrameInfo(0));

        delete[] charArray;

        q_video.pop();
    } catch (std::exception e) {
        std::cout << e.what() << std::endl;
    }

}

// void P2PConnection::send_video_screen_frame_p2p(std::queue<std::shared_ptr<std::vector<uint8_t>>>& q_video)
// {
//     //if(video_track_->isClosed()) return;
//     qDebug() << "send video frame";
//     try {
//         std::vector<unsigned char> data_send = *q_video.front();

//         rtc::byte* charArray = new rtc::byte[data_send.size()+1];

//         memcpy(charArray+1, data_send.data(), data_send.size());
//         charArray[0] = static_cast<std::byte>(0x02);
//         data_channel_->send(charArray, data_send.size());
//         //video_track_->sendFrame(charArray, data_send.size(), rtc::FrameInfo(0));

//         delete[] charArray;

//         q_video.pop();
//     } catch (std::exception e) {
//         std::cout << e.what() << std::endl;
//     }

// }

void P2PConnection::send_audio_frame_p2p(std::queue<std::shared_ptr<std::vector<unsigned char>>> &q_voice)
{
    qDebug() << "send audio frame";
    if(audio_track_->isClosed()) return;
    try {
        std::vector<unsigned char> data_send = *q_voice.front();
        rtc::byte* charArray = new rtc::byte[data_send.size()];
        memcpy(charArray, data_send.data(), data_send.size());
        audio_track_->sendFrame(charArray, data_send.size(), rtc::FrameInfo(0));
        delete[] charArray;
        q_voice.pop();
    } catch (std::exception e) {
        std::cout << e.what() << std::endl;
    }


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
