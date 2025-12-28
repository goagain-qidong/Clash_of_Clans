/****************************************************************
 * Project Name:  Clash_of_Clans
 * File Name:     StorageManager.h
 * File Function: 存储管理器
 * Author:        赵崇治
 * Update Date:   2025/12/28
 * License:       MIT License
 ****************************************************************/
#pragma once

#include <string>

#include "cocos2d.h"

/**
 * @class StorageManager
 * @brief 存储管理器（单例）- 封装UserDefault
 */
class StorageManager
{
public:
    /**
     * @brief 获取单例实例
     * @return StorageManager& 单例引用
     */
    static StorageManager& getInstance();

    /**
     * @brief 设置字符串值
     * @param key 键
     * @param value 值
     */
    void setString(const std::string& key, const std::string& value);

    /**
     * @brief 获取字符串值
     * @param key 键
     * @param defaultValue 默认值
     * @return std::string 值
     */
    std::string getString(const std::string& key, const std::string& defaultValue = "");

    /**
     * @brief 设置整数值
     * @param key 键
     * @param value 值
     */
    void setInt(const std::string& key, int value);

    /**
     * @brief 获取整数值
     * @param key 键
     * @param defaultValue 默认值
     * @return int 值
     */
    int getInt(const std::string& key, int defaultValue = 0);

    /**
     * @brief 设置布尔值
     * @param key 键
     * @param value 值
     */
    void setBool(const std::string& key, bool value);

    /**
     * @brief 获取布尔值
     * @param key 键
     * @param defaultValue 默认值
     * @return bool 值
     */
    bool getBool(const std::string& key, bool defaultValue = false);

    /**
     * @brief 设置ValueMap
     * @param key 键
     * @param map 值
     */
    void setValueMap(const std::string& key, const cocos2d::ValueMap& map);

    /**
     * @brief 获取ValueMap
     * @param key 键
     * @return cocos2d::ValueMap 值
     */
    cocos2d::ValueMap getValueMap(const std::string& key);

    /** @brief 强制写入磁盘 */
    void flush();

private:
    StorageManager() = default;
    StorageManager(const StorageManager&) = delete;
    StorageManager& operator=(const StorageManager&) = delete;
};
