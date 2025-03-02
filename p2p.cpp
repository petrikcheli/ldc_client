#include "p2p.h"
#include <QDebug>
#include <thread>

p2p::p2p()
{

}

p2p::p2p( io_context& ioc, ip::tcp::resolver& resolver )
{
    try {
        ws_ptr = std::make_shared<websocket::stream<ip::tcp::socket>>( ioc );
        connect_to_websocket_server(resolver);
    }
    catch ( std::exception& e ) {
        qDebug() << "Error: " << e.what();
    }
}

void p2p::connect_to_websocket_server(ip::tcp::resolver& resolver)
{
        auto const results = resolver.resolve( "95.183.12.120", "9001" );
        connect(ws_ptr->next_layer(), results.begin(), results.end());

        ws_ptr->handshake( "95.183.12.120", "/" );
        qDebug() << "Connected to the Server";

}

void p2p::write_to_websocket(json msg)
{
    if( ws_ptr ){
        ws_ptr->write( buffer( msg.dump() ) );
    }
}

void p2p::receive_loop_from_server()
{
    while( true ) {
        try {
            flat_buffer buffer;
            ws_ptr->read(buffer);

            std::string received = buffers_to_string(buffer.data());
            json msg = json::parse(received);

            std::string sender = msg["sender"];
            std::string type = msg["type"];

            if( type == "answer" ) {
                peer_connection->setRemoteDescription(rtc::Description(msg["sdp"], "answer"));
                qDebug() << "SDP Answer down established";
            }
            else if( type == "candidate" ) {
                rtc::Candidate candidate(msg["candidate"], msg["sdpMid"]);
                peer_connection->addRemoteCandidate(candidate);
                qDebug() << "ice-candidate added";
            }
            else if( type == "offer" ) {
                std::string candidate = msg["sdp"];
                rtc::Description offer(candidate, rtc::Description::Type::Offer);
                //init_peer_connection();
                //init_data_channel();
                peer_connection->setRemoteDescription(offer);
            }
        }
        catch( std::exception& e ) {
            qDebug() << "Error in reading: " << e.what();
            break;
        }
    }
}

void p2p::init_data_channel()
{
    // 5. Обработчик открытия DataChannel
    data_channel->onOpen([this] {
        qDebug() << "DataChannel open!";
        data_channel->send("Hello ot " + self_id);
    });

    // 6. Обработчик получения сообщений
    data_channel->onMessage([](rtc::message_variant msg) {
        if (std::holds_alternative<std::string>(msg)) {
            std::cout << "Message received: " << std::get<std::string>(msg) << "\n";
        }
    });
}

void p2p::init_peer_connection()
{
    config.iceServers.push_back( {"stun:stun.l.google.com:19302"} );

    // Создаем PeerConnection
    peer_connection = std::make_shared<rtc::PeerConnection>( config );

    // Обработчик локального SDP (это будет Answer)
    peer_connection->onLocalDescription( [this]( rtc::Description desc ) {
        qDebug() << "send " << desc.typeString() << " SDP";
        on_local_description(desc);
    });

    // Обработчик ICE-кандидатов
    peer_connection->onLocalCandidate( [this]( rtc::Candidate candidate ) {
        if( candidate.type() == rtc::Candidate::Type::ServerReflexive ){
            qDebug() << "send ice " << std::string( candidate );
            on_ice_candidate( candidate );
        }
    });

    peer_connection->onStateChange( [](rtc::PeerConnection::State state ) {
        if (state == rtc::PeerConnection::State::Connected) {
            qDebug() << "PeerConnection accept!\n";
        }
    });

    peer_connection->onDataChannel( [this](std::shared_ptr<rtc::DataChannel> dc ) {
        qDebug() << "DataChannel receive!\n";
        // Сохраняем в глобальную переменную
        data_channel = dc;

        // Обрабатываем получение сообщений
        data_channel->onMessage( [](rtc::message_variant msg ) {
            if (std::holds_alternative<std::string>(msg)) {
                std::cout << "Recived message: " << std::get<std::string>(msg) << "\n";
            }
        });
    });
}

void p2p::on_ice_candidate( rtc::Candidate candidate )
{
    json candidate_msg = {
        {"type", "candidate"},
        {"sender", self_id},
        {"target", peer_id},
        {"candidate", candidate.candidate()},
        {"sdpMid", candidate.mid()}
    };
    write_to_websocket( candidate_msg );
    qDebug() << "Send to ice candidate";
}
void p2p::on_local_description( rtc::Description desc )
{
    json sdp_msg = {
        {"type", desc.typeString()},
        {"sender", self_id},
        {"target", peer_id},
        {"sdp", std::string( desc )}
    };
    write_to_websocket( sdp_msg );
    qDebug() << "Send " << desc.typeString();
}

void p2p::register_on_server()
{
    json register_msg = {
        {"type", "register"},
        {"sender", self_id}
    };
    write_to_websocket( register_msg );
    qDebug() << "Send to reg server";
    init_peer_connection();
    // init_data_channel();
}

void p2p::send_offer()
{
    init_peer_connection();
    data_channel = peer_connection->createDataChannel("chat");
    init_data_channel();
}

void p2p::send_data_p2p( std::string msg ){
    if(data_channel->isClosed()) return;
    data_channel->send( msg );
}
