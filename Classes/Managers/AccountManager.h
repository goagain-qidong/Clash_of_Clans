/****************************************************************
 * Project Name:  Clash_of_Clans
 * File Name:     AccountManager.h
 * File Function: 账户管理器
 * Author:        赵崇治
 * Update Date:   2025/12/14
 * License:       MIT License
 ****************************************************************/
#pragma once

#include <string>

#include <vector>

#include <functional>

#include "json/document.h"

#include "json/writer.h"

#include "json/stringbuffer.h"



// Building data for serialization

struct BuildingSerialData {

    std::string name;

    int level;

    float gridX;

    float gridY;

    float gridWidth;

    float gridHeight;



    // Serialize to JSON

    rapidjson::Value toJson(rapidjson::Document::AllocatorType& allocator) const;



    // Deserialize from JSON

    static BuildingSerialData fromJson(const rapidjson::Value& obj);

};



// Account game state data

struct AccountGameData {

    int gold = 1000;

    int elixir = 1000;

    int darkElixir = 0;

    int gems = 0;

    int trophies = 0;

    int townHallLevel = 1;

    // 🆕 添加资源容量字段
    int goldCapacity = 3000;
    int elixirCapacity = 3000;
    
    // 🆕 添加士兵库存字段
    std::string troopInventory = "";  // JSON格式存储士兵库存
    
    std::vector<BuildingSerialData> buildings;



    // Serialize to JSON

    std::string toJson() const;



    // Deserialize from JSON

    static AccountGameData fromJson(const std::string& jsonStr);

};



// Simple account model

struct AccountInfo {

    std::string userId;   // unique id

    std::string username; // display name

    std::string password; // account password (plaintext,建议加密存储)

    std::string token;    // auth token (optional)
    
    std::string assignedMapName = "map/Map1.png"; // 每个账号分配的地图（默认Map1）

    AccountGameData gameData; // Game state data

};



class AccountManager {

public:

    static AccountManager& getInstance();



    // Initialize from storage. Returns true if an account was restored.

    bool initialize();



    // Get current account.

    const AccountInfo* getCurrentAccount() const;



    // Switch active account by userId. Returns true if success.
    // @param silent If true, suppresses UI notifications (e.g. defense logs)
    bool switchAccount(const std::string& userId, bool silent = false);

    // Create or update an account and set it active.

    void upsertAccount(const AccountInfo& acc);



    // List all accounts stored locally.

    const std::vector<AccountInfo>& listAccounts() const;



    // Sign out current account.

    void signOut();



    // Verify account password.

    bool verifyPassword(const std::string& userId, const std::string& password) const;



    // Delete an account by userId. Returns true if successful.

    bool deleteAccount(const std::string& userId);



    // ==================== Game State Management ====================

    

    /** @brief Update current account's game data */

    void updateGameData(const AccountGameData& gameData);

    

    /** @brief Get current account's game data */

    AccountGameData getCurrentGameData() const;

    

    /** @brief Save current account's game state to JSON file */

    bool saveGameStateToFile();

    

    /** @brief Load account's game state from JSON file */

    bool loadGameStateFromFile(const std::string& userId);

    

    /** @brief Get another player's game data by userId (for attacking) */

    AccountGameData getPlayerGameData(const std::string& userId) const;

    

    /** @brief Export current game state as JSON string (for server upload) */

    std::string exportGameStateJson() const;

    

    /** @brief Import game state from JSON string (from server) */

    bool importGameStateJson(const std::string& userId, const std::string& jsonData);



    // Persist current state to storage.

    void save();



private:

    AccountManager() = default;

    AccountManager(const AccountManager&) = delete;

    AccountManager& operator=(const AccountManager&) = delete;



    std::vector<AccountInfo> _accounts;

    int _activeIndex = -1;



    // Internal helpers

    void loadFromStorage();

    

    /** @brief Get the file path for a user's game data */

    std::string getGameDataFilePath(const std::string& userId) const;

};

