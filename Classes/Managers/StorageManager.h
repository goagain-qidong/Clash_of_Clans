#pragma once

#include "cocos2d.h"

#include <string>



// Simple wrapper over UserDefault for structured persistence

class StorageManager {

public:

    static StorageManager& getInstance();



    // Get/Set primitive types

    void setString(const std::string& key, const std::string& value);

    std::string getString(const std::string& key, const std::string& defaultValue = "");



    void setInt(const std::string& key, int value);

    int getInt(const std::string& key, int defaultValue = 0);



    void setBool(const std::string& key, bool value);

    bool getBool(const std::string& key, bool defaultValue = false);



    // Structured data via ValueMap

    void setValueMap(const std::string& key, const cocos2d::ValueMap& map);

    cocos2d::ValueMap getValueMap(const std::string& key);



    // Force write to disk

    void flush();



private:

    StorageManager() = default;

    StorageManager(const StorageManager&) = delete;

    StorageManager& operator=(const StorageManager&) = delete;

};

