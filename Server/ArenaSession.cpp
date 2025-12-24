/****************************************************************
 * Project Name:  Clash_of_Clans
 * File Name:     ArenaSession.cpp
 * File Function: PVP竞技场会话管理实现
 * Author:        赵崇治
 * Update Date:   2025/12/19
 * License:       MIT License
 ****************************************************************/
#include "ArenaSession.h"
#include "Protocol.h"
#include <iostream>
#include <sstream>

extern bool sendPacket(SOCKET socket, uint32_t type, const std::string& data);

ArenaSession::ArenaSession(PlayerRegistry* registry) : playerRegistry(registry) {}

void ArenaSession::HandlePvpRequest(SOCKET clientSocket, const std::string& targetId)
{
    PlayerContext* requester = playerRegistry->GetBySocket(clientSocket);
    if (!requester)
    {
        sendPacket(clientSocket, PACKET_PVP_START, "FAIL|NOT_LOGGED_IN|");
        return;
    }

    std::string requesterId = requester->playerId;

    PlayerContext* target = playerRegistry->GetById(targetId);
    if (!target)
    {
        sendPacket(clientSocket, PACKET_PVP_START, "FAIL|TARGET_OFFLINE|");
        return;
    }

    std::string targetMapData = target->mapData;
    if (targetMapData.empty())
    {
        sendPacket(clientSocket, PACKET_PVP_START, "FAIL|NO_MAP|");
        return;
    }

    {
        std::lock_guard<std::mutex> lock(sessionMutex);

        if (sessions.find(requesterId) != sessions.end() || sessions.find(targetId) != sessions.end())
        {
            sendPacket(clientSocket, PACKET_PVP_START, "FAIL|ALREADY_IN_BATTLE|");
            return;
        }

        PvpSession session;
        session.attackerId = requesterId;
        session.defenderId = targetId;
        session.mapData    = targetMapData;
        session.isActive   = true;

        sessions[requesterId] = session;

        std::cout << "[PVP] Session created: " << requesterId << " vs " << targetId << std::endl;
    }

    std::string attackerMsg = "ATTACK|" + targetId + "|" + targetMapData;
    sendPacket(clientSocket, PACKET_PVP_START, attackerMsg);

    std::string defenderMsg = "DEFEND|" + requesterId + "|";
    sendPacket(target->socket, PACKET_PVP_START, defenderMsg);

    BroadcastBattleStatusToAll();
}

void ArenaSession::HandlePvpAction(SOCKET clientSocket, const std::string& actionData)
{
    PlayerContext* player = playerRegistry->GetBySocket(clientSocket);
    if (!player)
        return;

    std::string playerId = player->playerId;
    PvpSession  sessionCopy;
    bool        found = false;

    {
        std::lock_guard<std::mutex> lock(sessionMutex);
        auto                        it = sessions.find(playerId);
        if (it != sessions.end())
        {
            // 🆕 Store action in history
            it->second.actionHistory.push_back(actionData);
            
            sessionCopy = it->second;
            found       = true;
        }
    }

    if (!found)
        return;

    // 广播给防守方和观战者
    PlayerContext* defender = playerRegistry->GetById(sessionCopy.defenderId);
    if (defender && defender->socket != INVALID_SOCKET)
    {
        sendPacket(defender->socket, PACKET_PVP_ACTION, actionData);
    }

    for (const auto& spectatorId : sessionCopy.spectatorIds)
    {
        PlayerContext* spectator = playerRegistry->GetById(spectatorId);
        if (spectator && spectator->socket != INVALID_SOCKET)
        {
            sendPacket(spectator->socket, PACKET_PVP_ACTION, actionData);
        }
    }
}

void ArenaSession::HandleSpectateRequest(SOCKET clientSocket, const std::string& targetId)
{
    PlayerContext* requester = playerRegistry->GetBySocket(clientSocket);
    if (!requester)
    {
        sendPacket(clientSocket, PACKET_SPECTATE_JOIN, "0|||");
        return;
    }

    std::string spectatorId = requester->playerId;
    std::string attackerId, defenderId, mapData;
    std::vector<std::string> history;
    bool        found = false;

    {
        std::lock_guard<std::mutex> lock(sessionMutex);

        for (auto& pair : sessions)
        {
            if (pair.second.attackerId == targetId || pair.second.defenderId == targetId)
            {
                attackerId = pair.second.attackerId;
                defenderId = pair.second.defenderId;
                mapData    = pair.second.mapData;
                history    = pair.second.actionHistory; // 🆕 Copy history

                if (pair.second.isActive)
                {
                    pair.second.spectatorIds.push_back(spectatorId);
                }
                found = true;
                break;
            }
        }
    }

    if (!found || mapData.empty())
    {
        sendPacket(clientSocket, PACKET_SPECTATE_JOIN, "0|||");
        return;
    }

    std::cout << "[Spectate] " << spectatorId << " watching " << attackerId << " vs " << defenderId << std::endl;

    std::string response = "1|" + attackerId + "|" + defenderId + "|" + mapData;
    
    // 🆕 Append history with special delimiter
    if (!history.empty()) {
        response += "|||HISTORY|||";
        for (const auto& action : history) {
            response += action + "|||";
        }
    }
    
    sendPacket(clientSocket, PACKET_SPECTATE_JOIN, response);
}

