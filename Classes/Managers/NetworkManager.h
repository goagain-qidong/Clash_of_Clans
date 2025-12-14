/****************************************************************
 * Project Name:  Clash_of_Clans
 * File Name:     NetworkManager.h
 * File Function: 网络管理器头文件
 * Author:        赵崇治
 * Update Date:   2025/12/14
 * License:       MIT License
 ****************************************************************/
#pragma once

#include <string>

#include <functional>

#include "cocos2d.h"

#include "cocos/network/HttpClient.h"



// Very basic networking helper to perform HTTP requests to a backend.

class NetworkManager {

public:

    static NetworkManager& getInstance();



    void setBaseUrl(const std::string& url) { _baseUrl = url; }



    // Async GET request

    void get(const std::string& path, const std::function<void(bool, const std::string&)>& cb);



    // Async POST request with JSON body

    void post(const std::string& path, const std::string& jsonBody, const std::function<void(bool, const std::string&)>& cb);



private:

    NetworkManager() = default;

    NetworkManager(const NetworkManager&) = delete;

    NetworkManager& operator=(const NetworkManager&) = delete;



    std::string _baseUrl;

};

