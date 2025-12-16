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
#include <string>
#include <WinSock2.h>
#include "WarModels.h"
#include "PlayerRegistry.h"

class Arena {
public:
    explicit Arena(PlayerRegistry* registry);

    void HandlePvpRequest(SOCKET clientSocket, const std::string& targetId);
    void HandlePvpAction(SOCKET clientSocket, const std::string& actionData);
    void HandleSpectateRequest(SOCKET clientSocket, const std::string& targetId);
    void EndPvpSession(const std::string& attackerId);
    
    std::string GetBattleStatusListJson();

private:
    // key: attackerId
    std::map<std::string, PvpSession> sessions;
    std::mutex arenaMutex;
    PlayerRegistry* playerRegistry;
};