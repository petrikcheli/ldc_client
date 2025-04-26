#include "client.h"
#include "../Audio/Audio_parametrs.h"
#include <iostream>

void Client::connect_to_server(const ip::tcp::resolver::results_type& endpoints, int channel_id) {
    //boost::asio::ip::tcp::no_delay option(true);
    //_socket_server.set_option(option);
    boost::asio::async_connect(_socket_server, endpoints,
    [this, endpoints](const boost::system::error_code& error, const ip::tcp::endpoint&) {
        if (!error) {
            std::cout << "Connected to server." << std::endl;
            _socket_server.async_send(channel_id,
              [this](const boost::system::error_code& error, std::size_t bytes_transferred) {
                  handle_write(error, bytes_transferred);
              });
            this->receive_response_video();
        } else {
            std::cout << "Failed connect to server" << std::endl;
        }

        //this->connect_to_server(endpoints);
    });
}

void Client::handle_write(const boost::system::error_code& error, std::size_t bytes_transferred) {
    if (!error) {
        std::cout << "done message!! " << bytes_transferred << " byte\n";
    } else {
        std::cerr << "error send message : " << error.message() << "\n";
    }
}

void Client::send_audio(std::queue<std::shared_ptr<std::vector<unsigned char>>>& q_voice) {
    std::vector<unsigned char> data_send = *q_voice.front();

    _socket_server.async_send(boost::asio::buffer(data_send),
    [this, &q_voice](const boost::system::error_code& error, std::size_t bytes_transferred) {
        q_voice.pop();
        handle_write(error, bytes_transferred);
    });
}

void Client::send_video(std::queue<std::shared_ptr<std::vector<uint8_t>>>& q_voice)
{
    std::vector<uint8_t> data_send = *q_voice.front();

    if(data_send.empty()) return;
    _socket_server.async_send(boost::asio::buffer(data_send),
    [this, &q_voice](const boost::system::error_code& error, std::size_t bytes_transferred) {
        q_voice.pop();
        handle_write(error, bytes_transferred);
    });
}
//127.0.0.1

// void Client::receive_response() {
//     auto buffer = std::make_shared<std::vector<unsigned char>>(daupi::FRAMES_PER_BUFFER*daupi::CHANNELS*sizeof(float));
//     _socket_server.async_receive(boost::asio::buffer(*buffer),
//     [this, buffer](const boost::system::error_code& error, std::size_t bytes_transferred){
//         if(!error){
//             buffer->resize(bytes_transferred);
//             this->signal_voice_arrived(buffer);
//             this->receive_response();
//         } else {
//             std::cout << "error receive message" << std::endl;
//         }
//     });
// }

void Client::receive_response_video() {
    auto buffer = std::make_shared<std::vector<uint8_t>>(150000); // Предположим размер буфера 4096 байт

    _socket_server.async_receive(boost::asio::buffer(*buffer),
                                 [this, buffer](const boost::system::error_code& error, std::size_t bytes_transferred) {
                                     if (!error) {
                                         buffer->resize(bytes_transferred);
                                         if(buffer->at(0) == 0x01){

                                             this->signal_video_arrived(buffer);
                                             std::cout << "receive message " << std::endl;

                                             this->receive_response_video();
                                         } else if(buffer->at(0) == 0x02){
                                             this->signal_voice_arrived(buffer);
                                             this->receive_response();
                                         } else {
                                             std::cout << "it's not video or audio frame" << std::endl;
                                         }
                                     } else {
                                         std::cout << "Error receiving message: " << error.message() << std::endl;
                                     }
                                 }
                                 );
}
