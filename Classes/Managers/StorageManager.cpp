#include "StorageManager.h"

#include "cocos2d.h"



using namespace cocos2d;



StorageManager& StorageManager::getInstance() {

    static StorageManager instance;

    return instance;

}



void StorageManager::setString(const std::string& key, const std::string& value) {

    UserDefault::getInstance()->setStringForKey(key.c_str(), value);

}



std::string StorageManager::getString(const std::string& key, const std::string& defaultValue) {

    return UserDefault::getInstance()->getStringForKey(key.c_str(), defaultValue);

}



void StorageManager::setInt(const std::string& key, int value) {

    UserDefault::getInstance()->setIntegerForKey(key.c_str(), value);

}



int StorageManager::getInt(const std::string& key, int defaultValue) {

    return UserDefault::getInstance()->getIntegerForKey(key.c_str(), defaultValue);

}



void StorageManager::setBool(const std::string& key, bool value) {

    UserDefault::getInstance()->setBoolForKey(key.c_str(), value);

}



bool StorageManager::getBool(const std::string& key, bool defaultValue) {

    return UserDefault::getInstance()->getBoolForKey(key.c_str(), defaultValue);

}



void StorageManager::setValueMap(const std::string& key, const ValueMap& map) {

    // Persist structured data to a file in writable path using plist format.

    std::string filename = FileUtils::getInstance()->getWritablePath() + key + ".plist";

    FileUtils::getInstance()->writeValueMapToFile(map, filename);

}



ValueMap StorageManager::getValueMap(const std::string& key) {

    std::string filename = FileUtils::getInstance()->getWritablePath() + key + ".plist";

    if (FileUtils::getInstance()->isFileExist(filename)) {

        return FileUtils::getInstance()->getValueMapFromFile(filename);

    }

    return ValueMap();

}



void StorageManager::flush() {

    UserDefault::getInstance()->flush();

}

