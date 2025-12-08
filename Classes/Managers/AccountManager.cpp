#include "AccountManager.h"

#include <algorithm>

#include "StorageManager.h"
#include "ResourceManager.h"
#include "cocos2d.h"

using namespace cocos2d;

// ==================== BuildingSerialData Implementation ====================

rapidjson::Value BuildingSerialData::toJson(rapidjson::Document::AllocatorType& allocator) const {
    rapidjson::Value obj(rapidjson::kObjectType);
    
    rapidjson::Value nameVal;
    nameVal.SetString(name.c_str(), static_cast<rapidjson::SizeType>(name.length()), allocator);
    obj.AddMember("name", nameVal, allocator);
    
    obj.AddMember("level", level, allocator);
    obj.AddMember("gridX", gridX, allocator);
    obj.AddMember("gridY", gridY, allocator);
    obj.AddMember("gridWidth", gridWidth, allocator);
    obj.AddMember("gridHeight", gridHeight, allocator);
    
    return obj;
}

BuildingSerialData BuildingSerialData::fromJson(const rapidjson::Value& obj) {
    BuildingSerialData data;
    
    if (obj.HasMember("name") && obj["name"].IsString()) {
        data.name = obj["name"].GetString();
    }
    if (obj.HasMember("level") && obj["level"].IsInt()) {
        data.level = obj["level"].GetInt();
    }
    if (obj.HasMember("gridX") && obj["gridX"].IsNumber()) {
        data.gridX = obj["gridX"].GetFloat();
    }
    if (obj.HasMember("gridY") && obj["gridY"].IsNumber()) {
        data.gridY = obj["gridY"].GetFloat();
    }
    if (obj.HasMember("gridWidth") && obj["gridWidth"].IsNumber()) {
        data.gridWidth = obj["gridWidth"].GetFloat();
    }
    if (obj.HasMember("gridHeight") && obj["gridHeight"].IsNumber()) {
        data.gridHeight = obj["gridHeight"].GetFloat();
    }
    
    return data;
}

// ==================== AccountGameData Implementation ====================

std::string AccountGameData::toJson() const {
    rapidjson::Document doc;
    doc.SetObject();
    auto& allocator = doc.GetAllocator();
    
    doc.AddMember("gold", gold, allocator);
    doc.AddMember("elixir", elixir, allocator);
    doc.AddMember("darkElixir", darkElixir, allocator);
    doc.AddMember("gems", gems, allocator);
    doc.AddMember("trophies", trophies, allocator);
    doc.AddMember("townHallLevel", townHallLevel, allocator);
    
    // Serialize buildings
    rapidjson::Value buildingsArray(rapidjson::kArrayType);
    for (const auto& building : buildings) {
        buildingsArray.PushBack(building.toJson(allocator), allocator);
    }
    doc.AddMember("buildings", buildingsArray, allocator);
    
    rapidjson::StringBuffer buffer;
    rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
    doc.Accept(writer);
    
    return buffer.GetString();
}

AccountGameData AccountGameData::fromJson(const std::string& jsonStr) {
    AccountGameData data;
    
    rapidjson::Document doc;
    doc.Parse(jsonStr.c_str());
    
    if (doc.HasParseError() || !doc.IsObject()) {
        CCLOG("AccountGameData::fromJson - Parse error");
        return data;
    }
    
    if (doc.HasMember("gold") && doc["gold"].IsInt()) {
        data.gold = doc["gold"].GetInt();
    }
    if (doc.HasMember("elixir") && doc["elixir"].IsInt()) {
        data.elixir = doc["elixir"].GetInt();
    }
    if (doc.HasMember("darkElixir") && doc["darkElixir"].IsInt()) {
        data.darkElixir = doc["darkElixir"].GetInt();
    }
    if (doc.HasMember("gems") && doc["gems"].IsInt()) {
        data.gems = doc["gems"].GetInt();
    }
    if (doc.HasMember("trophies") && doc["trophies"].IsInt()) {
        data.trophies = doc["trophies"].GetInt();
    }
    if (doc.HasMember("townHallLevel") && doc["townHallLevel"].IsInt()) {
        data.townHallLevel = doc["townHallLevel"].GetInt();
    }
    
    // Deserialize buildings
    if (doc.HasMember("buildings") && doc["buildings"].IsArray()) {
        const auto& buildingsArray = doc["buildings"].GetArray();
        for (const auto& buildingObj : buildingsArray) {
            data.buildings.push_back(BuildingSerialData::fromJson(buildingObj));
        }
    }
    
    return data;
}

