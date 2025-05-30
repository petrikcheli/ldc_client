#include "websocket_client.h"
#include <QDebug>
//WebSocketClient::WebSocketClient() {}

#define HOST "95.183.12.120"
#define PORT "9001"

// Инициализация вне класса
const std::string WebSocketClient::type_str[20] = {
    "register",
    "offer",
    "answer",
    "candidate",
    "candidate_answer",
    "end_call",
    "end_share",
    "message",
    "unknown",
    "trash"
};

WebSocketClient::WebSocketClient( io_context &ioc, const std::function<void(const nlohmann::json &)> &func )
{
    try{
        ws_ = std::make_shared<websocket::stream<ip::tcp::socket>>( ioc );
        resolver_ = std::make_shared<ip::tcp::resolver>( ioc );
        set_message_handler(func);
        connect( HOST, PORT );
    } catch( std::exception &e ){
        qDebug() << "Error: " << e.what();
    }
}

void WebSocketClient::connect( const std::string &host, const std::string &port )
{
    auto const results = resolver_->resolve( host, port );
    boost::asio::connect( ws_->next_layer(), results.begin(), results.end() );
    ws_->handshake( host, "/" );

    qDebug() << "Connected to the Server";

    receive_loop();

    qDebug() << "receive loop";
}

void WebSocketClient::send_message( const nlohmann::json &msg )
{
    if( ws_ ){
        ws_->write( boost::asio::buffer( msg.dump() ) );
    }
}

void WebSocketClient::send_rt_message(const std::string &sender, const std::string &target, const std::string &msg)
{
    json result_msg = {
        {"type", WebSocketClient::type_str[WebSocketClient::Etype_message::MESSAGE]},
        {"sender", sender},
        {"target", target},
        {WebSocketClient::type_str[WebSocketClient::Etype_message::MESSAGE], msg}
    };
    send_message(result_msg);
}

void WebSocketClient::receive_loop()
{
    auto buffer_read = std::make_shared<flat_buffer>();
    ws_->async_read(*buffer_read,
    [this, buffer_read](boost::system::error_code ec, std::size_t bytes_transferred)
    {
        if(!ec){
            std::string received = buffers_to_string(buffer_read->data());
            json msg = json::parse(received);

            if(on_message_){
                std::thread thread_send([this, msg](){this->on_message_(msg);});
                thread_send.detach();
                //on_message_(msg);
            }

            std::string type = msg["type"];
            //qDebug() << "ws recv type: " << type;
            receive_loop();
            // if(type != type_str[END_CALL]){

            // }
        } else {
            qDebug() << "Error in reading: " << ec.what();
            json msg_ec = {
                {"type", "error"},
                {"sender", ec.what()}
            };
            std::thread thread_send([this, msg_ec](){this->on_message_(msg_ec);});
            thread_send.detach();
            //on_message_(msg);
            receive_loop();
        }
    });
}


