/****************************************************************
 * Project Name:  Clash_of_Clans
 * File Name:     AccountManager.cpp
 * File Function: 账户管理器（重构版）
 * Author:        赵崇治
 * Update Date:   2025/12/24
 * License:       MIT License
 ****************************************************************/
#include "AccountManager.h"
#include "BuildingCapacityManager.h"
#include "DefenseLogSystem.h"
#include "GameDataSerializer.h"
#include "ResourceManager.h"
#include "TroopInventory.h"
#include "cocos2d.h"

#include <algorithm>

using namespace cocos2d;

// ==================== AccountManager 单例 ====================

AccountManager& AccountManager::getInstance()
{
    static AccountManager instance;
    return instance;
}

bool AccountManager::initialize()
{
    loadFromStorage();

    if (_activeIndex >= 0 && _activeIndex < static_cast<int>(_accounts.size()))
    {
        loadGameStateForUser(_accounts[_activeIndex].account.userId);
        DefenseLogSystem::getInstance().load();
        CCLOG("📂 Initialized with account: %s", _accounts[_activeIndex].account.userId.c_str());
    }

    return _activeIndex >= 0 && _activeIndex < static_cast<int>(_accounts.size());
}

const AccountInfo* AccountManager::getCurrentAccount() const
{
    if (_activeIndex >= 0 && _activeIndex < static_cast<int>(_accounts.size()))
    {
        return &_accounts[_activeIndex];
    }
    return nullptr;
}

bool AccountManager::switchAccount(const std::string& userId, bool silent)
{
    // 保存当前账号状态
    if (_activeIndex >= 0 && _activeIndex < static_cast<int>(_accounts.size()))
    {
        DefenseLogSystem::getInstance().save();
        saveCurrentGameState();
        CCLOG("💾 Saved state for: %s", _accounts[_activeIndex].account.userId.c_str());
    }

    // 清理全局状态
    BuildingCapacityManager::getInstance().clearAllBuildings();
    TroopInventory::getInstance().clearAll();

    // 查找并切换账号
    for (size_t i = 0; i < _accounts.size(); ++i)
    {
        if (_accounts[i].account.userId == userId)
        {
            _activeIndex = static_cast<int>(i);
            save();

            loadGameStateForUser(userId);
            DefenseLogSystem::getInstance().load();
            CCLOG("📂 Switched to account: %s", userId.c_str());

            // 显示防守日志
            if (!silent && DefenseLogSystem::getInstance().hasUnviewedLogs())
            {
                auto director = Director::getInstance();
                if (director && director->getRunningScene())
                {
                    director->getScheduler()->schedule(
                        [](float) { DefenseLogSystem::getInstance().showDefenseLogUI(); }, director->getRunningScene(),
                        1.0f, 0, 0.0f, false, "show_defense_log_after_switch");
                }
            }
            return true;
        }
    }
    return false;
}

bool AccountManager::verifyPassword(const std::string& userId, const std::string& password) const
{
    for (const auto& info : _accounts)
    {
        if (info.account.userId == userId)
        {
            return info.account.password == password;
        }
    }
    return false;
}

void AccountManager::upsertAccount(const AccountData& acc)
{
    auto it = std::find_if(_accounts.begin(), _accounts.end(),
                           [&](const AccountInfo& info) { return info.account.userId == acc.userId; });

    if (it != _accounts.end())
    {
        // 🔴 修复：只更新账户数据，不触发游戏状态保存
        // 游戏状态应该由调用者显式保存（如 BuildingManager::saveCurrentState）
        it->account  = acc;
        _activeIndex = static_cast<int>(std::distance(_accounts.begin(), it));
        
        // 只保存账户列表（不保存游戏状态）
        save();
    }
    else
    {
        // 新账户：需要初始化游戏状态
        AccountInfo newInfo;
        newInfo.account = acc;

        // 随机分配地图
        if (newInfo.account.assignedMapName.empty())
        {
            const std::vector<std::string> maps = {"map/Map1.png", "map/Map2.png", "map/Map3.png"};
            newInfo.account.assignedMapName     = maps[rand() % maps.size()];
        }

        // 确保新账号的士兵库存为空
        TroopInventory::getInstance().clearAll();
        newInfo.gameState.troopInventoryJson = TroopInventory::getInstance().toJson();

        _accounts.push_back(newInfo);
        _activeIndex = static_cast<int>(_accounts.size()) - 1;
        
        save();
        saveCurrentGameState();  // 只有新账户才需要保存初始游戏状态
    }
}

