/****************************************************************
 * Project Name:  Clash_of_Clans
 * File Name:     BuildingUpgradeService.cpp
 * File Function: 建筑升级服务实现
 * Author:        薛毓哲、刘相成
 * Update Date:   2025/12/08
 * License:       MIT License
 ****************************************************************/
#include "BuildingUpgradeService.h"
#include "Buildings/BaseBuilding.h"
#include "Managers/ResourceManager.h"
#include "Managers/UpgradeManager.h"
#include "Managers/BuildingLimitManager.h"
#include "cocos2d.h"

USING_NS_CC;

BuildingUpgradeService& BuildingUpgradeService::getInstance()
{
    static BuildingUpgradeService instance;
    return instance;
}

BuildingUpgradeService::BuildingUpgradeService()
{
}

bool BuildingUpgradeService::canUpgrade(const BaseBuilding* building) const
{
    if (!building)
    {
        return false;
    }
    
    return checkLevel(building) 
        && checkUpgrading(building) 
        && checkResource(building) 
        && checkBuilder(building);
}

UpgradeResult BuildingUpgradeService::tryUpgrade(BaseBuilding* building)
{
    if (!building)
    {
        return UpgradeResult::Failure(UpgradeError::kUnknownError, "建筑指针为空");
    }
    
    // 1. 检查等级
    if (!checkLevel(building))
    {
        // 区分是配置最大等级还是大本营等级限制
        if (building->isMaxLevel())
        {
            return UpgradeResult::Failure(UpgradeError::kMaxLevel, "已达最高等级");
        }
        else
        {
            auto* limitMgr = BuildingLimitManager::getInstance();
            std::string errorMsg = StringUtils::format(
                "需要先升级大本营！当前大本营 Lv.%d，建筑最高可升至 Lv.%d",
                limitMgr->getTownHallLevel(), limitMgr->getTownHallLevel());
            return UpgradeResult::Failure(UpgradeError::kTownHallLevelLimit, errorMsg);
        }
    }
    
    // 2. 检查是否已在升级中
    if (!checkUpgrading(building))
    {
        return UpgradeResult::Failure(UpgradeError::kAlreadyUpgrading, "已在升级中");
    }
    
    // 3. 获取升级信息
    auto& resMgr = ResourceManager::getInstance();
    auto* upgradeMgr = UpgradeManager::getInstance();
    
    int cost = building->getUpgradeCost();
    ResourceType costType = building->getUpgradeCostType();
    float time = building->getUpgradeTime();
    
    // 4. 检查工人（在扣除资源之前）
    if (!checkBuilder(building))
    {
        int availableBuilders = upgradeMgr->getAvailableBuilders();
        int totalBuilders = resMgr.getResourceCapacity(kBuilder);
        int usedBuilders = upgradeMgr->getUpgradeQueueLength();
        
        std::string errorMsg = StringUtils::format(
            "无空闲工人！当前空闲：%d，总数：%d，使用中：%d",
            availableBuilders, totalBuilders, usedBuilders);
        
        CCLOG("❌ 升级失败：%s", errorMsg.c_str());
        CCLOG("   - 建筑：%s", building->getDisplayName().c_str());
        
        return UpgradeResult::Failure(UpgradeError::kNoAvailableBuilder, errorMsg);
    }
    
        // 5. 检查并扣除资源
    if (!resMgr.consume(costType, cost))
    {
        std::string  resName;
        UpgradeError error;

        switch (costType)
        {
        case ResourceType::kGold:
            resName = "金币";
            error   = UpgradeError::kNotEnoughGold;
            break;
        case ResourceType::kElixir:
            resName = "圣水";
            error   = UpgradeError::kNotEnoughElixir;
            break;
        case ResourceType::kGem:
            resName = "宝石";
            error   = UpgradeError::kNotEnoughGem;
            break;
        default:
            resName = "资源";
            error   = UpgradeError::kUnknownError;
            break;
        }

        std::string errorMsg =
            StringUtils::format("%s不足！需要：%d，当前：%d", resName.c_str(), cost, resMgr.getResourceCount(costType));

        CCLOG("❌ 升级失败：%s", errorMsg.c_str());
        CCLOG("   - 建筑：%s", building->getDisplayName().c_str());

        return UpgradeResult::Failure(error, errorMsg);
    }
    
    // 6. 启动升级倒计时
    if (!upgradeMgr->startUpgrade(building, cost, time, true))
    {
        // 升级失败，退还资源
        resMgr.addResource(costType, cost);
        
        std::string errorMsg = StringUtils::format("启动升级失败，已退还 %d 资源", cost);
        CCLOG("❌ %s", errorMsg.c_str());
        
        return UpgradeResult::Failure(UpgradeError::kStartUpgradeFailed, errorMsg);
    }
    
    // 7. 输出成功日志
    std::string displayName = building->getDisplayName();
    size_t lvPos = displayName.find(" Lv.");
    std::string buildingName = (lvPos != std::string::npos) 
        ? displayName.substr(0, lvPos) 
        : displayName;
    
    CCLOG("✅ %s 开始升级：Lv.%d → Lv.%d（需要 %.0f 秒，费用 %d）", 
          buildingName.c_str(), 
          building->getLevel(), 
          building->getLevel() + 1, 
          time, 
          cost);
    
    return UpgradeResult::Success();
}

// ==================== 私有检查方法 ====================

bool BuildingUpgradeService::checkLevel(const BaseBuilding* building) const
{
    // 1. 检查是否已达配置的最大等级
    if (building->isMaxLevel()) {
        return false;
    }
    
    // 2. 检查大本营等级限制
    auto* limitMgr = BuildingLimitManager::getInstance();
    std::string buildingName = building->getDisplayName();
    
    // 提取纯建筑名（去除 " Lv.X" 后缀）
    size_t lvPos = buildingName.find(" Lv.");
    if (lvPos != std::string::npos) {
        buildingName = buildingName.substr(0, lvPos);
    }
    
    int targetLevel = building->getLevel() + 1;
    if (!limitMgr->canUpgradeToLevel(buildingName, targetLevel)) {
        CCLOG("❌ %s 无法升级到 Lv.%d：需要先升级大本营（当前大本营 Lv.%d）",
              buildingName.c_str(), targetLevel, limitMgr->getTownHallLevel());
        return false;
    }
    
    return true;
}

bool BuildingUpgradeService::checkUpgrading(const BaseBuilding* building) const
{
    return !building->isUpgrading();
}

bool BuildingUpgradeService::checkResource(const BaseBuilding* building) const
{
    auto& resMgr = ResourceManager::getInstance();
    int cost = building->getUpgradeCost();
    ResourceType costType = building->getUpgradeCostType();
    return resMgr.hasEnough(costType, cost);
}

bool BuildingUpgradeService::checkBuilder(const BaseBuilding* building) const
{
    auto* upgradeMgr = UpgradeManager::getInstance();
    return upgradeMgr->canStartUpgrade(const_cast<BaseBuilding*>(building), true);
}
