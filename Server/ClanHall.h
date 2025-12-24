/****************************************************************
 * Project Name:  Clash_of_Clans
 * File Name:     ClanHall.h
 * File Function: 负责管理部落的创建、加入、离开及信息查询
 * Author:        赵崇治
 * Update Date:   2025/12/19
 * License:       MIT License
 ****************************************************************/
#pragma once
#include <map>
#include <mutex>
#include <string>
#include "ClanInfo.h"
#include "PlayerRegistry.h"

class ClanHall {
public:
    explicit ClanHall(PlayerRegistry* registry);

    bool CreateClan(const std::string& playerId, const std::string& clanName);
    bool JoinClan(const std::string& playerId, const std::string& clanId);
    bool LeaveClan(const std::string& playerId);
    
    std::string GetClanListJson();
    std::string GetClanMembersJson(const std::string& clanId);
    
    // 供其他模块查询
    bool IsPlayerInClan(const std::string& playerId, const std::string& clanId);
    std::vector<std::string> GetClanMemberIds(const std::string& clanId);

private:
    std::map<std::string, ClanInfo> clans;
    std::mutex clanMutex;
    PlayerRegistry* playerRegistry;
    
    std::string GenerateClanId();
};