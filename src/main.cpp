
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

#include "ui/mainwindow.h"
//#include "p2p.h"

#include <QApplication>
#include <queue>

int main(int argc, char *argv[])
{
    io_context ioc;
    ip::tcp::resolver resolver(ioc);
    QApplication a(argc, argv);
    MainWindow w(ioc, resolver);
    std::thread io_thread([&ioc]() { ioc.run(); });
    w.show();
    io_thread.detach();
    //reader.detach();
    return a.exec();
}

/*

*/
// #include "rtc/rtc.hpp"

// #include <atomic>
// #include <chrono>
// #include <iostream>
// #include <memory>
// #include <thread>

// using namespace rtc;
// using namespace std;

// template <class T> weak_ptr<T> make_weak_ptr(shared_ptr<T> ptr) { return ptr; }

// int main() {
//     InitLogger(LogLevel::Debug);

//     Configuration config1;
//     // STUN server example
//     // config1.iceServers.emplace_back("stun:stun.l.google.com:19302");

//     PeerConnection pc1(config1);

//     Configuration config2;
//     // STUN server example
//     // config2.iceServers.emplace_back("stun:stun.l.google.com:19302");
//     // Port range example
//     config2.portRangeBegin = 5000;
//     config2.portRangeEnd = 6000;

//     PeerConnection pc2(config2);

//     pc1.onLocalDescription([&pc2](Description sdp) {
//         cout << "Description 1: " << sdp << endl;
//         pc2.setRemoteDescription(string(sdp));
//     });

//     pc1.onLocalCandidate([&pc2](Candidate candidate) {
//         cout << "Candidate 1: " << candidate << endl;
//         pc2.addRemoteCandidate(string(candidate));
//     });

//     pc1.onStateChange([](PeerConnection::State state) { cout << "State 1: " << state << endl; });

//     pc1.onGatheringStateChange([](PeerConnection::GatheringState state) {
//         cout << "Gathering state 1: " << state << endl;
//     });

//     pc2.onLocalDescription([&pc1](Description sdp) {
//         cout << "Description 2: " << sdp << endl;
//         pc1.setRemoteDescription(string(sdp));
//     });

//     pc2.onLocalCandidate([&pc1](Candidate candidate) {
//         cout << "Candidate 2: " << candidate << endl;
//         pc1.addRemoteCandidate(string(candidate));
//     });

//     pc2.onStateChange([](PeerConnection::State state) { cout << "State 2: " << state << endl; });

//     pc2.onGatheringStateChange([](PeerConnection::GatheringState state) {
//         cout << "Gathering state 2: " << state << endl;
//     });

//     shared_ptr<Track> t2;
//     string newTrackMid;
//     pc2.onTrack([&t2, &newTrackMid](shared_ptr<Track> t) {
//         string mid = t->mid();
//         cout << "Track 2: Received track with mid \"" << mid << "\"" << endl;
//         if (mid != newTrackMid) {
//             cerr << "Wrong track mid" << endl;
//             return;
//         }

//         t->onOpen([mid]() { cout << "Track 2: Track with mid \"" << mid << "\" is open" << endl; });

//         t->onClosed(
//             [mid]() { cout << "Track 2: Track with mid \"" << mid << "\" is closed" << endl; });

//         std::atomic_store(&t2, t);
//     });

//     // Test opening a track
//     newTrackMid = "test";

//     Description::Video media(newTrackMid, Description::Direction::SendOnly);
//     media.addH264Codec(96);
//     media.setBitrate(3000);
//     media.addSSRC(1234, "video-send");

//     const auto mediaSdp1 = string(media);
//     const auto mediaSdp2 = string(Description::Media(mediaSdp1));
//     if (mediaSdp2 != mediaSdp1) {
//         cout << mediaSdp2 << endl;
//         throw runtime_error("Media description parsing test failed");
//     }

//     auto t1 = pc1.addTrack(media);

//     pc1.setLocalDescription();

//     int attempts = 10;
//     shared_ptr<Track> at2;
//     while ((!(at2 = std::atomic_load(&t2)) || !at2->isOpen() || !t1->isOpen()) && attempts--)
//         this_thread::sleep_for(1s);

//     if (pc1.state() != PeerConnection::State::Connected ||
//         pc2.state() != PeerConnection::State::Connected)
//         throw runtime_error("PeerConnection is not connected");

//     if (!at2 || !at2->isOpen() || !t1->isOpen())
//         throw runtime_error("Track is not open");

//     // Test renegotiation
//     newTrackMid = "added";

//     Description::Video media2(newTrackMid, Description::Direction::SendOnly);
//     media2.addH264Codec(96);
//     media2.setBitrate(3000);
//     media2.addSSRC(2468, "video-send");

//     // NOTE: Overwriting the old shared_ptr for t1 will cause it's respective
//     //       track to be dropped (so it's SSRCs won't be on the description next time)
//     t1 = pc1.addTrack(media2);

//     pc1.setLocalDescription();

//     attempts = 10;
//     t2.reset();
//     while ((!(at2 = std::atomic_load(&t2)) || !at2->isOpen() || !t1->isOpen()) && attempts--)
//         this_thread::sleep_for(1s);

//     if (!at2 || !at2->isOpen() || !t1->isOpen())
//         throw runtime_error("Renegotiated track is not open");

//     // TODO: Test sending RTP packets in track

//     // Delay close of peer 2 to check closing works properly
//     pc1.close();
//     this_thread::sleep_for(1s);
//     pc2.close();
//     this_thread::sleep_for(1s);

//     if (!t1->isClosed() || !t2->isClosed())
//         throw runtime_error("Track is not closed");

//     cout << "Success" << endl;

// }


