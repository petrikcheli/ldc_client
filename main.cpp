
/*
#include <boost/asio.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>
#include <iostream>
#include <string>
#include <thread>
#include <nlohmann/json.hpp>
#include <rtc/rtc.hpp>

using namespace boost::asio;
using namespace boost::beast;
using namespace std;
using json = nlohmann::json;
namespace websocket = boost::beast::websocket;

const string client_id = "client_1";
std::shared_ptr<rtc::PeerConnection> pc1;

websocket::stream<ip::tcp::socket>* ws_ptr;

void send_message(json msg) {
    if (ws_ptr) {
        ws_ptr->write(buffer(msg.dump()));
    }
}

void on_ice_candidate(rtc::Candidate candidate) {
    json candidate_msg = {
        {"type", "candidate"},
        {"sender", client_id},
        {"target", "client_2"},
        {"candidate", candidate.candidate()},
        {"sdpMid", candidate.mid()}
    };
    send_message(candidate_msg);
    cout << "Send to ice candidate" << endl;
}

void on_local_description(rtc::Description desc) {
    json sdp_msg = {
        {"type", desc.typeString()},
        {"sender", client_id},
        {"target", "client_2"},
        {"sdp", string(desc)}
    };
    send_message(sdp_msg);
    cout << "Send " << desc.typeString() << endl;
}

void handle_set_description(){
    cout << "set sdp" << endl;
}

void read_loop(websocket::stream<ip::tcp::socket>& ws) {
    while (true) {
        try {
            flat_buffer buffer;
            ws.read(buffer);
            string received = buffers_to_string(buffer.data());
            json msg = json::parse(received);
            //cout << "message received: " << msg.dump(4) << endl;

            string sender = msg["sender"];
            string type = msg["type"];

            if (type == "answer") {
                pc1->setRemoteDescription(rtc::Description(msg["sdp"], "answer"));
                cout << "SDP Answer down established" << endl;
            } else if (type == "candidate") {
                rtc::Candidate candidate(msg["candidate"], msg["sdpMid"]);
                pc1->addRemoteCandidate(candidate);
                cout << "ice-candidate added" << endl;
            }
        } catch (exception& e) {
            cerr << "Error in reading: " << e.what() << endl;
            break;
        }
    }
}

int main() {
    setlocale(LC_ALL, "rus");
    try {
        io_context ioc;
        ip::tcp::resolver resolver(ioc);
        websocket::stream<ip::tcp::socket> ws(ioc);
        ws_ptr = &ws;

        auto const results = resolver.resolve("localhost", "9001");
        connect(ws.next_layer(), results.begin(), results.end());
        ws.handshake("localhost", "/");
        cout << "Connected to the Server!" << endl;

        json register_msg = {
            {"type", "register"},
            {"sender", client_id}
        };
        ws.write(buffer(register_msg.dump()));

        rtc::Configuration config;
        config.iceServers.push_back({"stun:stun1.l.google.com:19302"});

        pc1 = std::make_shared<rtc::PeerConnection>(config);

        // 2. Устанавливаем обработчик получения локального SDP (Offer)
        pc1->onLocalDescription([ref(pc1)](rtc::Description desc) {
            std::this_thread::sleep_for(std::chrono::seconds(1));
            std::cout << "Client 1 SDP:\n" << std::string(desc) << "\n";
            on_local_description(desc);
            // Здесь тебе нужно передать SDP клиенту 2 (напрвимер, через WebSocket)
        });

        // 3. Обработчик ICE-кандидатов
        pc1->onLocalCandidate([](rtc::Candidate candidate) {
            //std::this_thread::sleep_for(std::chrono::seconds(5));

            if (candidate.type() ==rtc::Candidate::Type::ServerReflexive){
                std::cout << "Client 1 ICE Candidate: " << std::string(candidate) << "\n";
                on_ice_candidate(candidate);
            }
            // Передай этот ICE-кандидат клиенту 2
        });

        pc1->onStateChange([](rtc::PeerConnection::State state) {
            if (state == rtc::PeerConnection::State::Connected) {
                std::cout << "PeerConnection accept!\n";
            }
        });


        // 4. Создание DataChannel
        auto dc = pc1->createDataChannel("chat");

        // 5. Обработчик открытия DataChannel
        dc->onOpen([dc] {
            std::cout << "DataChannel open!\n";
            while(true){
               dc->send("Hello ot Client 1!");
            }

        });

        // 6. Обработчик получения сообщений
        dc->onMessage([](rtc::message_variant msg) {
            if (std::holds_alternative<std::string>(msg)) {
                std::cout << "Message received: " << std::get<std::string>(msg) << "\n";
            }
        });

        thread read_thread(read_loop, ref(ws));

        read_thread.join();
    } catch (exception& e) {
        cerr << "Error: " << e.what() << endl;
    }

    return 0;
}
*/
#include "mainwindow.h"
//#include "p2p.h"

#include <QApplication>

int main(int argc, char *argv[])
{
    io_context ioc;
    ip::tcp::resolver resolver(ioc);
    QApplication a(argc, argv);
    MainWindow w;
    w.connection_to_the_server(ioc, resolver);
    std::thread reader([&w](){w.p2p_worker->receive_loop_from_server();});
    w.show();
    reader.detach();
    return a.exec();
}
