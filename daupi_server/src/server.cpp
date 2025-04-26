#include "server.h"
#include <iostream>
#include <memory>

void Server::start_accept(io_context& io_context){
    std::shared_ptr<ip::tcp::socket> socket = std::make_shared<ip::tcp::socket>(io_context);
    boost::system::error_code err;
    _acceptor.async_accept(*socket, [this, &io_context, socket](const boost::system::error_code& error) {
        if (!error) {
            std::cout << "Client connected!" << std::endl;
            read_socket(socket); // Start reading from the client
        }
        //start_accept(io_context);
    });
}

void Server::read_socket(std::shared_ptr<ip::tcp::socket> socket){
    //auto buff = std::make_shared<std::vector<unsigned char>>(960);
    auto buff = std::make_shared<std::vector<uint8_t>>(50000);
    socket->async_receive(boost::asio::buffer(*buff),
                          [this, socket, buff](const boost::system::error_code& error, std::size_t size_receive_buff){
                              if (!error) {
                                  std::cout << "server receive : " << size_receive_buff << std::endl;
                                  buff->resize(size_receive_buff);
                                  this->write_socket(socket, buff);
                              } else {
                                  //std::cout << "error server receive message" << std::endl;
                              }
                              this->read_socket(socket);
                          });

}

void Server::write_socket(std::shared_ptr<ip::tcp::socket> socket, std::shared_ptr<std::vector<uint8_t>> buff){
    socket->async_send(boost::asio::buffer(*buff, buff->size()),
                             [this, socket](const boost::system::error_code& error, std::size_t) {
                                 if (!error) {
                                     std::cout << "Server write message" << std::endl;
                                 } else {
                                     std::cout << "send frame" << std::endl;
                                 }
                             });
}
