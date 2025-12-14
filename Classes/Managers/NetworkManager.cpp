/****************************************************************
 * Project Name:  Clash_of_Clans
 * File Name:     NetworkManager.cpp
 * File Function: 网络管理器实现
 * Author:        赵崇治
 * Update Date:   2025/12/14
 * License:       MIT License
 ****************************************************************/
#include "NetworkManager.h"



using namespace cocos2d::network;



NetworkManager& NetworkManager::getInstance() {

    static NetworkManager instance;

    return instance;

}



void NetworkManager::get(const std::string& path, const std::function<void(bool, const std::string&)>& cb) {

    auto req = new HttpRequest();

    std::string url = _baseUrl.empty() ? path : (_baseUrl + path);

    req->setUrl(url);

    req->setRequestType(HttpRequest::Type::GET);

    req->setResponseCallback([cb](HttpClient* client, HttpResponse* response){

        bool ok = response && response->isSucceed();

        std::string data;

        if (response && response->getResponseData()) {

            data.assign(response->getResponseData()->begin(), response->getResponseData()->end());

        }

        if (cb) cb(ok, data);

    });

    HttpClient::getInstance()->send(req);

    req->release();

}



void NetworkManager::post(const std::string& path, const std::string& jsonBody, const std::function<void(bool, const std::string&)>& cb) {

    auto req = new HttpRequest();

    std::string url = _baseUrl.empty() ? path : (_baseUrl + path);

    req->setUrl(url);

    req->setRequestType(HttpRequest::Type::POST);

    std::vector<char> buf(jsonBody.begin(), jsonBody.end());

    req->setRequestData(buf.data(), buf.size());

    std::vector<std::string> headers;

    headers.push_back("Content-Type: application/json");

    req->setHeaders(headers);

    req->setResponseCallback([cb](HttpClient* client, HttpResponse* response){

        bool ok = response && response->isSucceed();

        std::string data;

        if (response && response->getResponseData()) {

            data.assign(response->getResponseData()->begin(), response->getResponseData()->end());

        }

        if (cb) cb(ok, data);

    });

    HttpClient::getInstance()->send(req);

    req->release();

}

