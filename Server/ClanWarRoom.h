/****************************************************************
 * Project Name:  Clash_of_Clans
 * File Name:     
 * File Function: 
 * Author:        赵崇治
 * Update Date:   2025/12/17
 * License:       MIT License
 ****************************************************************/
#pragma once
#include <map>
#include <mutex>
#include <vector>
#include <string>
#include "WarModels.h"
#include "PlayerRegistry.h"
#include "ClanHall.h"

class ClanWarRoom {
public:
    ClanWarRoom(PlayerRegistry* registry, ClanHall* hall);

    void AddToQueue(const std::string& clanId);
    void HandleAttackStart(SOCKET clientSocket, const std::string& warId, const std::string& targetId);
    void HandleAttackEnd(const std::string& warId, const AttackRecord& record);
    void HandleSpectate(SOCKET clientSocket, const std::string& warId, const std::string& targetId);
    
    std::string GetMemberListJson(const std::string& warId, const std::string& requesterId);

private:
    std::map<std::string, ClanWarSession> activeWars;
    std::vector<std::string> warQueue;
    std::mutex warMutex;
    std::mutex sessionMutex;
    
    PlayerRegistry* playerRegistry;
    ClanHall* clanHall;

    std::string GenerateWarId();
    void ProcessQueue();
    void StartWar(const std::string& clan1Id, const std::string& clan2Id);
    void BroadcastWarUpdate(const std::string& warId);
};