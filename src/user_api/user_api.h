#ifndef USER_API_H
#define USER_API_H

#include <cpr/cpr.h>
#include <iostream>
#include <nlohmann/json.hpp> // для парсинга JSON
//#include <vector>

using json = nlohmann::json;

class User_api
{
public:
    User_api();


private:
    //std::string base_url = "http://192.168.0.190:8000/account/api/"; // Укороченный путь, без /account/
    std::string base_url = "http://127.0.0.1:8000/account/api/"; // Укороченный путь, без /account/
    std::string token; // Токен для авторизации

public:
    std::string user_name;

public:
    bool register_user(const std::string& username, const std::string& password);

    bool login_user(const std::string& username, const std::string& password);
    //"id", "username"
    json get_friends();

    //{"id":3,"username":"vadlap","email":"petricheli@mail.ru"}
    json search_users(const std::string& query);

    bool send_friend_request(int to_user_id);

    bool send_message(const std::string &mes, const std::string &peer_username);

    json get_messages_with_user(const std::string &peer_username);

    // Получить список каналов
    json get_channels();

    // Получить подканалы конкретного канала
    json get_subchannels(int channel_id);

    // Создать канал
    void create_channel(const std::string& name);

    // Создать подканал в канале
    void create_subchannel(int channel_id, const std::string& name);

    void print_username_user(json &json_data){
        for (const auto& user : json_data) {
            std::cout << user["username"] << std::endl;
        }
    }

    void print_id_user(json &json_data){
        for (const auto& user : json_data) {
            std::cout << json_data["id"] << std::endl;
        }
    }

};

#endif // USER_API_H
