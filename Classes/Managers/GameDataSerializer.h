/****************************************************************
 * Project Name:  Clash_of_Clans
 * File Name:     GameDataSerializer.h
 * File Function: 游戏数据序列化器
 * Author:        赵崇治、薛毓哲
 * Update Date:   2025/12/24
 * License:       MIT License
 ****************************************************************/
#pragma once

#include "GameDataModels.h"
#include "JsonSerializer.h"

/**
 * @class GameDataSerializer
 * @brief 游戏数据序列化/反序列化
 */
class GameDataSerializer
{
public:
    // ==================== BuildingSerialData ====================
    static rapidjson::Value serializeBuilding(const BuildingSerialData& data, JsonSerializer::Allocator& alloc)
    {
        return JsonSerializer::Writer(alloc)
            .write("name", data.name)
            .write("level", data.level)
            .write("gridX", data.gridX)
            .write("gridY", data.gridY)
            .write("gridWidth", data.gridWidth)
            .write("gridHeight", data.gridHeight)
            .build();
    }

    static BuildingSerialData deserializeBuilding(const rapidjson::Value& obj)
    {
        JsonSerializer::Reader reader(obj);
        BuildingSerialData     data;
        data.name       = reader.readString("name");
        data.level      = reader.readInt("level", 1);
        data.gridX      = reader.readFloat("gridX");
        data.gridY      = reader.readFloat("gridY");
        data.gridWidth  = reader.readFloat("gridWidth", 1.0f);
        data.gridHeight = reader.readFloat("gridHeight", 1.0f);
        return data;
    }
    
    // ==================== UpgradeTaskSerialData ====================
    static rapidjson::Value serializeUpgradeTask(const UpgradeTaskSerialData& data, JsonSerializer::Allocator& alloc)
    {
        return JsonSerializer::Writer(alloc)
            .write("gridX", data.gridX)
            .write("gridY", data.gridY)
            .write("totalTime", data.totalTime)
            .write("elapsedTime", data.elapsedTime)
            .write("cost", data.cost)
            .write("useBuilder", data.useBuilder)
            .build();
    }

    static UpgradeTaskSerialData deserializeUpgradeTask(const rapidjson::Value& obj)
    {
        JsonSerializer::Reader reader(obj);
        UpgradeTaskSerialData  data;
        data.gridX       = reader.readFloat("gridX");
        data.gridY       = reader.readFloat("gridY");
        data.totalTime   = reader.readFloat("totalTime");
        data.elapsedTime = reader.readFloat("elapsedTime");
        data.cost        = reader.readInt("cost");
        data.useBuilder  = reader.readBool("useBuilder", true);
        return data;
    }

    // ==================== GameStateData ====================
    static std::string serializeGameState(const GameStateData& state)
    {
        rapidjson::Document doc;
        doc.SetObject();
        auto& alloc = doc.GetAllocator();

        // 资源
        doc.AddMember("gold", state.resources.gold, alloc);
        doc.AddMember("elixir", state.resources.elixir, alloc);
        doc.AddMember("darkElixir", state.resources.darkElixir, alloc);
        doc.AddMember("gems", state.resources.gems, alloc);
        doc.AddMember("goldCapacity", state.resources.goldCapacity, alloc);
        doc.AddMember("elixirCapacity", state.resources.elixirCapacity, alloc);

        // 进度
        doc.AddMember("trophies", state.progress.trophies, alloc);
        doc.AddMember("townHallLevel", state.progress.townHallLevel, alloc);

        rapidjson::Value clanIdVal, playerIdVal;
        clanIdVal.SetString(state.progress.clanId.c_str(), alloc);
        playerIdVal.SetString(state.progress.playerId.c_str(), alloc);
        doc.AddMember("clanId", clanIdVal, alloc);
        doc.AddMember("playerId", playerIdVal, alloc);

        // 士兵库存
        rapidjson::Value troopVal;
        troopVal.SetString(state.troopInventoryJson.c_str(), alloc);
        doc.AddMember("troopInventory", troopVal, alloc);

        // 建筑列表
        rapidjson::Value buildingsArr(rapidjson::kArrayType);
        for (const auto& building : state.buildings)
        {
            buildingsArr.PushBack(serializeBuilding(building, alloc), alloc);
        }
        doc.AddMember("buildings", buildingsArr, alloc);
        
        // 升级任务列表
        rapidjson::Value upgradeTasksArr(rapidjson::kArrayType);
        for (const auto& task : state.upgradeTasks)
        {
            upgradeTasksArr.PushBack(serializeUpgradeTask(task, alloc), alloc);
        }
        doc.AddMember("upgradeTasks", upgradeTasksArr, alloc);

        return JsonSerializer::stringify(doc);
    }

    static GameStateData deserializeGameState(const std::string& jsonStr)
    {
        GameStateData       state;
        rapidjson::Document doc;

        if (!JsonSerializer::parse(jsonStr, doc))
        {
            return state;
        }

        JsonSerializer::Reader reader(doc);

        // 资源
        state.resources.gold           = reader.readInt("gold", 1000);
        state.resources.elixir         = reader.readInt("elixir", 1000);
        state.resources.darkElixir     = reader.readInt("darkElixir", 0);
        state.resources.gems           = reader.readInt("gems", 0);
        state.resources.goldCapacity   = reader.readInt("goldCapacity", 3000);
        state.resources.elixirCapacity = reader.readInt("elixirCapacity", 3000);

        if (state.resources.goldCapacity <= 0)
            state.resources.goldCapacity = 3000;
        if (state.resources.elixirCapacity <= 0)
            state.resources.elixirCapacity = 3000;

        // 进度
        state.progress.trophies      = reader.readInt("trophies", 0);
        state.progress.townHallLevel = reader.readInt("townHallLevel", 1);
        state.progress.clanId        = reader.readString("clanId");
        state.progress.playerId      = reader.readString("playerId");

        // 士兵库存
        state.troopInventoryJson = reader.readString("troopInventory");

        // 建筑列表
        state.buildings = reader.readArray<BuildingSerialData>("buildings", deserializeBuilding);
        
        // 升级任务列表
        state.upgradeTasks = reader.readArray<UpgradeTaskSerialData>("upgradeTasks", deserializeUpgradeTask);

        return state;
    }

    // ==================== 向后兼容方法 ====================
    static GameStateData fromJson(const std::string& jsonStr) { return deserializeGameState(jsonStr); }
    static std::string   toJson(const GameStateData& state) { return serializeGameState(state); }
};

// ==================== GameStateData 成员方法实现 ====================

inline std::string GameStateData::toJson() const
{
    return GameDataSerializer::serializeGameState(*this);
}

inline GameStateData GameStateData::fromJson(const std::string& jsonStr)
{
    return GameDataSerializer::deserializeGameState(jsonStr);
}