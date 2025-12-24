/****************************************************************
 * Project Name:  Clash_of_Clans
 * File Name:     PlayerRegistry.cpp
 * File Function: 玩家注册管理实现
 * Author:        赵崇治
 * Update Date:   2025/12/19
 * License:       MIT License
 ****************************************************************/
#include "PlayerRegistry.h"

void PlayerRegistry::Register(SOCKET s, const PlayerContext& ctx)
{
    std::lock_guard<std::mutex> lock(registryMutex);
    players[s] = ctx;
}

void PlayerRegistry::Unregister(SOCKET s)
{
    std::lock_guard<std::mutex> lock(registryMutex);
    players.erase(s);
}

PlayerContext* PlayerRegistry::GetBySocket(SOCKET s)
{
    std::lock_guard<std::mutex> lock(registryMutex);
    auto it = players.find(s);
    return it != players.end() ? &it->second : nullptr;
}

PlayerContext* PlayerRegistry::GetById(const std::string& playerId)
{
    std::lock_guard<std::mutex> lock(registryMutex);
    for (auto& pair : players)
    {
        if (pair.second.playerId == playerId)
        {
            return &pair.second;
        }
    }
    return nullptr;
}

std::map<SOCKET, PlayerContext> PlayerRegistry::GetAllSnapshot()
{
    std::lock_guard<std::mutex> lock(registryMutex);
    return players;  // 返回副本
}