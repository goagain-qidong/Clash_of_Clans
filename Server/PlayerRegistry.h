/****************************************************************
 * Project Name:  Clash_of_Clans
 * File Name:     
 * File Function: 
 * Author:        赵崇治
 * Update Date:   2025/12/19
 * License:       MIT License
 ****************************************************************/
#pragma once
#include <map>
#include <mutex>
#include <string>
#include <WinSock2.h>
#include "ClanInfo.h" 

class PlayerRegistry {
public:
    void Register(SOCKET s, const PlayerContext& ctx);
    void Unregister(SOCKET s);
    
    PlayerContext* GetBySocket(SOCKET s);
    PlayerContext* GetById(const std::string& playerId);
    
    // 只提供只读的列表数据用于生成JSON
    std::map<SOCKET, PlayerContext> GetAllSnapshot();

private:
    std::map<SOCKET, PlayerContext> players;
    std::mutex registryMutex; 
};