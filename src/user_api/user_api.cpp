#include "user_api.h"

User_api::User_api() {}


bool User_api::register_user(const std::string& username, const std::string& password) {
    json payload = {
        {"username", username},
        {"password", password}
    };

    cpr::Response r = cpr::Post(
        cpr::Url{base_url + "register/"},
        cpr::Header{{"Content-Type", "application/json"}},
        cpr::Body{payload.dump()}
        );

    if (r.status_code == 201) {
        std::cout << "Register successful!\n";
        return true;
    } else {
        std::cout << "Erorr register: " << r.text << std::endl;
        return false;
    }
}

bool User_api::login_user(const std::string& username, const std::string& password) {
    cpr::Response r = cpr::Post(
        cpr::Url{base_url + "token/"},
        cpr::Payload{{"username", username}, {"password", password}}
        );

    if (r.status_code == 200) {
        try {
            auto jsonData = nlohmann::json::parse(r.text);
            if (jsonData.contains("access")) {
                token = jsonData["access"].get<std::string>();
                std::cout << "login successful!" << std::endl;
                this->user_name = username;
                return true;
            } else {
                std::cout << "Ответ не содержит токена: " << r.text << std::endl;
                return false;
            }
        } catch (const std::exception& e) {
            std::cout << "Ошибка парсинга JSON: " << e.what() << std::endl;
            return false;
        }
    } else {
        std::cout << "Ошибка входа: " << std::endl;
        return false;
    }
}

//"id", "username"
json User_api::get_friends() {
    cpr::Response r = cpr::Get(
        cpr::Url{base_url + "friends/"},
        cpr::Header{{"Authorization", "Bearer " + token}}
        );

    if (r.status_code == 200) {
        //std::cout << "Список друзей: " << r.text << std::endl;
        return json::parse(r.text);
    } else {
        //std::cout << "Ошибка получения списка друзей: " << r.text << std::endl;
        return {};
    }
}

//{"id":3,"username":"vadlap","email":"petricheli@mail.ru"}
json User_api::search_users(const std::string& query) {
    cpr::Response r = cpr::Get(
        cpr::Url{base_url + "search/"},
        cpr::Header{{"Authorization", "Bearer " + token}},
        cpr::Parameters{{"query", query}}
        );

    if (r.status_code == 200) {
        //std::cout << "Результаты поиска: " << r.text << std::endl;

        return json::parse(r.text);
    } else {
        //std::cout << "Ошибка поиска пользователей: " << r.text << std::endl;
        return {};
    }
}

bool User_api::send_friend_request(int to_user_id) {
    json payload = {
        {"to_user_id", to_user_id}
    };

    cpr::Response r = cpr::Post(
        cpr::Url{base_url + "friend-request/"},
        cpr::Header{
            {"Content-Type", "application/json"},
            {"Authorization", "Bearer " + token}
        },
        cpr::Body{payload.dump()}
        );

    if (r.status_code == 200) {
        //std::cout << "Запрос в друзья отправлен!" << std::endl;
        return true;
    } else {
        //std::cout << "Ошибка отправки запроса в друзья: " << r.text << std::endl;
        return false;
    }
}

bool User_api::send_message(const std::string &mes, const std::string &peer_username ) {
    json payload = {
        {"receiver_username", peer_username},
        {"content", mes}
    };

    cpr::Response r = cpr::Post(
        cpr::Url{base_url + "send-message/"},
        cpr::Header{
            {"Content-Type", "application/json"},
            {"Authorization", "Bearer " + token}
        },
        cpr::Body{payload.dump()}
        );

    if (r.status_code == 200 || r.status_code == 201) {
        std::cout << "Message send" << std::endl;
        return true;
    } else {
        std::cerr << "Error: " << r.status_code  << std::endl;
        return false;
    }
}

json User_api::get_messages_with_user(const std::string &peer_username){
    cpr::Response r = cpr::Get(
        cpr::Url{base_url + "messages/with/" + peer_username + "/"},
        cpr::Header{{"Authorization", "Bearer " + token}}
    );

    if (r.status_code == 200) {
        auto json_data = nlohmann::json::parse(r.text);
        return json_data;
        // for (const auto& msg : jsonData) {
        //     std::string from = msg["sender_username"];
        //     std::string content = msg["content"];
        //     std::string time = msg["timestamp"];
        //     std::cout << "[" << time << "] " << from << ": " << content << std::endl;
        // }
    } else {
        std::cerr << "Error recive: " << r.status_code <<  std::endl;
        return {};
    }
}

json User_api::get_channels()
{
    cpr::Response r = cpr::Get(
        cpr::Url{base_url + "channels/"},
        cpr::Header{{"Authorization", "Bearer " + token}}
        );

    if (r.status_code == 200) {
        auto channels = nlohmann::json::parse(r.text);
        return channels;
        // std::cout << "Channels:\n";
        // for (const auto& channel : channels) {
        //     std::cout << "ID: " << channel["id"] << ", Name: " << channel["name"] << "\n";
        // }
    } else {
        std::cout << "Error fetching channels: " << r.text << std::endl;
        return {};
    }
}

json User_api::get_subchannels(int channel_id)
{
    cpr::Response r = cpr::Get(
        cpr::Url{base_url + "channels/" + std::to_string(channel_id) + "/subchannels/"},
        cpr::Header{{"Authorization", "Bearer " + token}}
        );

    if (r.status_code == 200) {
        auto subchannels = nlohmann::json::parse(r.text);
        return subchannels;
        // std::cout << "Subchannels in Channel " << channel_id << ":\n";
        // for (const auto& subchannel : subchannels) {
        //     std::cout << "ID: " << subchannel["id"] << ", Name: " << subchannel["name"] << "\n";
        // }
    } else {
        std::cout << "Error fetching subchannels: " << r.text << std::endl;
        return {};
    }
}

void User_api::create_channel(const std::string &name)
{
    cpr::Response r = cpr::Post(
        cpr::Url{base_url + "channels/"},
        cpr::Header{{"Authorization", "Bearer " + token}},
        cpr::Payload{{"name", name}}
        );

    if (r.status_code == 201) {
        std::cout << "Channel created successfully!" << std::endl;
    } else {
        std::cout << "Error creating channel: " << r.text << std::endl;
    }
}

void User_api::create_subchannel(int channel_id, const std::string &name)
{
    cpr::Response r = cpr::Post(
        cpr::Url{base_url + "channels/" + std::to_string(channel_id) + "/subchannels/"},
        cpr::Header{{"Authorization", "Bearer " + token}},
        cpr::Payload{{"name", name}}
        );

    if (r.status_code == 201) {
        std::cout << "Subchannel created successfully!" << std::endl;
    } else {
        std::cout << "Error creating subchannel: " << r.text << std::endl;
    }
}

