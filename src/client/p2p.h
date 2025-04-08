#ifndef P2P_H
#define P2P_H

#include <boost/asio.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>
#include <string>
#include <nlohmann/json.hpp>
#include <rtc/rtc.hpp>
#include <QByteArray>

class MainWindow;

using namespace boost::asio;
using namespace boost::beast;
using json = nlohmann::json;
namespace websocket = boost::beast::websocket;

class p2p
{
public:
    p2p();
    p2p(io_context& ioc, ip::tcp::resolver& resolver, MainWindow *window);
    void register_on_server();
    void set_self_id(std::string id){this->self_id = id;}
    void set_peer_id(std::string id){this->peer_id = id;}
    void send_offer();
    void receive_loop_from_server();

    void send_data_p2p(std::string msg);
    void send_data_p2p(rtc::message_variant data);
    void send_data_p2p(const char* data);
    void send_data_p2p(QByteArray data);
    void send_data_p2p(int data);

private:
    void connect_to_websocket_server(ip::tcp::resolver& resolver);

    void write_to_websocket(json msg);


    void init_data_channel();
    void init_peer_connection();

    void on_ice_candidate(rtc::Candidate candidate);
    void on_local_description(rtc::Description desc);



    std::string get_self_id(){return this->self_id;}

private:
    std::shared_ptr<rtc::PeerConnection> peer_connection;
    std::shared_ptr<rtc::DataChannel> data_channel;
    std::shared_ptr<websocket::stream<ip::tcp::socket>> ws_ptr;
    rtc::Configuration config;
    std::string self_id;
    std::string peer_id;

private:
    MainWindow *main_window;
};

#endif // P2P_H