// ==================== AccountManager Implementation ====================

AccountManager& AccountManager::getInstance()
{
    static AccountManager instance;
    return instance;
}

bool AccountManager::initialize()
{
    loadFromStorage();
    
    // Load game data for current account
    if (_activeIndex >= 0 && _activeIndex < (int)_accounts.size()) {
        loadGameStateFromFile(_accounts[_activeIndex].userId);
    }
    
    return _activeIndex >= 0 && _activeIndex < (int)_accounts.size();
}

const AccountInfo* AccountManager::getCurrentAccount() const
{
    if (_activeIndex >= 0 && _activeIndex < (int)_accounts.size())
    {
        return &_accounts[_activeIndex];
    }
    return nullptr;
}

bool AccountManager::switchAccount(const std::string& userId)
{
    for (size_t i = 0; i < _accounts.size(); ++i)
    {
        if (_accounts[i].userId == userId)
        {
            _activeIndex = (int)i;
            save();
            
            // Load game state for new account
            loadGameStateFromFile(userId);
            
            return true;
        }
    }
    return false;
}

bool AccountManager::verifyPassword(const std::string& userId, const std::string& password) const
{
    for (const auto& acc : _accounts)
    {
        if (acc.userId == userId)
        {
            return acc.password == password;
        }
    }
    return false;
}

void AccountManager::upsertAccount(const AccountInfo& acc)
{
    auto it =
        std::find_if(_accounts.begin(), _accounts.end(), [&](const AccountInfo& a) { return a.userId == acc.userId; });

    if (it != _accounts.end())
    {
        *it = acc;
        _activeIndex = (int)std::distance(_accounts.begin(), it);
    }
    else
    {
        // 新账号，随机分配地图
        AccountInfo newAcc = acc;
        if (newAcc.assignedMapName.empty() || newAcc.assignedMapName == "map/Map1.png") {
            const std::vector<std::string> availableMaps = {"map/Map1.png", "map/Map2.png", "map/Map3.png"};
            int randomIndex = rand() % availableMaps.size();
            newAcc.assignedMapName = availableMaps[randomIndex];
            CCLOG("✅ Assigned map %s to new account %s", newAcc.assignedMapName.c_str(), newAcc.userId.c_str());
        }
        
        _accounts.push_back(newAcc);
        _activeIndex = (int)_accounts.size() - 1;
    }

    save();
    saveGameStateToFile();
}

const std::vector<AccountInfo>& AccountManager::listAccounts() const
{
    return _accounts;
}

void AccountManager::signOut()
{
    _activeIndex = -1;
    save();
}

// ==================== Game State Management ====================

void AccountManager::updateGameData(const AccountGameData& gameData) {
    if (_activeIndex >= 0 && _activeIndex < (int)_accounts.size()) {
        _accounts[_activeIndex].gameData = gameData;
        saveGameStateToFile();
    }
}

AccountGameData AccountManager::getCurrentGameData() const {
    if (_activeIndex >= 0 && _activeIndex < (int)_accounts.size()) {
        return _accounts[_activeIndex].gameData;
    }
    return AccountGameData();
}

std::string AccountManager::getGameDataFilePath(const std::string& userId) const {
    return FileUtils::getInstance()->getWritablePath() + "gamedata_" + userId + ".json";
}

bool AccountManager::saveGameStateToFile() {
    if (_activeIndex < 0 || _activeIndex >= (int)_accounts.size()) {
        return false;
    }
    
    const auto& account = _accounts[_activeIndex];
    std::string filePath = getGameDataFilePath(account.userId);
    std::string jsonData = account.gameData.toJson();
    
    bool result = FileUtils::getInstance()->writeStringToFile(jsonData, filePath);
    
    if (result) {
        CCLOG("✅ Game state saved for user %s to %s", account.userId.c_str(), filePath.c_str());
    } else {
        CCLOG("❌ Failed to save game state for user %s", account.userId.c_str());
    }
    
    return result;
}