void AccountManager::upsertAccount(const AccountInfo& info)
{
    // 向后兼容：从 AccountInfo 提取 AccountData 并调用主方法
    upsertAccount(info.account);
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

bool AccountManager::deleteAccount(const std::string& userId)
{
    auto it = std::find_if(_accounts.begin(), _accounts.end(),
                           [&](const AccountInfo& info) { return info.account.userId == userId; });

    if (it == _accounts.end())
    {
        CCLOG("❌ Account not found: %s", userId.c_str());
        return false;
    }

    int  indexToDelete     = static_cast<int>(std::distance(_accounts.begin(), it));
    bool isDeletingCurrent = (indexToDelete == _activeIndex);

    // 删除游戏数据文件
    GameDataRepository::getInstance().deleteGameState(userId);

    _accounts.erase(it);

    if (isDeletingCurrent)
    {
        _activeIndex = -1;
    }
    else if (_activeIndex > indexToDelete)
    {
        _activeIndex--;
    }

    save();
    CCLOG("✅ Account deleted: %s", userId.c_str());
    return true;
}

// ==================== 游戏状态管理 ====================

void AccountManager::updateGameState(const GameStateData& state)
{
    if (_activeIndex >= 0 && _activeIndex < static_cast<int>(_accounts.size()))
    {
        _accounts[_activeIndex].gameState = state;
        saveCurrentGameState();
    }
}

GameStateData AccountManager::getCurrentGameState() const
{
    if (_activeIndex >= 0 && _activeIndex < static_cast<int>(_accounts.size()))
    {
        return _accounts[_activeIndex].gameState;
    }
    return GameStateData();
}

bool AccountManager::saveCurrentGameState()
{
    if (_activeIndex < 0 || _activeIndex >= static_cast<int>(_accounts.size()))
    {
        return false;
    }

    auto& info = _accounts[_activeIndex];

    // 同步士兵库存
    info.gameState.troopInventoryJson = TroopInventory::getInstance().toJson();

    return GameDataRepository::getInstance().saveGameState(info.account.userId, info.gameState);
}

bool AccountManager::loadGameStateForUser(const std::string& userId)
{
    for (auto& info : _accounts)
    {
        if (info.account.userId == userId)
        {
            info.gameState = GameDataRepository::getInstance().loadGameState(userId);

            auto& resMgr = ResourceManager::getInstance();

            // 先设置容量
            resMgr.setResourceCapacity(ResourceType::kGold, info.gameState.resources.goldCapacity);
            resMgr.setResourceCapacity(ResourceType::kElixir, info.gameState.resources.elixirCapacity);

            // 再设置数量
            resMgr.setResourceCount(ResourceType::kGold, info.gameState.resources.gold);
            resMgr.setResourceCount(ResourceType::kElixir, info.gameState.resources.elixir);
            resMgr.setResourceCount(ResourceType::kGem, info.gameState.resources.gems);

            // 修复：从保存的数据中恢复士兵库存
            auto& troopInv = TroopInventory::getInstance();
            troopInv.clearAll();
            if (!info.gameState.troopInventoryJson.empty())
            {
                troopInv.fromJson(info.gameState.troopInventoryJson);
                CCLOG("✅ Troop inventory loaded: %s", info.gameState.troopInventoryJson.c_str());
            }

            CCLOG("✅ Game state loaded for: %s", userId.c_str());
            return true;
        }
    }
    return false;
}

GameStateData AccountManager::getPlayerGameState(const std::string& userId) const
{
    // 当前账号返回内存数据
    if (_activeIndex >= 0 && _activeIndex < static_cast<int>(_accounts.size()))
    {
        if (_accounts[_activeIndex].account.userId == userId)
        {
            return _accounts[_activeIndex].gameState;
        }
    }

    // 其他账号从文件加载
    return GameDataRepository::getInstance().loadGameState(userId);
}

std::string AccountManager::exportGameStateJson() const
{
    if (_activeIndex >= 0 && _activeIndex < static_cast<int>(_accounts.size()))
    {
        return GameDataSerializer::serializeGameState(_accounts[_activeIndex].gameState);
    }
    return "{}";
}

bool AccountManager::importGameStateJson(const std::string& userId, const std::string& jsonData)
{
    for (auto& info : _accounts)
    {
        if (info.account.userId == userId)
        {
            info.gameState = GameDataSerializer::deserializeGameState(jsonData);
            return GameDataRepository::getInstance().saveGameState(userId, info.gameState);
        }
    }
    return false;
}

// ==================== 存储 ====================

void AccountManager::save()
{
    std::vector<AccountData> accountList;
    for (const auto& info : _accounts)
    {
        accountList.push_back(info.account);
    }
    GameDataRepository::getInstance().saveAccountList(accountList, _activeIndex);
}

void AccountManager::loadFromStorage()
{
    std::vector<AccountData> accountList;
    int                      activeIdx = -1;

    GameDataRepository::getInstance().loadAccountList(accountList, activeIdx);

    _accounts.clear();
    for (const auto& acc : accountList)
    {
        AccountInfo info;
        info.account = acc;
        _accounts.push_back(info);
    }

    _activeIndex = activeIdx;
    if (_activeIndex < 0 || _activeIndex >= static_cast<int>(_accounts.size()))
    {
        _activeIndex = _accounts.empty() ? -1 : 0;
    }
}