/****************************************************************
 * Project Name:  Clash_of_Clans
 * File Name:     GameDataModels.h
 * File Function: 游戏数据模型定义（纯数据结构）
 * Author:        赵崇治、薛毓哲
 * Update Date:   2025/12/24
 * License:       MIT License
 ****************************************************************/
#pragma once

#include <string>
#include <vector>

// 前向声明
class GameDataSerializer;

/**
 * @struct BuildingSerialData
 * @brief 建筑序列化数据（用于存储/传输）
 */
struct BuildingSerialData
{
    std::string name;
    int         level      = 1;
    float       gridX      = 0.0f;
    float       gridY      = 0.0f;
    float       gridWidth  = 1.0f;
    float       gridHeight = 1.0f;
};

/**
 * @struct UpgradeTaskSerialData
 * @brief 升级任务序列化数据（用于保存/加载升级进度）
 */
struct UpgradeTaskSerialData
{
    float gridX       = 0.0f;   ///< 建筑网格X坐标
    float gridY       = 0.0f;   ///< 建筑网格Y坐标
    float totalTime   = 0.0f;   ///< 总升级时间（秒）
    float elapsedTime = 0.0f;   ///< 已经过的时间（秒）
    int   cost        = 0;      ///< 升级费用
    bool  useBuilder  = true;   ///< 是否占用工人
};

/**
 * @struct ResourceData
 * @brief 资源数据
 */
struct ResourceData
{
    int gold           = 1000;
    int elixir         = 1000;
    int darkElixir     = 0;
    int gems           = 0;
    int goldCapacity   = 3000;
    int elixirCapacity = 3000;
};

/**
 * @struct PlayerProgressData
 * @brief 玩家进度数据
 */
struct PlayerProgressData
{
    int         trophies      = 0;
    int         townHallLevel = 1;
    std::string clanId;
    std::string playerId;
};

/**
 * @struct GameStateData
 * @brief 完整游戏状态
 */
struct GameStateData
{
    ResourceData                       resources;
    PlayerProgressData                 progress;
    std::string                        troopInventoryJson;
    std::vector<BuildingSerialData>    buildings;
    std::vector<UpgradeTaskSerialData> upgradeTasks;  ///< 升级任务列表

    // ==================== 向后兼容访问器（用于旧代码）====================

    // 资源快捷访问
    int&       gold           = resources.gold;
    int&       elixir         = resources.elixir;
    int&       darkElixir     = resources.darkElixir;
    int&       gems           = resources.gems;
    int&       goldCapacity   = resources.goldCapacity;
    int&       elixirCapacity = resources.elixirCapacity;

    // 进度快捷访问
    int&         trophies      = progress.trophies;
    int&         townHallLevel = progress.townHallLevel;
    std::string& clanId        = progress.clanId;
    std::string& playerId      = progress.playerId;

    // 士兵库存快捷访问
    std::string& troopInventory = troopInventoryJson;

    // 默认构造函数
    GameStateData()
        : gold(resources.gold)
        , elixir(resources.elixir)
        , darkElixir(resources.darkElixir)
        , gems(resources.gems)
        , goldCapacity(resources.goldCapacity)
        , elixirCapacity(resources.elixirCapacity)
        , trophies(progress.trophies)
        , townHallLevel(progress.townHallLevel)
        , clanId(progress.clanId)
        , playerId(progress.playerId)
        , troopInventory(troopInventoryJson)
    {
    }

    // 拷贝构造函数
    GameStateData(const GameStateData& other)
        : resources(other.resources)
        , progress(other.progress)
        , troopInventoryJson(other.troopInventoryJson)
        , buildings(other.buildings)
        , gold(resources.gold)
        , elixir(resources.elixir)
        , darkElixir(resources.darkElixir)
        , gems(resources.gems)
        , goldCapacity(resources.goldCapacity)
        , elixirCapacity(resources.elixirCapacity)
        , trophies(progress.trophies)
        , townHallLevel(progress.townHallLevel)
        , clanId(progress.clanId)
        , playerId(progress.playerId)
        , troopInventory(troopInventoryJson)
    {
    }

    // 拷贝赋值运算符
    GameStateData& operator=(const GameStateData& other)
    {
        if (this != &other)
        {
            resources          = other.resources;
            progress           = other.progress;
            troopInventoryJson = other.troopInventoryJson;
            buildings          = other.buildings;
        }
        return *this;
    }

    // 移动构造函数
    GameStateData(GameStateData&& other) noexcept
        : resources(std::move(other.resources))
        , progress(std::move(other.progress))
        , troopInventoryJson(std::move(other.troopInventoryJson))
        , buildings(std::move(other.buildings))
        , gold(resources.gold)
        , elixir(resources.elixir)
        , darkElixir(resources.darkElixir)
        , gems(resources.gems)
        , goldCapacity(resources.goldCapacity)
        , elixirCapacity(resources.elixirCapacity)
        , trophies(progress.trophies)
        , townHallLevel(progress.townHallLevel)
        , clanId(progress.clanId)
        , playerId(progress.playerId)
        , troopInventory(troopInventoryJson)
    {
    }

    // 移动赋值运算符
    GameStateData& operator=(GameStateData&& other) noexcept
    {
        if (this != &other)
        {
            resources          = std::move(other.resources);
            progress           = std::move(other.progress);
            troopInventoryJson = std::move(other.troopInventoryJson);
            buildings          = std::move(other.buildings);
        }
        return *this;
    }

    // ==================== 向后兼容序列化方法 ====================
    // toJson() 是成员函数，序列化当前对象
    // fromJson() 是静态函数，从 JSON 字符串创建对象

    std::string                    toJson() const;
    static GameStateData           fromJson(const std::string& jsonStr);
};

/**
 * @struct AccountData
 * @brief 账户数据
 */
struct AccountData
{
    std::string userId;
    std::string username;
    std::string password;
    std::string token;
    std::string assignedMapName = "map/Map1.png";
};

/**
 * @typedef AccountGameData
 * @brief 向后兼容别名
 */
using AccountGameData = GameStateData;