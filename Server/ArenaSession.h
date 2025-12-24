/****************************************************************
 * Project Name:  Clash_of_Clans
 * File Name:     ArenaSession.h
 * File Function: PVP竞技场会话管理
 * Author:        赵崇治
 * Update Date:   2025/12/19
 * License:       MIT License
 ****************************************************************/
#pragma once
#include "PlayerRegistry.h"
#include "WarModels.h"
#include <map>
#include <mutex>
#include <string>

class ArenaSession
{
public:
    explicit ArenaSession(PlayerRegistry* registry);

    void HandlePvpRequest(SOCKET clientSocket, const std::string& targetId);
    void HandlePvpAction(SOCKET clientSocket, const std::string& actionData);
    void HandleSpectateRequest(SOCKET clientSocket, const std::string& targetId);
    void EndSession(const std::string& attackerId);

    // 🆕 清理玩家相关的所有会话（防止残留）
    void CleanupPlayerSessions(const std::string& playerId);

    std::string GetBattleStatusListJson();
    void        BroadcastBattleStatusToAll();

private:
    std::map<std::string, PvpSession> sessions;
    std::mutex                        sessionMutex;
    PlayerRegistry*                   playerRegistry;
};