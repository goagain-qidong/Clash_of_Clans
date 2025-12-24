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

/**
 * @class NetworkManager
 * @brief 网络管理器（单例）- 处理HTTP请求
 */
class NetworkManager {
public:
    /**
     * @brief 获取单例实例
     * @return NetworkManager& 单例引用
     */
    static NetworkManager& getInstance();

    /**
     * @brief 设置基础URL
     * @param url 基础URL
     */
    void setBaseUrl(const std::string& url) { _baseUrl = url; }

    /**
     * @brief 异步GET请求
     * @param path 请求路径
     * @param cb 回调函数
     */
    void get(const std::string& path, const std::function<void(bool, const std::string&)>& cb);

    /**
     * @brief 异步POST请求
     * @param path 请求路径
     * @param jsonBody JSON请求体
     * @param cb 回调函数
     */
    void post(const std::string& path, const std::string& jsonBody, const std::function<void(bool, const std::string&)>& cb);

private:
    NetworkManager() = default;
    NetworkManager(const NetworkManager&) = delete;
    NetworkManager& operator=(const NetworkManager&) = delete;

    std::string _baseUrl;  ///< 基础URL
};