bool AccountManager::loadGameStateFromFile(const std::string& userId) {
    std::string filePath = getGameDataFilePath(userId);
    
    if (!FileUtils::getInstance()->isFileExist(filePath)) {
        CCLOG("⚠️ Game data file not found for user %s, using defaults", userId.c_str());
        return false;
    }
    
    std::string jsonData = FileUtils::getInstance()->getStringFromFile(filePath);
    
    if (jsonData.empty()) {
        CCLOG("❌ Failed to read game data file for user %s", userId.c_str());
        return false;
    }
    
    // Find account and update game data
    for (auto& account : _accounts) {
        if (account.userId == userId) {
            account.gameData = AccountGameData::fromJson(jsonData);
            
            // Sync resources to ResourceManager
            auto* resMgr = ResourceManager::GetInstance();
            resMgr->SetResourceCount(ResourceType::kGold, account.gameData.gold);
            resMgr->SetResourceCount(ResourceType::kElixir, account.gameData.elixir);
            // darkElixir 暂不支持
            resMgr->SetResourceCount(ResourceType::kGem, account.gameData.gems);
            
            CCLOG("✅ Game state loaded for user %s: Gold=%d, Elixir=%d, Buildings=%zu",
                  userId.c_str(), account.gameData.gold, account.gameData.elixir, 
                  account.gameData.buildings.size());
            
            return true;
        }
    }
    
    return false;
}

AccountGameData AccountManager::getPlayerGameData(const std::string& userId) const {
    // First check if user is in local accounts
    for (const auto& account : _accounts) {
        if (account.userId == userId) {
            return account.gameData;
        }
    }
    
    // Try to load from file
    std::string filePath = getGameDataFilePath(userId);
    if (FileUtils::getInstance()->isFileExist(filePath)) {
        std::string jsonData = FileUtils::getInstance()->getStringFromFile(filePath);
        return AccountGameData::fromJson(jsonData);
    }
    
    CCLOG("⚠️ Player game data not found for userId: %s", userId.c_str());
    return AccountGameData();
}

std::string AccountManager::exportGameStateJson() const {
    if (_activeIndex >= 0 && _activeIndex < (int)_accounts.size()) {
        return _accounts[_activeIndex].gameData.toJson();
    }
    return "{}";
}

bool AccountManager::importGameStateJson(const std::string& userId, const std::string& jsonData) {
    for (auto& account : _accounts) {
        if (account.userId == userId) {
            account.gameData = AccountGameData::fromJson(jsonData);
            
            // Save to file
            std::string filePath = getGameDataFilePath(userId);
            bool result = FileUtils::getInstance()->writeStringToFile(jsonData, filePath);
            
            CCLOG("Import game state for %s: %s", userId.c_str(), result ? "Success" : "Failed");
            return result;
        }
    }
    
    return false;
}

// ==================== Storage ====================

void AccountManager::save()
{
    ValueMap root;
    root["activeIndex"] = Value(_activeIndex);

    ValueVector arr;
    arr.reserve(_accounts.size());

    for (const auto& a : _accounts)
    {
        ValueMap vm;
        vm["userId"] = Value(a.userId);
        vm["username"] = Value(a.username);
        vm["password"] = Value(a.password);
        vm["token"] = Value(a.token);
        vm["assignedMapName"] = Value(a.assignedMapName);
        arr.push_back(Value(vm));
    }

    root["accounts"] = Value(arr);
    StorageManager::getInstance().setValueMap("accounts", root);
    StorageManager::getInstance().flush();
}

void AccountManager::loadFromStorage()
{
    auto root = StorageManager::getInstance().getValueMap("accounts");
    _accounts.clear();
    _activeIndex = -1;

    if (root.empty())
        return;

    auto itIdx = root.find("activeIndex");
    if (itIdx != root.end())
    {
        _activeIndex = itIdx->second.asInt();
    }

    auto itArr = root.find("accounts");
    if (itArr != root.end())
    {
        const ValueVector& arr = itArr->second.asValueVector();
        for (const auto& v : arr)
        {
            const ValueMap& vm = v.asValueMap();
            AccountInfo a;

            auto it = vm.find("userId");
            if (it != vm.end())
                a.userId = it->second.asString();

            it = vm.find("username");
            if (it != vm.end())
                a.username = it->second.asString();

            it = vm.find("password");
            if (it != vm.end())
                a.password = it->second.asString();

            it = vm.find("token");
            if (it != vm.end())
                a.token = it->second.asString();

            it = vm.find("assignedMapName");
            if (it != vm.end())
                a.assignedMapName = it->second.asString();
            else
                a.assignedMapName = "map/Map1.png"; // 默认地图

            _accounts.push_back(a);
        }
    }

    if (_activeIndex < 0 || _activeIndex >= (int)_accounts.size())
    {
        _activeIndex = _accounts.empty() ? -1 : 0;
    }
}
