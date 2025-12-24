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

/**
 * @struct BuildingSerialData
 * @brief 建筑序列化数据
 */
struct BuildingSerialData {
    std::string name;   ///< 建筑名称
    int level;          ///< 建筑等级
    float gridX;        ///< 网格X坐标
    float gridY;        ///< 网格Y坐标
    float gridWidth;    ///< 网格宽度
    float gridHeight;   ///< 网格高度

    /**
     * @brief 序列化为JSON
     * @param allocator JSON分配器
     * @return rapidjson::Value JSON值
     */
    rapidjson::Value toJson(rapidjson::Document::AllocatorType& allocator) const;

    /**
     * @brief 从JSON反序列化
     * @param obj JSON对象
     * @return BuildingSerialData 建筑数据
     */
    static BuildingSerialData fromJson(const rapidjson::Value& obj);
};

/**
 * @struct AccountGameData
 * @brief 账户游戏数据
 */
struct AccountGameData {
    int gold = 1000;           ///< 金币
    int elixir = 1000;         ///< 圣水
    int darkElixir = 0;        ///< 暗黑重油
    int gems = 0;              ///< 宝石
    int trophies = 0;          ///< 奖杯
    int townHallLevel = 1;     ///< 大本营等级
    int goldCapacity = 3000;   ///< 金币容量
    int elixirCapacity = 3000; ///< 圣水容量
    std::string troopInventory = "";  ///< 士兵库存(JSON)
    std::string clanId = "";   ///< 部落ID
    std::string playerId = ""; ///< 玩家ID
    std::vector<BuildingSerialData> buildings;  ///< 建筑列表

    /**
     * @brief 序列化为JSON字符串
     * @return std::string JSON字符串
     */
    std::string toJson() const;

    /**
     * @brief 从JSON字符串反序列化
     * @param jsonStr JSON字符串
     * @return AccountGameData 游戏数据
     */
    static AccountGameData fromJson(const std::string& jsonStr);
};

/**
 * @struct AccountInfo
 * @brief 账户信息
 */
struct AccountInfo {
    std::string userId;    ///< 用户ID
    std::string username;  ///< 用户名
    std::string password;  ///< 密码
    std::string token;     ///< 认证令牌
    std::string assignedMapName = "map/Map1.png";  ///< 分配的地图
    AccountGameData gameData;  ///< 游戏数据
};

/**
 * @class AccountManager
 * @brief 账户管理器（单例）
 */
class AccountManager {
public:
    /**
     * @brief 获取单例实例
     * @return AccountManager& 单例引用
     */
    static AccountManager& getInstance();

    /**
     * @brief 初始化
     * @return bool 是否恢复了账户
     */
    bool initialize();

    /** @brief 获取当前账户 */
    const AccountInfo* getCurrentAccount() const;

    /**
     * @brief 切换账户
     * @param userId 用户ID
     * @param silent 是否静默模式
     * @return bool 是否成功
     */
    bool switchAccount(const std::string& userId, bool silent = false);

    /**
     * @brief 创建或更新账户
     * @param acc 账户信息
     */
    void upsertAccount(const AccountInfo& acc);

    /** @brief 列出所有账户 */
    const std::vector<AccountInfo>& listAccounts() const;

    /** @brief 登出 */
    void signOut();

    /**
     * @brief 验证密码
     * @param userId 用户ID
     * @param password 密码
     * @return bool 是否正确
     */
    bool verifyPassword(const std::string& userId, const std::string& password) const;

    /**
     * @brief 删除账户
     * @param userId 用户ID
     * @return bool 是否成功
     */
    bool deleteAccount(const std::string& userId);

    /**
     * @brief 更新游戏数据
     * @param gameData 游戏数据
     */
    void updateGameData(const AccountGameData& gameData);

    /** @brief 获取当前游戏数据 */
    AccountGameData getCurrentGameData() const;

    /** @brief 保存游戏状态到文件 */
    bool saveGameStateToFile();

    /**
     * @brief 从文件加载游戏状态
     * @param userId 用户ID
     * @return bool 是否成功
     */
    bool loadGameStateFromFile(const std::string& userId);

    /**
     * @brief 获取其他玩家的游戏数据
     * @param userId 用户ID
     * @return AccountGameData 游戏数据
     */
    AccountGameData getPlayerGameData(const std::string& userId) const;

    /** @brief 导出游戏状态为JSON */
    std::string exportGameStateJson() const;

    /**
     * @brief 导入游戏状态
     * @param userId 用户ID
     * @param jsonData JSON数据
     * @return bool 是否成功
     */
    bool importGameStateJson(const std::string& userId, const std::string& jsonData);

    /** @brief 保存到存储 */
    void save();

private:
    AccountManager() = default;
    AccountManager(const AccountManager&) = delete;
    AccountManager& operator=(const AccountManager&) = delete;

    std::vector<AccountInfo> _accounts;  ///< 账户列表
    int _activeIndex = -1;               ///< 当前活动账户索引

    void loadFromStorage();  ///< 从存储加载

    /**
     * @brief 获取游戏数据文件路径
     * @param userId 用户ID
     * @return std::string 文件路径
     */
    std::string getGameDataFilePath(const std::string& userId) const;
};

