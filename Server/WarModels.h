/****************************************************************
 * Project Name:  Clash_of_Clans
 * File Name:     
 * File Function: 
 * Author:        赵崇治
 * Update Date:   2025/12/17
 * License:       MIT License
 ****************************************************************/
#pragma once
#include <string>
#include <vector>
#include <map>
#include <chrono>
#include "ClanInfo.h"

struct AttackResult {
    std::string attackerId;
    std::string defenderId;
    int starsEarned = 0;
    int goldLooted = 0;
    int elixirLooted = 0;
    int trophyChange = 0;
    std::string replayData;
};

struct AttackRecord {
    std::string attackerId;
    std::string attackerName;
    int starsEarned = 0;
    float destructionRate = 0.0f;
    std::chrono::steady_clock::time_point attackTime;
};

struct PvpSession {
    std::string attackerId;
    std::string defenderId;
    std::vector<std::string> spectatorIds;
    std::string mapData;
    bool isActive = true;
};

struct ClanWarMember {
    std::string memberId;
    std::string memberName;
    std::string mapData;
    int bestStars = 0;
    float bestDestructionRate = 0.0f;
    std::vector<AttackRecord> attacksReceived;
};

struct ClanWarSession {
    std::string warId;
    std::string clan1Id;
    std::string clan2Id;
    std::vector<ClanWarMember> clan1Members;
    std::vector<ClanWarMember> clan2Members;
    std::map<std::string, PvpSession> activeBattles;
    int clan1TotalStars = 0;
    int clan2TotalStars = 0;
    std::chrono::steady_clock::time_point startTime;
    std::chrono::steady_clock::time_point endTime;
    bool isActive = true;
};