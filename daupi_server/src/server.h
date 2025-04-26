#pragma once

#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <vector>

using namespace boost::asio;

class Server : public std::enable_shared_from_this<Server>
{
private:
    Server();

    std::vector<ip::tcp::socket> _clients;
    ip::tcp::endpoint _ep_server{ip::address::from_string("127.0.0.1"), 8989};
    ip::tcp::socket _sock_server;
    ip::tcp::acceptor _acceptor;
public:
    //это для тестов
    //std::vector<unsigned char> buff;

public:
    Server(io_context& io_context) : _sock_server(io_context), _acceptor(io_context){
        _acceptor.open(_ep_server.protocol());
        _acceptor.bind(_ep_server);
        _acceptor.listen();
        this->start_accept(io_context);
        //так же для тестов
        //buff.resize(100);
    };
    void start_accept(io_context& io_context);

    void read_socket(std::shared_ptr<ip::tcp::socket> socket);

    void write_socket(std::shared_ptr<ip::tcp::socket> socket, std::shared_ptr<std::vector<unsigned char>> buff);
};