void ArenaSession::EndSession(const std::string& attackerId)
{
    PvpSession sessionCopy;
    bool       found = false;

    {
        std::lock_guard<std::mutex> lock(sessionMutex);
        auto                        it = sessions.find(attackerId);
        if (it == sessions.end())
            return;

        sessionCopy = it->second;
        found       = true;
        sessions.erase(it);

        std::cout << "[PVP] Session ended: " << attackerId << std::endl;
    }

    if (!found)
        return;

    // 🔧 修复：通知防守方战斗结束
    PlayerContext* defender = playerRegistry->GetById(sessionCopy.defenderId);
    if (defender && defender->socket != INVALID_SOCKET)
    {
        sendPacket(defender->socket, PACKET_PVP_END, "BATTLE_ENDED");
        std::cout << "[PVP] Notified defender: " << sessionCopy.defenderId << std::endl;
    }

    // 🔧 修复：通知所有观战者战斗结束
    for (const auto& spectatorId : sessionCopy.spectatorIds)
    {
        PlayerContext* spectator = playerRegistry->GetById(spectatorId);
        if (spectator && spectator->socket != INVALID_SOCKET)
        {
            sendPacket(spectator->socket, PACKET_PVP_END, "BATTLE_ENDED");
            std::cout << "[PVP] Notified spectator: " << spectatorId << std::endl;
        }
    }

    // 🔧 修复：广播更新的战斗状态列表（移除已结束的战斗）
    BroadcastBattleStatusToAll();
}

// 🆕 新增：清理玩家相关的所有PVP会话（防止残留）
void ArenaSession::CleanupPlayerSessions(const std::string& playerId)
{
    std::lock_guard<std::mutex> lock(sessionMutex);

    // 查找并清理作为攻击方的会话
    auto it = sessions.find(playerId);
    if (it != sessions.end())
    {
        std::cout << "[PVP] Cleanup session for attacker: " << playerId << std::endl;

        // 通知防守方和观战者
        PvpSession& session = it->second;

        PlayerContext* defender = playerRegistry->GetById(session.defenderId);
        if (defender && defender->socket != INVALID_SOCKET)
        {
            sendPacket(defender->socket, PACKET_PVP_END, "OPPONENT_DISCONNECTED");
        }

        for (const auto& spectatorId : session.spectatorIds)
        {
            PlayerContext* spectator = playerRegistry->GetById(spectatorId);
            if (spectator && spectator->socket != INVALID_SOCKET)
            {
                sendPacket(spectator->socket, PACKET_PVP_END, "BATTLE_ENDED");
            }
        }

        sessions.erase(it);
    }

    // 查找并清理作为防守方或观战者的会话
    for (auto it = sessions.begin(); it != sessions.end();)
    {
        auto& session = it->second;

        // 从观战者列表中移除
        auto spectatorIt = std::find(session.spectatorIds.begin(), session.spectatorIds.end(), playerId);
        if (spectatorIt != session.spectatorIds.end())
        {
            session.spectatorIds.erase(spectatorIt);
            std::cout << "[PVP] Removed spectator from session: " << playerId << std::endl;
        }

        // 如果是防守方断开，结束会话
        if (session.defenderId == playerId)
        {
            std::cout << "[PVP] Defender disconnected, ending session: " << it->first << std::endl;

            PlayerContext* attacker = playerRegistry->GetById(session.attackerId);
            if (attacker && attacker->socket != INVALID_SOCKET)
            {
                sendPacket(attacker->socket, PACKET_PVP_END, "DEFENDER_DISCONNECTED");
            }

            for (const auto& spectatorId : session.spectatorIds)
            {
                PlayerContext* spectator = playerRegistry->GetById(spectatorId);
                if (spectator && spectator->socket != INVALID_SOCKET)
                {
                    sendPacket(spectator->socket, PACKET_PVP_END, "BATTLE_ENDED");
                }
            }

            it = sessions.erase(it);
        }
        else
        {
            ++it;
        }
    }
}

std::string ArenaSession::GetBattleStatusListJson()
{
    std::lock_guard<std::mutex> lock(sessionMutex);

    std::ostringstream oss;
    oss << "{\"statuses\":[";

    bool first = true;
    for (const auto& pair : sessions)
    {
        const PvpSession& session = pair.second;
        if (!session.isActive)
            continue;

        if (!first)
            oss << ",";
        first = false;
        oss << "{\"userId\":\"" << session.attackerId << "\","
            << "\"inBattle\":true,"
            << "\"opponentId\":\"" << session.defenderId << "\","
            << "\"isAttacker\":true}";

        oss << ",{\"userId\":\"" << session.defenderId << "\","
            << "\"inBattle\":true,"
            << "\"opponentId\":\"" << session.attackerId << "\","
            << "\"isAttacker\":false}";
    }

    oss << "]}";
    return oss.str();
}

void ArenaSession::BroadcastBattleStatusToAll()
{
    std::string statusJson = GetBattleStatusListJson();

    auto allPlayers = playerRegistry->GetAllSnapshot();
    for (const auto& pair : allPlayers)
    {
        if (!pair.second.playerId.empty())
        {
            sendPacket(pair.first, PACKET_BATTLE_STATUS_LIST, statusJson);
        }
    }
}