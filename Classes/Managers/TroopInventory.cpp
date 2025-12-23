/****************************************************************
 * Project Name:  Clash_of_Clans
 * File Name:     TroopInventory.cpp
 * File Function: 士兵库存管理器实现
 * Author:        薛毓哲
 * Update Date:   2025/01/10
 * License:       MIT License
 ****************************************************************/
#include "TroopInventory.h"
#include "ResourceManager.h"
#include "AccountManager.h"
#include "json/document.h"
#include "json/writer.h"
#include "json/stringbuffer.h"

USING_NS_CC;

// 静态成员初始化
TroopInventory* TroopInventory::_instance = nullptr;

// ==================== 单例实现 ====================

TroopInventory::TroopInventory()
{
    // 初始化所有兵种数量为0
    _troops[UnitType::kBarbarian] = 0;
    _troops[UnitType::kArcher] = 0;
    _troops[UnitType::kGiant] = 0;
    _troops[UnitType::kGoblin] = 0;
    _troops[UnitType::kWallBreaker] = 0;
}

TroopInventory& TroopInventory::getInstance()
{
    if (!_instance)
    {
        _instance = new (std::nothrow) TroopInventory();
    }
    return *_instance;
}

// ==================== 士兵数量管理 ====================

int TroopInventory::getTroopCount(UnitType type) const
{
    auto it = _troops.find(type);
    if (it != _troops.end())
    {
        return it->second;
    }
    return 0;
}

int TroopInventory::addTroops(UnitType type, int count)
{
    if (count <= 0)
        return 0;
    
    // 检查人口上限
    auto& resMgr = ResourceManager::getInstance();
    int currentPop = resMgr.getCurrentTroopCount();
    int maxPop = resMgr.getMaxTroopCapacity();
    int unitPop = getUnitPopulation(type);
    
    int neededPop = count * unitPop;
    int availablePop = maxPop - currentPop;
    
    if (availablePop <= 0)
    {
        CCLOG("⚠️ 人口已满，无法添加士兵！");
        return 0;
    }
    
    // 计算实际可添加的数量
    int actualCount = std::min(count, availablePop / unitPop);
    
    if (actualCount > 0)
    {
        _troops[type] += actualCount;
        
        // 更新人口计数
        resMgr.addTroops(actualCount * unitPop);
        
        CCLOG("✅ 添加 %d 个士兵（类型：%d），当前库存：%d", 
              actualCount, static_cast<int>(type), _troops[type]);
        
        // 通知UI更新
        notifyChange(type, _troops[type]);
        
        // 自动保存
        save();
    }
    
    return actualCount;
}

bool TroopInventory::consumeTroops(UnitType type, int count)
{
    if (count <= 0)
        return false;
    
    // 检查是否有足够的士兵
    if (!hasEnoughTroops(type, count))
    {
        CCLOG("⚠️ 士兵不足！需要：%d，拥有：%d", count, getTroopCount(type));
        return false;
    }
    
    // 扣除士兵
    _troops[type] -= count;
    
    // 减少人口计数
    int unitPop = getUnitPopulation(type);
    ResourceManager::getInstance().consume(ResourceType::kTroopPopulation, count * unitPop);
    
    CCLOG("✅ 消耗 %d 个士兵（类型：%d），剩余：%d", 
          count, static_cast<int>(type), _troops[type]);
    
    // 通知UI更新
    notifyChange(type, _troops[type]);
    
    // 自动保存
    save();
    
    return true;
}

bool TroopInventory::hasEnoughTroops(UnitType type, int count) const
{
    return getTroopCount(type) >= count;
}

int TroopInventory::getTotalPopulation() const
{
    int total = 0;
    for (const auto& pair : _troops)
    {
        int unitPop = getUnitPopulation(pair.first);
        total += pair.second * unitPop;
    }
    return total;
}

void TroopInventory::clearAll()
{
    for (auto& pair : _troops)
    {
        pair.second = 0;
    }
    
    // 清空人口计数
    ResourceManager::getInstance().setResourceCount(ResourceType::kTroopPopulation, 0);
    
    CCLOG("⚠️ 清空所有士兵库存");
    
    // 通知UI更新
    for (const auto& pair : _troops)
    {
        notifyChange(pair.first, 0);
    }
}

// ==================== 序列化/反序列化 ====================

