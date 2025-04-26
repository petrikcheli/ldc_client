#ifndef WEBSOCKET_CLIENT_H
#define WEBSOCKET_CLIENT_H

#include <memory>
#include <boost/asio.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>
#include <string>
#include <nlohmann/json.hpp>

using namespace boost::asio;
using namespace boost::beast;
using json = nlohmann::json;
namespace websocket = boost::beast::websocket;

//TODO: сделать защиту

class WebSocketClient
{
public:
    // конструктор в котором инициализируются поля и происходит конект
    WebSocketClient(boost::asio::io_context& ioc, const std::function<void(const nlohmann::json &)> &func );

    // Метод для отправки сообщения на сервер
    void send_message( const nlohmann::json& msg );

    void send_rt_message(const std::string &sender, const std::string &target, const std::string &msg);

    enum Etype_message{
        REGISTER,

        OFFER,
        ANSWER,
        CANDIDATE,
        CANDIDATE_ANSWER,
        END_CALL,
        END_SHARE,
        MESSAGE,

        UNKNOWN,
        TRASH
    };

    const static std::string type_str[20];

private:
    //socket сервера
    std::shared_ptr<websocket::stream<ip::tcp::socket>> ws_;

    // ip и port сервера
    std::shared_ptr<ip::tcp::resolver> resolver_;

    // буфер сообщений
    flat_buffer buffer;

    // ссылка на функцию из другого метода которая обрабатывает сообещния
    // в данном случае обрабатывать должен mainwindow
    std::function<void( const nlohmann::json & )> on_message_;

    // пока не ассинхронное подключение
    void connect( const std::string& host, const std::string& port );

    // тут асинхронно принимаются сообщения от сервера
    void receive_loop();

    // Метод для установки функции-обработчика
    void set_message_handler( const std::function<void(const nlohmann::json &)> &func ){
        on_message_ = func;
    };
};

#endif // WEBSOCKET_CLIENT_H
