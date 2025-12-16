/****************************************************************
 * Project Name:  Clash_of_Clans
 * File Name:
 * File Function:
 * Author:        赵崇治
 * Update Date:   2025/12/17
 * License:       MIT License
 ****************************************************************/
#pragma once
#include <WinSock2.h>
#include <chrono>
#include <string>
#include <vector>

struct PlayerContext
{
    SOCKET                                socket = INVALID_SOCKET;
    std::string                           playerId;
    std::string                           playerName;
    std::string                           clanId;
    std::string                           mapData;
    int                                   trophies         = 0;
    int                                   gold             = 1000;
    int                                   elixir           = 1000;
    bool                                  isSearchingMatch = false;
    std::chrono::steady_clock::time_point matchStartTime;
};

struct ClanInfo
{
    std::string              clanId;
    std::string              clanName;
    std::string              leaderId;
    std::string              description;
    std::vector<std::string> memberIds;
    int                      clanTrophies     = 0;
    int                      requiredTrophies = 0;
    bool                     isOpen           = true;
};

struct MatchQueueEntry
{
    SOCKET                                socket;
    std::string                           playerId;
    int                                   trophies;
    std::chrono::steady_clock::time_point queueTime;
};