std::string TroopInventory::toJson() const
{
    rapidjson::Document doc;
    doc.SetObject();
    auto& allocator = doc.GetAllocator();
    
    // 保存各兵种数量
    rapidjson::Value troops(rapidjson::kObjectType);
    
    for (const auto& pair : _troops)
    {
        std::string key = std::to_string(static_cast<int>(pair.first));
        troops.AddMember(
            rapidjson::Value(key.c_str(), allocator),
            rapidjson::Value(pair.second),
            allocator
        );
    }
    
    doc.AddMember("troops", troops, allocator);
    
    // 转换为字符串
    rapidjson::StringBuffer buffer;
    rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
    doc.Accept(writer);
    
    return buffer.GetString();
}

bool TroopInventory::fromJson(const std::string& jsonStr)
{
    if (jsonStr.empty())
    {
        CCLOG("⚠️ 士兵库存JSON为空，使用默认值");
        return false;
    }
    
    rapidjson::Document doc;
    doc.Parse(jsonStr.c_str());
    
    if (doc.HasParseError() || !doc.IsObject())
    {
        CCLOG("❌ 解析士兵库存JSON失败！");
        return false;
    }
    
    if (!doc.HasMember("troops") || !doc["troops"].IsObject())
    {
        CCLOG("⚠️ JSON中缺少troops字段");
        return false;
    }
    
    // 清空当前库存
    for (auto& pair : _troops)
    {
        pair.second = 0;
    }
    
    // 读取各兵种数量
    const auto& troopsObj = doc["troops"];
    for (auto it = troopsObj.MemberBegin(); it != troopsObj.MemberEnd(); ++it)
    {
        int typeInt = std::atoi(it->name.GetString());
        int count = it->value.GetInt();
        
        UnitType type = static_cast<UnitType>(typeInt);
        _troops[type] = count;
        
        CCLOG("📦 加载士兵：类型=%d，数量=%d", typeInt, count);
    }
    
    // 重新计算人口数
    int totalPop = getTotalPopulation();
    ResourceManager::getInstance().setResourceCount(ResourceType::kTroopPopulation, totalPop);
    
    CCLOG("✅ 士兵库存加载完成，总人口：%d", totalPop);
    
    return true;
}

// ==================== 文件保存/加载 ====================

void TroopInventory::save(const std::string& forceUserId)
{
    auto& accMgr = AccountManager::getInstance();
    const auto* account = accMgr.getCurrentAccount();
    
    std::string userId;
    if (!forceUserId.empty())
    {
        userId = forceUserId;
    }
    else if (account)
    {
        userId = account->userId;
    }
    else
    {
        CCLOG("⚠️ 无当前账号，无法保存士兵库存");
        return;
    }
    
    std::string filename = "troop_inv_" + userId + ".json";
    std::string path = FileUtils::getInstance()->getWritablePath() + filename;
    std::string json = toJson();
    
    FILE* file = fopen(path.c_str(), "w");
    if (file)
    {
        fputs(json.c_str(), file);
        fclose(file);
        CCLOG("💾 士兵库存已保存：%s", filename.c_str());
    }
    else
    {
        CCLOG("❌ 保存士兵库存失败：%s", filename.c_str());
    }
}

void TroopInventory::load()
{
    auto& accMgr = AccountManager::getInstance();
    const auto* account = accMgr.getCurrentAccount();
    
    if (!account)
    {
        CCLOG("⚠️ 无当前账号，无法加载士兵库存");
        return;
    }
    
    std::string filename = "troop_inv_" + account->userId + ".json";
    std::string path = FileUtils::getInstance()->getWritablePath() + filename;
    
    if (!FileUtils::getInstance()->isFileExist(path))
    {
        CCLOG("⚠️ 士兵库存文件不存在，使用默认值：%s", filename.c_str());
        return;
    }
    
    std::string json = FileUtils::getInstance()->getStringFromFile(path);
    if (fromJson(json))
    {
        CCLOG("📂 士兵库存已加载：%s", filename.c_str());
    }
}

// ==================== 回调通知 ====================

void TroopInventory::setOnTroopChangeCallback(const std::function<void(UnitType, int)>& callback)
{
    _onTroopChangeCallback = callback;
}

void TroopInventory::notifyChange(UnitType type, int newCount)
{
    if (_onTroopChangeCallback)
    {
        _onTroopChangeCallback(type, newCount);
    }
}

// ==================== 辅助函数 ====================

int TroopInventory::getUnitPopulation(UnitType type) const
{
    // 各兵种占用的人口数
    switch (type)
    {
    case UnitType::kBarbarian:   return 1;  // 野蛮人：1人口
    case UnitType::kArcher:      return 1;  // 弓箭手：1人口
    case UnitType::kGoblin:      return 1;  // 哥布林：1人口
    case UnitType::kGiant:       return 5;  // 巨人：5人口
    case UnitType::kWallBreaker: return 2;  // 炸弹人：2人口
    default:                     return 1;
    }
}
