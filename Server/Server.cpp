/****************************************************************
 * Project Name:  Clash_of_Clans
 * File Name:     Server.cpp
 * File Function: 服务器主逻辑
 * Author:        赵崇治
 * Update Date:   2025/12/14
 * License:       MIT License
 ****************************************************************/
#define _CRT_SECURE_NO_WARNINGS
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#pragma comment(lib, "Ws2_32.lib")
#include "Server.h"
#include <algorithm>
#include <iostream>
#include <random>
#include <sstream>
#include <thread>
#include <vector>

// ==================== 网络基础函数 ====================

bool Server::recvFixedAmount(SOCKET socket, char* buffer, int totalBytes)

{
    int received = 0;

    while (received < totalBytes)

    {
        int ret = recv(socket, buffer + received, totalBytes - received, 0);

        if (ret <= 0)

        {
            return false;
        }

        received += ret;
    }

    return true;
}

bool Server::sendPacket(SOCKET socket, uint32_t type, const std::string& data)

{
    PacketHeader header;

    header.type = type;

    header.length = static_cast<uint32_t>(data.size());

    int headerSent = send(socket, reinterpret_cast<char*>(&header), sizeof(PacketHeader), 0);

    if (headerSent != sizeof(PacketHeader))

    {
        return false;
    }

    if (header.length > 0)

    {
        int bodySent = send(socket, data.c_str(), static_cast<int>(header.length), 0);

        if (bodySent != static_cast<int>(header.length))

        {
            return false;
        }
    }

    return true;
}

bool Server::recvPacket(SOCKET socket, uint32_t& outType, std::string& outData)

{
    PacketHeader header;

    if (!recvFixedAmount(socket, reinterpret_cast<char*>(&header), sizeof(PacketHeader)))

    {
        return false;
    }

    outType = header.type;

    outData.clear();

    if (header.length > 0)

    {
        std::vector<char> buffer(header.length);

        if (!recvFixedAmount(socket, buffer.data(), static_cast<int>(header.length)))

        {
            return false;
        }

        outData.assign(buffer.begin(), buffer.end());
    }

    return true;
}

// ==================== 辅助函数 ====================

std::string Server::serializeAttackResult(const AttackResult& result)

{
    std::ostringstream oss;

    oss << result.attackerId << "|" << result.defenderId << "|" << result.starsEarned << "|" << result.goldLooted << "|"

        << result.elixirLooted << "|" << result.trophyChange << "|" << result.replayData;

    return oss.str();
}

AttackResult Server::deserializeAttackResult(const std::string& data)

{
    AttackResult result;

    std::istringstream iss(data);

    std::string token;

    std::getline(iss, result.attackerId, '|');

    std::getline(iss, result.defenderId, '|');

    std::getline(iss, token, '|');

    result.starsEarned = std::stoi(token);

    std::getline(iss, token, '|');

    result.goldLooted = std::stoi(token);

    std::getline(iss, token, '|');

    result.elixirLooted = std::stoi(token);

    std::getline(iss, token, '|');

    result.trophyChange = std::stoi(token);

    std::getline(iss, result.replayData);

    return result;
}

SOCKET Server::findSocketByPlayerId(const std::string& playerId)

{
    for (const auto& pair : onlinePlayers)

    {
        if (pair.second.playerId == playerId)

        {
            return pair.first;
        }
    }

    return INVALID_SOCKET;
}

// 🆕 PVP系统实现

void Server::handlePvpRequest(SOCKET clientSocket, const std::string& targetId)
{
    std::string requesterId;
    std::string targetMapData;
    SOCKET targetSocket = INVALID_SOCKET;
    
    // 第一步：获取必要数据（短时间持锁）
    {
        std::lock_guard<std::mutex> lock(dataMutex);
        
        auto requesterIt = onlinePlayers.find(clientSocket);
        if (requesterIt == onlinePlayers.end())
        {
            std::cout << "[PVP] Requester not found" << std::endl;
            sendPacket(clientSocket, PACKET_PVP_START, "FAIL|NOT_LOGGED_IN|");
            return;
        }
        
        requesterId = requesterIt->second.playerId;
        
        // 查找目标玩家
        targetSocket = findSocketByPlayerId(targetId);
        if (targetSocket == INVALID_SOCKET)
        {
            std::cout << "[PVP] Target player " << targetId << " not online" << std::endl;
            sendPacket(clientSocket, PACKET_PVP_START, "FAIL|TARGET_OFFLINE|");
            return;
        }
        
        auto targetIt = onlinePlayers.find(targetSocket);
        if (targetIt == onlinePlayers.end())
        {
            std::cout << "[PVP] Target context missing" << std::endl;
            sendPacket(clientSocket, PACKET_PVP_START, "FAIL|NO_MAP|");
            return;
        }

        // 优先使用在线玩家的缓存地图数据；若为空则从持久化存储中回退
        if (!targetIt->second.mapData.empty())
        {
            targetMapData = targetIt->second.mapData;
        }
        else
        {
            auto savedIt = savedMaps.find(targetId);
            if (savedIt != savedMaps.end() && !savedIt->second.empty())
            {
                targetMapData = savedIt->second;
                targetIt->second.mapData = targetMapData; // 缓存，避免后续再次回退
                std::cout << "[PVP] Target map loaded from savedMaps" << std::endl;
            }
            else
            {
                auto dbIt = playerDatabase.find(targetId);
                if (dbIt != playerDatabase.end() && !dbIt->second.mapData.empty())
                {
                    targetMapData = dbIt->second.mapData;
                    targetIt->second.mapData = targetMapData;
                    std::cout << "[PVP] Target map loaded from playerDatabase" << std::endl;
                }
            }
        }

        if (targetMapData.empty())
        {
            std::cout << "[PVP] Target has no map data" << std::endl;
            sendPacket(clientSocket, PACKET_PVP_START, "FAIL|NO_MAP|");
            return;
        }
    }
    
    // 第二步：检查和创建PVP会话（独立持锁）
    {
        std::lock_guard<std::mutex> pvpLock(pvpMutex);
        
        // 检查是否已经在PVP中
        if (pvpSessions.find(requesterId) != pvpSessions.end() ||
            pvpSessions.find(targetId) != pvpSessions.end())
        {
            sendPacket(clientSocket, PACKET_PVP_START, "FAIL|ALREADY_IN_BATTLE|");
            return;
        }
        
        // 创建PVP会话
        PvpSession session;
        session.attackerId = requesterId;
        session.defenderId = targetId;
        session.mapData = targetMapData;
        session.isActive = true;
        
        pvpSessions[requesterId] = session;
        
        std::cout << "[PVP] Session created: " << requesterId << " vs " << targetId << std::endl;
    }
    
    // 第三步：发送通知（不需要持锁）
    std::string attackerMsg = "ATTACK|" + targetId + "|" + targetMapData;
    sendPacket(clientSocket, PACKET_PVP_START, attackerMsg);
    
    std::string defenderMsg = "DEFEND|" + requesterId + "|";
    sendPacket(targetSocket, PACKET_PVP_START, defenderMsg);

    broadcastBattleStatusToAll();
}

void Server::handlePvpAction(SOCKET clientSocket, const std::string& actionData)
{
    std::string playerId;
    PvpSession sessionCopy;
    bool foundSession = false;
    
    // 第一步：获取玩家ID和会话副本
    {
        std::lock_guard<std::mutex> lock(dataMutex);
        auto it = onlinePlayers.find(clientSocket);
        if (it == onlinePlayers.end()) return;
        playerId = it->second.playerId;
    }
    
    // 第二步：查找PVP会话
    {
        std::lock_guard<std::mutex> pvpLock(pvpMutex);
        auto sessionIt = pvpSessions.find(playerId);
        if (sessionIt == pvpSessions.end()) return;
        sessionCopy = sessionIt->second;
        foundSession = true;
    }
    
    if (!foundSession) return;
    
    // 第三步：广播操作（使用会话副本，不需要持锁）
    {
        std::lock_guard<std::mutex> lock(dataMutex);
        
        SOCKET defenderSocket = findSocketByPlayerId(sessionCopy.defenderId);
        if (defenderSocket != INVALID_SOCKET)
        {
            sendPacket(defenderSocket, PACKET_PVP_ACTION, actionData);
        }
        
        for (const auto& spectatorId : sessionCopy.spectatorIds)
        {
            SOCKET spectatorSocket = findSocketByPlayerId(spectatorId);
            if (spectatorSocket != INVALID_SOCKET)
            {
                sendPacket(spectatorSocket, PACKET_PVP_ACTION, actionData);
            }
        }
    }
}

void Server::handleSpectateRequest(SOCKET clientSocket, const std::string& targetId)
{
    std::string spectatorId;
    PvpSession* foundSession = nullptr;
    std::string attackerId, defenderId, mapData;
    
    // 第一步：获取观战者ID
    {
        std::lock_guard<std::mutex> lock(dataMutex);
        auto requesterIt = onlinePlayers.find(clientSocket);
        if (requesterIt == onlinePlayers.end())
        {
            sendPacket(clientSocket, PACKET_SPECTATE_JOIN, "0|||");
            return;
        }
        spectatorId = requesterIt->second.playerId;
    }
    
    // 第二步：查找目标的PVP会话并添加观战者
    {
        std::lock_guard<std::mutex> pvpLock(pvpMutex);
        
        for (auto& pair : pvpSessions)
        {
            if (pair.second.attackerId == targetId || pair.second.defenderId == targetId)
            {
                foundSession = &pair.second;
                attackerId = pair.second.attackerId;
                defenderId = pair.second.defenderId;
                mapData = pair.second.mapData;
                
                if (foundSession->isActive)
                {
                    foundSession->spectatorIds.push_back(spectatorId);
                }
                break;
            }
        }
    }
    
    if (!foundSession || mapData.empty())
    {
        sendPacket(clientSocket, PACKET_SPECTATE_JOIN, "0|||");
        return;
    }
    
    std::cout << "[Spectate] " << spectatorId << " watching " << attackerId << " vs " << defenderId << std::endl;
    
    std::string response = "1|" + attackerId + "|" + defenderId + "|" + mapData;
    sendPacket(clientSocket, PACKET_SPECTATE_JOIN, response);
}

void Server::endPvpSession(const std::string& attackerId)
{
    PvpSession sessionCopy;
    bool foundSession = false;
    
    // 第一步：获取会话副本并删除
    {
        std::lock_guard<std::mutex> pvpLock(pvpMutex);
        auto it = pvpSessions.find(attackerId);
        if (it == pvpSessions.end()) return;
        
        sessionCopy = it->second;
        foundSession = true;
        pvpSessions.erase(it);
        
        std::cout << "[PVP] Session ended: " << attackerId << std::endl;
    }
    
    if (!foundSession) return;
    
    // 第二步：通知相关玩家（使用副本数据）
    {
        std::lock_guard<std::mutex> dataLock(dataMutex);
        
        SOCKET defenderSocket = findSocketByPlayerId(sessionCopy.defenderId);
        if (defenderSocket != INVALID_SOCKET)
        {
            sendPacket(defenderSocket, PACKET_PVP_END, "BATTLE_ENDED");
        }
        
        for (const auto& spectatorId : sessionCopy.spectatorIds)
        {
            SOCKET spectatorSocket = findSocketByPlayerId(spectatorId);
            if (spectatorSocket != INVALID_SOCKET)
            {
                sendPacket(spectatorSocket, PACKET_PVP_END, "BATTLE_ENDED");
            }
        }
    }
    broadcastBattleStatusToAll();
}

ClanWarSession* Server::getClanWarSession(const std::string& warId)
{
    std::lock_guard<std::mutex> lock(clanWarSessionMutex);
    auto it = clanWarSessions.find(warId);
    return it != clanWarSessions.end() ? &it->second : nullptr;
}

void Server::initClanWarMembers(ClanWarSession& session)
{
    // 注意：此函数在调用时，调用者应该已经持有 clanWarSessionMutex
    // 但需要获取 clanMutex 和 dataMutex 来访问部落成员和玩家数据
    
    std::lock_guard<std::mutex> clanLock(clanMutex);
    std::lock_guard<std::mutex> dataLock(dataMutex);
    
    // 初始化clan1成员
    auto clan1It = clans.find(session.clan1Id);
    if (clan1It != clans.end())
    {
        for (const auto& memberId : clan1It->second.memberIds)
        {
            ClanWarMember member;
            member.memberId = memberId;
            
            // 从在线玩家或数据库获取地图数据
            SOCKET memberSocket = findSocketByPlayerId(memberId);
            if (memberSocket != INVALID_SOCKET)
            {
                auto playerIt = onlinePlayers.find(memberSocket);
                if (playerIt != onlinePlayers.end())
                {
                    member.memberName = playerIt->second.playerName;
                    member.mapData = playerIt->second.mapData;
                }
            }
            else if (playerDatabase.find(memberId) != playerDatabase.end())
            {
                member.memberName = playerDatabase[memberId].playerName;
                member.mapData = playerDatabase[memberId].mapData;
            }
            
            session.clan1Members.push_back(member);
        }
    }
    
    // 初始化clan2成员
    auto clan2It = clans.find(session.clan2Id);
    if (clan2It != clans.end())
    {
        for (const auto& memberId : clan2It->second.memberIds)
        {
            ClanWarMember member;
            member.memberId = memberId;
            
            SOCKET memberSocket = findSocketByPlayerId(memberId);
            if (memberSocket != INVALID_SOCKET)
            {
                auto playerIt = onlinePlayers.find(memberSocket);
                if (playerIt != onlinePlayers.end())
                {
                    member.memberName = playerIt->second.playerName;
                    member.mapData = playerIt->second.mapData;
                }
            }
            else if (playerDatabase.find(memberId) != playerDatabase.end())
            {
                member.memberName = playerDatabase[memberId].playerName;
                member.mapData = playerDatabase[memberId].mapData;
            }
            
            session.clan2Members.push_back(member);
        }
    }
}

std::string Server::getClanWarMemberListJson(const std::string& warId, const std::string& requesterId)
{
    // 先获取战争会话的副本（避免长时间持锁）
    ClanWarSession sessionCopy;
    bool foundSession = false;
    
    {
        std::lock_guard<std::mutex> lock(clanWarSessionMutex);
        
        auto sessionIt = clanWarSessions.find(warId);
        if (sessionIt == clanWarSessions.end())
        {
            return "{\"error\":\"War not found\"}";
        }
        
        sessionCopy = sessionIt->second;
        foundSession = true;
    }
    
    if (!foundSession)
    {
        return "{\"error\":\"War not found\"}";
    }
    
    // 判断请求者属于哪个部落
    bool isInClan1 = false;
    {
        std::lock_guard<std::mutex> clanLock(clanMutex);
        auto clan1It = clans.find(sessionCopy.clan1Id);
        if (clan1It != clans.end())
        {
            auto& members = clan1It->second.memberIds;
            isInClan1 = std::find(members.begin(), members.end(), requesterId) != members.end();
        }
    }
    
    // 构建JSON（返回敌方成员列表）
    std::ostringstream oss;
    oss << "{";
    oss << "\"warId\":\"" << warId << "\",";
    oss << "\"clan1TotalStars\":" << sessionCopy.clan1TotalStars << ",";
    oss << "\"clan2TotalStars\":" << sessionCopy.clan2TotalStars << ",";
    oss << "\"enemyMembers\":[";
    
    const auto& enemyMembers = isInClan1 ? sessionCopy.clan2Members : sessionCopy.clan1Members;
    bool first = true;
    for (const auto& member : enemyMembers)
    {
        if (!first) oss << ",";
        first = false;
        
        oss << "{";
        oss << "\"id\":\"" << member.memberId << "\",";
        oss << "\"name\":\"" << member.memberName << "\",";
        oss << "\"bestStars\":" << member.bestStars << ",";
        oss << "\"bestDestruction\":" << member.bestDestructionRate << ",";
        oss << "\"canAttack\":" << (!member.mapData.empty() ? "true" : "false");
        oss << "}";
    }
    
    oss << "]}";
    return oss.str();
}

void Server::handleClanWarAttackStart(SOCKET clientSocket, const std::string& warId, const std::string& targetId)
{
    std::string attackerId;
    std::string targetMapData;
    std::string targetIdCopy = targetId;
    
    // 第一步：从dataMutex获取攻击者信息
    {
        std::lock_guard<std::mutex> dataLock(dataMutex);
        auto attackerIt = onlinePlayers.find(clientSocket);
        if (attackerIt == onlinePlayers.end())
        {
            sendPacket(clientSocket, PACKET_CLAN_WAR_ATTACK_START, "FAIL|NOT_LOGGED_IN|");
            return;
        }
        attackerId = attackerIt->second.playerId;
    }
    
    // 第二步：从clanWarSessionMutex获取战争会话和目标地图数据
    {
        std::lock_guard<std::mutex> sessionLock(clanWarSessionMutex);
        
        auto sessionIt = clanWarSessions.find(warId);
        if (sessionIt == clanWarSessions.end())
        {
            sendPacket(clientSocket, PACKET_CLAN_WAR_ATTACK_START, "FAIL|WAR_NOT_FOUND|");
            return;
        }
        
        ClanWarSession& session = sessionIt->second;
        
        // 查找目标成员的地图数据
        ClanWarMember* targetMember = nullptr;
        for (auto& member : session.clan1Members)
        {
            if (member.memberId == targetId)
            {
                targetMember = &member;
                break;
            }
        }
        if (!targetMember)
        {
            for (auto& member : session.clan2Members)
            {
                if (member.memberId == targetId)
                {
                    targetMember = &member;
                    break;
                }
            }
        }
        
        if (!targetMember || targetMember->mapData.empty())
        {
            sendPacket(clientSocket, PACKET_CLAN_WAR_ATTACK_START, "FAIL|NO_MAP_DATA|");
            return;
        }
        
        targetMapData = targetMember->mapData;
        
        // 创建战斗会话（在同一个锁范围内）
        PvpSession pvpSession;
        pvpSession.attackerId = attackerId;
        pvpSession.defenderId = targetId;
        pvpSession.mapData = targetMapData;
        pvpSession.isActive = true;
        
        session.activeBattles[attackerId] = pvpSession;
    }
    
    std::cout << "[ClanWar] Attack started: " << attackerId << " -> " << targetId << " in " << warId << std::endl;
    
    // 第三步：发送响应（不需要持锁）
    std::string response = "ATTACK|" + targetId + "|" + targetMapData;
    sendPacket(clientSocket, PACKET_CLAN_WAR_ATTACK_START, response);
}

void Server::handleClanWarAttackEnd(const std::string& warId, const AttackRecord& record)
{
    std::string defenderId;
    bool isAttackerInClan1 = false;
    bool needBroadcast = false;
    
    // 第一步：在战争会话中更新数据
    {
        std::lock_guard<std::mutex> sessionLock(clanWarSessionMutex);
        
        auto sessionIt = clanWarSessions.find(warId);
        if (sessionIt == clanWarSessions.end()) return;
        
        ClanWarSession& session = sessionIt->second;
        
        // 从activeBattles中获取defenderId
        auto battleIt = session.activeBattles.find(record.attackerId);
        if (battleIt == session.activeBattles.end())
        {
            std::cout << "[ClanWar] Error: No active battle found for attacker " << record.attackerId << std::endl;
            return;
        }
        
        defenderId = battleIt->second.defenderId;
        
        // 检查攻击者所属部落（需要在clanMutex内）
        {
            std::lock_guard<std::mutex> clanLock(clanMutex);
            auto clan1It = clans.find(session.clan1Id);
            if (clan1It != clans.end())
            {
                auto& members = clan1It->second.memberIds;
                isAttackerInClan1 = std::find(members.begin(), members.end(), record.attackerId) != members.end();
            }
        }
        
        // 目标在敌方部落，找到目标成员
        auto& targetMembers = isAttackerInClan1 ? session.clan2Members : session.clan1Members;
        
        ClanWarMember* targetMember = nullptr;
        for (auto& member : targetMembers)
        {
            if (member.memberId == defenderId)
            {
                targetMember = &member;
                break;
            }
        }
        
        if (!targetMember)
        {
            std::cout << "[ClanWar] Error: Target member " << defenderId << " not found" << std::endl;
            session.activeBattles.erase(record.attackerId);
            return;
        }
        
        // 更新目标成员的被攻击记录
        targetMember->attacksReceived.push_back(record);
        
        // 更新最佳战绩
        if (record.starsEarned > targetMember->bestStars ||
            (record.starsEarned == targetMember->bestStars && record.destructionRate > targetMember->bestDestructionRate))
        {
            targetMember->bestStars = record.starsEarned;
            targetMember->bestDestructionRate = record.destructionRate;
        }
        
        // 更新部落总星数
        if (isAttackerInClan1)
        {
            session.clan1TotalStars += record.starsEarned;
        }
        else
        {
            session.clan2TotalStars += record.starsEarned;
        }
        
        std::cout << "[ClanWar] Attack ended: " << record.attackerId << " -> " << defenderId
                  << " earned " << record.starsEarned << " stars ("
                  << (record.destructionRate * 100) << "% destruction)" << std::endl;
        
        // 移除战斗会话
        session.activeBattles.erase(record.attackerId);
        needBroadcast = true;
    }
    
    // 第二步：广播状态更新（在释放sessionLock后调用）
    if (needBroadcast)
    {
        broadcastClanWarStateUpdate(warId);
    }
}

void Server::handleClanWarSpectate(SOCKET clientSocket, const std::string& warId, const std::string& targetId)
{
    std::string spectatorId;
    std::string attackerId, defenderId, mapData;
    bool found = false;
    
    // 第一步：获取观战者信息
    {
        std::lock_guard<std::mutex> dataLock(dataMutex);
        auto spectatorIt = onlinePlayers.find(clientSocket);
        if (spectatorIt == onlinePlayers.end())
        {
            sendPacket(clientSocket, PACKET_CLAN_WAR_SPECTATE, "0|||");
            return;
        }
        spectatorId = spectatorIt->second.playerId;
    }
    
    // 第二步：在战争会话中查找战斗并添加观战者
    {
        std::lock_guard<std::mutex> sessionLock(clanWarSessionMutex);
        
        auto sessionIt = clanWarSessions.find(warId);
        if (sessionIt == clanWarSessions.end())
        {
            sendPacket(clientSocket, PACKET_CLAN_WAR_SPECTATE, "0|||");
            return;
        }
        
        ClanWarSession& session = sessionIt->second;
        
        // 查找目标的战斗会话
        for (auto& pair : session.activeBattles)
        {
            if (pair.second.attackerId == targetId || pair.second.defenderId == targetId)
            {
                attackerId = pair.second.attackerId;
                defenderId = pair.second.defenderId;
                mapData = pair.second.mapData;
                
                // 添加观战者
                pair.second.spectatorIds.push_back(spectatorId);
                found = true;
                break;
            }
        }
    }
    
    if (!found || mapData.empty())
    {
        sendPacket(clientSocket, PACKET_CLAN_WAR_SPECTATE, "0|||");
        return;
    }
    
    std::cout << "[ClanWar] Spectator " << spectatorId 
              << " watching " << attackerId << " vs " << defenderId << std::endl;
    
    // 发送响应
    std::string response = "1|" + attackerId + "|" + defenderId + "|" + mapData;
    sendPacket(clientSocket, PACKET_CLAN_WAR_SPECTATE, response);
}

void Server::broadcastClanWarStateUpdate(const std::string& warId)
{
    std::string stateJson;
    std::vector<std::string> clan1MemberIds;
    std::vector<std::string> clan2MemberIds;
    std::string clan1Id, clan2Id;
    
    // 第一步：获取会话状态
    {
        std::lock_guard<std::mutex> sessionLock(clanWarSessionMutex);
        
        auto sessionIt = clanWarSessions.find(warId);
        if (sessionIt == clanWarSessions.end()) return;
        
        const ClanWarSession& session = sessionIt->second;
        clan1Id = session.clan1Id;
        clan2Id = session.clan2Id;
        
        // 构建状态JSON
        std::ostringstream oss;
        oss << "{";
        oss << "\"warId\":\"" << warId << "\",";
        oss << "\"clan1Stars\":" << session.clan1TotalStars << ",";
        oss << "\"clan2Stars\":" << session.clan2TotalStars;
        oss << "}";
        
        stateJson = oss.str();
    }
    
    // 第二步：获取两个部落的成员ID列表
    {
        std::lock_guard<std::mutex> clanLock(clanMutex);
        
        auto clan1It = clans.find(clan1Id);
        if (clan1It != clans.end())
        {
            clan1MemberIds = clan1It->second.memberIds;
        }
        
        auto clan2It = clans.find(clan2Id);
        if (clan2It != clans.end())
        {
            clan2MemberIds = clan2It->second.memberIds;
        }
    }
    
    // 第三步：发送给所有在线成员
    {
        std::lock_guard<std::mutex> dataLock(dataMutex);
        
        auto sendToMembers = [&](const std::vector<std::string>& memberIds) {
            for (const auto& memberId : memberIds)
            {
                SOCKET memberSocket = findSocketByPlayerId(memberId);
                if (memberSocket != INVALID_SOCKET)
                {
                    sendPacket(memberSocket, PACKET_CLAN_WAR_STATE_UPDATE, stateJson);
                }
            }
        };
        
        sendToMembers(clan1MemberIds);
        sendToMembers(clan2MemberIds);
    }
}

// ==================== 客户端处理 ====================

void clientHandler(SOCKET clientSocket, Server& server)

{
    uint32_t msgType;

    std::string msgData;

    while (Server::recvPacket(clientSocket, msgType, msgData))

    {
        // ⚠️ 不要在整个 switch 开始就持有 dataMutex，而是在每个case内部按需加锁

        switch (msgType)

        {
            // ========== 基础功能 ==========

        case PACKET_LOGIN: {
            std::lock_guard<std::mutex> lock(server.dataMutex);
            // 格式: playerId|playerName|trophies

            std::istringstream iss(msgData);

            std::string playerId, playerName, trophiesStr;

            std::getline(iss, playerId, '|');

            std::getline(iss, playerName, '|');

            std::getline(iss, trophiesStr, '|');

            server.onlinePlayers[clientSocket].playerId = playerId;

            server.onlinePlayers[clientSocket].playerName = playerName.empty() ? playerId : playerName;

            if (!trophiesStr.empty())

            {
                try {
                    server.onlinePlayers[clientSocket].trophies = std::stoi(trophiesStr);
                } catch (...) {
                    server.onlinePlayers[clientSocket].trophies = 0;
                }
            }

            std::cout << "[Login] User: " << playerId

                      << " (Trophies: " << server.onlinePlayers[clientSocket].trophies << ")" << std::endl;

            Server::sendPacket(clientSocket, PACKET_LOGIN, "Login Success");
        }

        break;

        case PACKET_UPLOAD_MAP: {
            std::lock_guard<std::mutex> lock(server.dataMutex);
            std::string uid = server.onlinePlayers[clientSocket].playerId;

            if (!uid.empty())

            {
                server.savedMaps[uid] = msgData;

                server.onlinePlayers[clientSocket].mapData = msgData;

                std::cout << "[Map] Saved for: " << uid << " (Size: " << msgData.size() << ")" << std::endl;
            }
        }

        break;

        case PACKET_QUERY_MAP: {
            std::lock_guard<std::mutex> lock(server.dataMutex);
            std::string targetId = msgData;

            if (server.savedMaps.find(targetId) != server.savedMaps.end())

            {
                Server::sendPacket(clientSocket, PACKET_QUERY_MAP, server.savedMaps[targetId]);

                std::cout << "[Query] Sent map of " << targetId << " to client." << std::endl;
            }

            else

            {
                Server::sendPacket(clientSocket, PACKET_QUERY_MAP, "");
            }
        }

        break;

        case PACKET_ATTACK_DATA:

            std::cout << "[Attack] Received replay data." << std::endl;

            break;

            // ========== 🆕 用户列表 ==========
        case REQ_USER_LIST: {
            std::lock_guard<std::mutex> lock(server.dataMutex);
            if (server.onlinePlayers.find(clientSocket) == server.onlinePlayers.end())
            {
                std::cout << "[UserList] Error: clientSocket not found" << std::endl;
                break;
            }

            std::string requesterId = server.onlinePlayers[clientSocket].playerId;

            if (requesterId.empty())
            {
                std::cout << "[UserList] Error: Player not logged in" << std::endl;
                Server::sendPacket(clientSocket, RESP_USER_LIST, "");
                break;
            }

            std::cout << "[UserList] Request from: " << requesterId << std::endl;
            std::string userList = server.getUserListJson(requesterId);
            Server::sendPacket(clientSocket, RESP_USER_LIST, userList);
            std::cout << "[UserList] Sent list to: " << requesterId << " (Size: " << userList.size() << ")"
                      << std::endl;
        }
        break;

            // ========== 玩家匹配对战 ==========

        case PACKET_FIND_MATCH:

            server.addToMatchQueue(clientSocket);

            break;

        case PACKET_MATCH_CANCEL:

            server.removeFromMatchQueue(clientSocket);

            std::cout << "[Match] Player cancelled matchmaking." << std::endl;

            break;

        case PACKET_ATTACK_START: {
            std::lock_guard<std::mutex> lock(server.dataMutex);
            // 请求攻击目标的地图

            std::string targetId = msgData;

            if (server.savedMaps.find(targetId) != server.savedMaps.end())

            {
                Server::sendPacket(clientSocket, PACKET_ATTACK_START, server.savedMaps[targetId]);

                std::cout << "[Battle] " << server.onlinePlayers[clientSocket].playerId << " attacking " << targetId

                          << std::endl;
            }
        }

        break;

        case PACKET_BATTLE_STATUS_LIST: {
            std::string statusJson = server.getBattleStatusListJson();
            Server::sendPacket(clientSocket, PACKET_BATTLE_STATUS_LIST, statusJson);
            break;
        }

        case PACKET_ATTACK_RESULT: {
            std::lock_guard<std::mutex> lock(server.dataMutex);
            try {
                AttackResult result = server.deserializeAttackResult(msgData);

                // 更新攻击者资源

                server.onlinePlayers[clientSocket].gold += result.goldLooted;

                server.onlinePlayers[clientSocket].elixir += result.elixirLooted;

                server.onlinePlayers[clientSocket].trophies += result.trophyChange;

                // 更新被攻击者资源（如果在线）

                SOCKET defenderSocket = server.findSocketByPlayerId(result.defenderId);

                if (defenderSocket != INVALID_SOCKET)

                {
                    server.onlinePlayers[defenderSocket].gold -= result.goldLooted;

                    server.onlinePlayers[defenderSocket].elixir -= result.elixirLooted;

                    server.onlinePlayers[defenderSocket].trophies -= result.trophyChange;

                    // 通知被攻击者

                    Server::sendPacket(defenderSocket, PACKET_ATTACK_RESULT, msgData);
                }

                std::cout << "[Battle] Result - Stars: " << result.starsEarned << ", Gold: " << result.goldLooted

                          << ", Trophies: " << result.trophyChange << std::endl;
            } catch (const std::exception& e) {
                std::cout << "[Battle] Error deserializing result: " << e.what() << std::endl;
            }
        }

        break;

            // ========== 部落系统 ==========

        case PACKET_CREATE_CLAN: {
            std::lock_guard<std::mutex> lock(server.dataMutex);
            std::string playerId = server.onlinePlayers[clientSocket].playerId;

            if (server.createClan(playerId, msgData))

            {
                std::string clanId = server.onlinePlayers[clientSocket].clanId;

                Server::sendPacket(clientSocket, PACKET_CREATE_CLAN, "OK|" + clanId);
            }

            else

            {
                Server::sendPacket(clientSocket, PACKET_CREATE_CLAN, "FAIL");
            }
        }

        break;

        case PACKET_JOIN_CLAN: {
            std::lock_guard<std::mutex> lock(server.dataMutex);
            std::string playerId = server.onlinePlayers[clientSocket].playerId;

            if (server.joinClan(playerId, msgData))

            {
                Server::sendPacket(clientSocket, PACKET_JOIN_CLAN, "OK");
            }

            else

            {
                Server::sendPacket(clientSocket, PACKET_JOIN_CLAN, "FAIL");
            }
        }

        break;

        case PACKET_LEAVE_CLAN: {
            std::lock_guard<std::mutex> lock(server.dataMutex);
            std::string playerId = server.onlinePlayers[clientSocket].playerId;

            if (server.leaveClan(playerId))

            {
                Server::sendPacket(clientSocket, PACKET_LEAVE_CLAN, "OK");
            }

            else

            {
                Server::sendPacket(clientSocket, PACKET_LEAVE_CLAN, "FAIL");
            }
        }

        break;

        case PACKET_CLAN_LIST: {
            std::string clanList = server.getClanListJson();

            Server::sendPacket(clientSocket, PACKET_CLAN_LIST, clanList);
        }

        break;

        case PACKET_CLAN_MEMBERS: {
            std::string members = server.getClanMembersJson(msgData);

            Server::sendPacket(clientSocket, PACKET_CLAN_MEMBERS, members);
        }

        break;

            // ========== 部落战争 ==========

        case PACKET_CLAN_WAR_SEARCH: {
            std::lock_guard<std::mutex> lock(server.dataMutex);
            std::string clanId = server.onlinePlayers[clientSocket].clanId;

            if (!clanId.empty())

            {
                server.addToClanWarQueue(clanId);

                Server::sendPacket(clientSocket, PACKET_CLAN_WAR_SEARCH, "SEARCHING");
            }

            else

            {
                Server::sendPacket(clientSocket, PACKET_CLAN_WAR_SEARCH, "NO_CLAN");
            }
        }

        break;

        case PACKET_CLAN_WAR_ATTACK: {
            std::lock_guard<std::mutex> lock(server.dataMutex);
            // 格式: warId|targetMemberId

            std::istringstream iss(msgData);

            std::string warId, targetId;

            std::getline(iss, warId, '|');

            std::getline(iss, targetId, '|');

            if (server.savedMaps.find(targetId) != server.savedMaps.end())

            {
                Server::sendPacket(clientSocket, PACKET_CLAN_WAR_ATTACK, warId + "|" + server.savedMaps[targetId]);
            }
        }

        break;

        case PACKET_CLAN_WAR_RESULT: {
            // 格式: warId|attackResult

            size_t pos = msgData.find('|');

            if (pos != std::string::npos)

            {
                std::string warId = msgData.substr(0, pos);

                std::string resultData = msgData.substr(pos + 1);

                try {
                    AttackResult result = server.deserializeAttackResult(resultData);
                    server.processClanWarAttack(warId, result);
                } catch (const std::exception& e) {
                    std::cout << "[ClanWar] Error deserializing result: " << e.what() << std::endl;
                }
            }
        }

        break;

        // 🆕 PVP与观战处理
        case PACKET_PVP_REQUEST:
            server.handlePvpRequest(clientSocket, msgData);
            break;

        case PACKET_PVP_ACTION:
            server.handlePvpAction(clientSocket, msgData);
            break;

        case PACKET_PVP_END: {
            std::string playerIdToClean;
            {
                std::lock_guard<std::mutex> lock(server.dataMutex);
                auto it = server.onlinePlayers.find(clientSocket);
                if (it != server.onlinePlayers.end())
                {
                    playerIdToClean = it->second.playerId;
                }
            }
            if (!playerIdToClean.empty())
            {
                server.endPvpSession(playerIdToClean);
            }
            break;
        }
        
        case PACKET_SPECTATE_REQUEST:
            server.handleSpectateRequest(clientSocket, msgData);
            break;

        case PACKET_CLAN_WAR_MEMBER_LIST: {
            std::string requesterId;
            {
                std::lock_guard<std::mutex> lock(server.dataMutex);
                auto it = server.onlinePlayers.find(clientSocket);
                if (it != server.onlinePlayers.end())
                {
                    requesterId = it->second.playerId;
                }
            }
            if (!requesterId.empty())
            {
                std::string json = server.getClanWarMemberListJson(msgData, requesterId);
                Server::sendPacket(clientSocket, PACKET_CLAN_WAR_MEMBER_LIST, json);
            }
            break;
        }
        
        case PACKET_CLAN_WAR_ATTACK_START:
        {
            // data格式: "warId|targetId"
            size_t pos = msgData.find('|');
            if (pos != std::string::npos)
            {
                std::string warId = msgData.substr(0, pos);
                std::string targetId = msgData.substr(pos + 1);
                server.handleClanWarAttackStart(clientSocket, warId, targetId);
            }
            break;
        }
        
        case PACKET_CLAN_WAR_ATTACK_END:
        {
            // data格式: "warId|attackerId|attackerName|stars|destruction"
            try {
                AttackRecord record;
                std::istringstream iss(msgData);
                std::string warId;
                std::getline(iss, warId, '|');
                std::getline(iss, record.attackerId, '|');
                std::getline(iss, record.attackerName, '|');
                std::string starsStr, destructionStr;
                std::getline(iss, starsStr, '|');
                std::getline(iss, destructionStr, '|');
                
                if (!starsStr.empty()) {
                    record.starsEarned = std::stoi(starsStr);
                }
                if (!destructionStr.empty()) {
                    record.destructionRate = std::stof(destructionStr);
                }
                record.attackTime = std::chrono::steady_clock::now();
                
                server.handleClanWarAttackEnd(warId, record);
            } catch (const std::exception& e) {
                std::cout << "[ClanWar] Error parsing attack end: " << e.what() << std::endl;
            }
            break;
        }
        
        case PACKET_CLAN_WAR_SPECTATE:
        {
            // data格式: "warId|targetId"
            size_t pos = msgData.find('|');
            if (pos != std::string::npos)
            {
                std::string warId = msgData.substr(0, pos);
                std::string targetId = msgData.substr(pos + 1);
                server.handleClanWarSpectate(clientSocket, warId, targetId);
            }
            break;
        }

        default:

            break;
        }
    }

    // 玩家断开连接时的清理
    std::string playerIdToClean;
    {
        std::lock_guard<std::mutex> lock(server.dataMutex);
        auto it = server.onlinePlayers.find(clientSocket);
        if (it != server.onlinePlayers.end())
        {
            playerIdToClean = it->second.playerId;
        }
    }

    server.removeFromMatchQueue(clientSocket);
    
    // 🆕 清理PVP会话
    if (!playerIdToClean.empty())
    {
        server.endPvpSession(playerIdToClean);
    }

    server.closeClientSocket(clientSocket);
}

// ==================== 服务器生命周期 ====================

Server::Server() : port(8888)

{
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)

    {
        exit(EXIT_FAILURE);
    }
}

Server::~Server()

{
    closesocket(serverSocket);

    WSACleanup();
}

void Server::run()

{
    createAndBindSocket();

    handleConnections();
}

void Server::createAndBindSocket()

{
    serverSocket = socket(AF_INET, SOCK_STREAM, 0);

    if (serverSocket == INVALID_SOCKET)

    {
        exit(EXIT_FAILURE);
    }

    serverAddr.sin_family = AF_INET;

    serverAddr.sin_addr.s_addr = INADDR_ANY;

    serverAddr.sin_port = htons(port);

    if (bind(serverSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR)

    {
        closesocket(serverSocket);

        exit(EXIT_FAILURE);
    }
}

void Server::handleConnections()

{
    listen(serverSocket, SOMAXCONN);

    std::cout << "=== Clash of Clans Server ===" << std::endl;

    std::cout << "Server started on port " << port << std::endl;

    std::cout << "Waiting for players..." << std::endl;

    while (true)

    {
        sockaddr_in clientAddr;

        int clientAddrLen = sizeof(clientAddr);

        SOCKET clientSocket = accept(serverSocket, (struct sockaddr*)&clientAddr, &clientAddrLen);

        if (clientSocket != INVALID_SOCKET)

        {
            std::cout << "[Connect] New client: " << clientSocket << std::endl;

            {
                std::lock_guard<std::mutex> lock(dataMutex);

                PlayerContext ctx;

                ctx.socket = clientSocket;

                onlinePlayers[clientSocket] = ctx;
            }

            std::thread clientThread(clientHandler, clientSocket, std::ref(*this));

            clientThread.detach();
        }
    }
}

void Server::closeClientSocket(SOCKET clientSocket)
{
    std::lock_guard<std::mutex> lock(dataMutex);
    std::string playerId = onlinePlayers[clientSocket].playerId;
    closesocket(clientSocket);
    onlinePlayers.erase(clientSocket);
    std::cout << "[Disconnect] Client: " << clientSocket;
    if (!playerId.empty())
    {
        std::cout << " (Player: " << playerId << ")";
    }
    std::cout << std::endl;
}

// ==================== 部落系统实现 ====================

std::string Server::getUserListJson(const std::string& requesterId)
{
    // ⚠️ 调用此函数前应该已经持有 dataMutex
    // 但为了安全，我们检查是否在持锁状态调用
    
    std::ostringstream oss;
    bool first = true;
    
    for (const auto& pair : onlinePlayers)
    {
        const auto& player = pair.second;
        if (player.playerId.empty() || player.playerId == requesterId)
            continue;
        
        if (!first) oss << "|";
        first = false;
        
        oss << player.playerId << ","
            << player.playerName << ","
            << player.trophies << ","
            << player.gold << ","
            << player.elixir;
    }
    
    return oss.str();
}

// ==================== 🆕 战斗状态广播 ====================

std::string Server::getBattleStatusListJson()
{
    std::lock_guard<std::mutex> pvpLock(pvpMutex);

    std::ostringstream oss;
    oss << "{\"statuses\":[";

    bool first = true;
    for (const auto& pair : pvpSessions)
    {
        const PvpSession& session = pair.second;
        if (!session.isActive)
            continue;

        // 攻击方状态
        if (!first)
            oss << ",";
        first = false;
        oss << "{\"userId\":\"" << session.attackerId << "\","
            << "\"inBattle\":true,"
            << "\"opponentId\":\"" << session.defenderId << "\","
            << "\"isAttacker\":true}";

        // 防守方状态
        oss << ",{\"userId\":\"" << session.defenderId << "\","
            << "\"inBattle\":true,"
            << "\"opponentId\":\"" << session.attackerId << "\","
            << "\"isAttacker\":false}";
    }

    oss << "]}";
    return oss.str();
}

void Server::broadcastBattleStatusToAll()
{
    std::string statusJson = getBattleStatusListJson();

    std::lock_guard<std::mutex> lock(dataMutex);
    for (const auto& pair : onlinePlayers)
    {
        if (!pair.second.playerId.empty())
        {
            sendPacket(pair.first, PACKET_BATTLE_STATUS_LIST, statusJson);
        }
    }
}

std::string Server::generateClanId()
{
    static int counter = 0;
    return "CLAN_" + std::to_string(++counter);
}

bool Server::createClan(const std::string& playerId, const std::string& clanName)
{
    // ⚠️ 调用者已持有 dataMutex

    SOCKET socket = findSocketByPlayerId(playerId);
    if (socket == INVALID_SOCKET)
    {
        std::cout << "[Clan] createClan failed: player " << playerId << " not found" << std::endl;
        return false;
    }

    if (!onlinePlayers[socket].clanId.empty())
    {
        std::cout << "[Clan] createClan failed: " << playerId << " already in a clan" << std::endl;
        return false;
    }

    // ✅ 锁 clanMutex 来修改 clans
    std::string clanId = generateClanId();

    {
        std::lock_guard<std::mutex> clanLock(clanMutex);

        ClanInfo clan;
        clan.clanId   = clanId;
        clan.clanName = clanName;
        clan.leaderId = playerId;
        clan.memberIds.push_back(playerId);
        clan.clanTrophies     = onlinePlayers[socket].trophies;
        clan.requiredTrophies = 0;
        clan.isOpen           = true;

        clans[clanId] = clan;
    }

    // 在 dataMutex 保护下修改 onlinePlayers
    onlinePlayers[socket].clanId = clanId;

    std::cout << "[Clan] ✅ Created: " << clanName << " (ID: " << clanId << ") by " << playerId << std::endl;
    return true;
}

bool Server::joinClan(const std::string& playerId, const std::string& clanId)
{
    // ⚠️ 重要：此函数被调用时，调用者已经持有 dataMutex
    // 我们这里只锁 clanMutex

    SOCKET socket = findSocketByPlayerId(playerId);
    if (socket == INVALID_SOCKET)
    {
        std::cout << "[Clan] joinClan failed: player " << playerId << " not found" << std::endl;
        return false;
    }

    // 检查玩家是否已在部落中
    if (!onlinePlayers[socket].clanId.empty())
    {
        std::cout << "[Clan] joinClan failed: " << playerId << " already in clan " << onlinePlayers[socket].clanId
                  << std::endl;
        return false;
    }

    // ✅ 现在锁 clanMutex 来访问 clans
    std::lock_guard<std::mutex> clanLock(clanMutex);

    auto it = clans.find(clanId);
    if (it == clans.end())
    {
        std::cout << "[Clan] joinClan failed: clan " << clanId << " not found" << std::endl;
        return false;
    }

    if (!it->second.isOpen)
    {
        std::cout << "[Clan] joinClan failed: clan " << clanId << " is closed" << std::endl;
        return false;
    }

    if (onlinePlayers[socket].trophies < it->second.requiredTrophies)
    {
        std::cout << "[Clan] joinClan failed: " << playerId << " has " << onlinePlayers[socket].trophies
                  << " trophies, required " << it->second.requiredTrophies << std::endl;
        return false;
    }

    // 加入部落
    it->second.memberIds.push_back(playerId);
    it->second.clanTrophies += onlinePlayers[socket].trophies;
    onlinePlayers[socket].clanId = clanId;

    std::cout << "[Clan] ✅ " << playerId << " successfully joined " << it->second.clanName << " (ID: " << clanId << ")"
              << std::endl;
    return true;
}

bool Server::leaveClan(const std::string& playerId)
{
    std::lock_guard<std::mutex> lock(clanMutex);
    
    SOCKET socket = findSocketByPlayerId(playerId);
    if (socket == INVALID_SOCKET) return false;
    
    std::string clanId = onlinePlayers[socket].clanId;
    if (clanId.empty()) return false;
    
    auto it = clans.find(clanId);
    if (it == clans.end()) return false;
    
    auto& members = it->second.memberIds;
    members.erase(std::remove(members.begin(), members.end(), playerId), members.end());
    it->second.clanTrophies -= onlinePlayers[socket].trophies;
    onlinePlayers[socket].clanId = "";
    
    if (members.empty())
    {
        clans.erase(it);
        std::cout << "[Clan] Deleted empty clan: " << clanId << std::endl;
    }
    
    std::cout << "[Clan] " << playerId << " left clan " << clanId << std::endl;
    return true;
}

std::string Server::getClanListJson()
{
    std::lock_guard<std::mutex> lock(clanMutex);
    
    std::ostringstream oss;
    oss << "[";
    bool first = true;
    
    for (const auto& pair : clans)
    {
        const auto& clan = pair.second;
        if (!first) oss << ",";
        first = false;
        
        oss << "{"
            << "\"id\":\"" << clan.clanId << "\","
            << "\"name\":\"" << clan.clanName << "\","
            << "\"members\":" << clan.memberIds.size() << ","
            << "\"trophies\":" << clan.clanTrophies << ","
            << "\"required\":" << clan.requiredTrophies << ","
            << "\"open\":" << (clan.isOpen ? "true" : "false")
            << "}";
    }
    
    oss << "]";
    return oss.str();
}

std::string Server::getClanMembersJson(const std::string& clanId)
{
    std::lock_guard<std::mutex> lock(clanMutex);
    
    auto it = clans.find(clanId);
    if (it == clans.end())
        return "{\"error\":\"CLAN_NOT_FOUND\"}";
    
    std::ostringstream oss;
    oss << "{\"members\":[";
    
    bool first = true;
    for (const auto& memberId : it->second.memberIds)
    {
        if (!first) oss << ",";
        first = false;
        
        SOCKET socket = findSocketByPlayerId(memberId);
        bool online = (socket != INVALID_SOCKET);
        int trophies = 0;
        std::string name = memberId;
        
        if (online)
        {
            trophies = onlinePlayers[socket].trophies;
            name = onlinePlayers[socket].playerName;
        }
        
        oss << "{"
            << "\"id\":\"" << memberId << "\","
            << "\"name\":\"" << name << "\","
            << "\"trophies\":" << trophies << ","
            << "\"online\":" << (online ? "true" : "false")
            << "}";
    }
    
    oss << "]}";
    return oss.str();
}

// ==================== 部落战争实现 ====================

std::string Server::generateWarId()
{
    static int counter = 0;
    return "WAR_" + std::to_string(++counter);
}

void Server::addToClanWarQueue(const std::string& clanId)
{
    std::lock_guard<std::mutex> lock(warMutex);
    
    if (std::find(clanWarQueue.begin(), clanWarQueue.end(), clanId) != clanWarQueue.end())
        return;
    
    clanWarQueue.push_back(clanId);
    std::cout << "[ClanWar] Clan " << clanId << " added to queue" << std::endl;
    
    processClanWarQueue();
}

void Server::processClanWarQueue()
{
    if (clanWarQueue.size() < 2)
        return;
    
    std::string clan1Id = clanWarQueue[0];
    std::string clan2Id = clanWarQueue[1];
    
    clanWarQueue.erase(clanWarQueue.begin());
    clanWarQueue.erase(clanWarQueue.begin());
    
    startClanWar(clan1Id, clan2Id);
}

void Server::startClanWar(const std::string& clan1Id, const std::string& clan2Id)
{
    std::string warId;
    std::vector<std::string> clan1MemberIds;
    std::vector<std::string> clan2MemberIds;
    
    // 第一步：生成warId并创建会话
    {
        std::lock_guard<std::mutex> warLock(warMutex);
        warId = generateWarId();
    }
    
    // 第二步：创建会话并初始化成员
    {
        std::lock_guard<std::mutex> sessionLock(clanWarSessionMutex);
        
        ClanWarSession session;
        session.warId = warId;
        session.clan1Id = clan1Id;
        session.clan2Id = clan2Id;
        session.startTime = std::chrono::steady_clock::now();
        session.isActive = true;
        
        initClanWarMembers(session);
        
        clanWarSessions[warId] = session;
        
        std::cout << "[ClanWar] Started: " << warId << " between " << clan1Id << " and " << clan2Id << std::endl;
    }
    
    // 第三步：获取两个部落的成员列表
    {
        std::lock_guard<std::mutex> clanLock(clanMutex);
        
        auto clan1It = clans.find(clan1Id);
        if (clan1It != clans.end())
        {
            clan1MemberIds = clan1It->second.memberIds;
        }
        
        auto clan2It = clans.find(clan2Id);
        if (clan2It != clans.end())
        {
            clan2MemberIds = clan2It->second.memberIds;
        }
    }
    
    // 第四步：通知所有成员
    {
        std::lock_guard<std::mutex> dataLock(dataMutex);
        
        std::string msg = warId + "|" + clan1Id + "|" + clan2Id;
        
        auto notifyMembers = [&](const std::vector<std::string>& memberIds) {
            for (const auto& memberId : memberIds)
            {
                SOCKET memberSocket = findSocketByPlayerId(memberId);
                if (memberSocket != INVALID_SOCKET)
                {
                    sendPacket(memberSocket, PACKET_CLAN_WAR_MATCH, msg);
                }
            }
        };
        
        notifyMembers(clan1MemberIds);
        notifyMembers(clan2MemberIds);
    }
}

void Server::processClanWarAttack(const std::string& warId, const AttackResult& result)
{
    std::lock_guard<std::mutex> lock(warMutex);
    
    auto it = activeWars.find(warId);
    if (it == activeWars.end()) return;
    
    it->second.attacks.push_back(result);
    
    std::cout << "[ClanWar] Attack in " << warId << ": "
              << result.attackerId << " earned " << result.starsEarned << " stars" << std::endl;
}

// ==================== 匹配系统实现 ====================

void Server::addToMatchQueue(SOCKET socket)
{
    std::lock_guard<std::mutex> lock(matchQueueMutex);
    
    std::string playerId;
    int trophies = 0;
    
    {
        std::lock_guard<std::mutex> dataLock(dataMutex);
        auto it = onlinePlayers.find(socket);
        if (it == onlinePlayers.end()) return;
        
        // 检查是否已经在队列中
        for (const auto& entry : matchQueue)
        {
            if (entry.socket == socket) return;
        }
        
        playerId = it->second.playerId;
        trophies = it->second.trophies;
        it->second.isSearchingMatch = true;
    }
    
    MatchQueueEntry entry;
    entry.socket = socket;
    entry.playerId = playerId;
    entry.trophies = trophies;
    entry.queueTime = std::chrono::steady_clock::now();
    
    matchQueue.push_back(entry);
    
    std::cout << "[Match] Player " << entry.playerId << " joined queue (Trophies: " << entry.trophies << ")" << std::endl;
    
    processMatchQueue();
}

void Server::removeFromMatchQueue(SOCKET socket)
{
    std::lock_guard<std::mutex> lock(matchQueueMutex);
    
    auto it = std::find_if(matchQueue.begin(), matchQueue.end(), 
        [socket](const MatchQueueEntry& e) { return e.socket == socket; });
    
    if (it != matchQueue.end())
    {
        std::cout << "[Match] Player " << it->playerId << " left queue" << std::endl;
        matchQueue.erase(it);
    }
    
    {
        std::lock_guard<std::mutex> dataLock(dataMutex);
        auto playerIt = onlinePlayers.find(socket);
        if (playerIt != onlinePlayers.end())
        {
            playerIt->second.isSearchingMatch = false;
        }
    }
}

void Server::processMatchQueue()
{
    // 已经在持有 matchQueueMutex 的情况下调用
    if (matchQueue.size() < 2) return;
    
    auto now = std::chrono::steady_clock::now();
    
    for (size_t i = 0; i < matchQueue.size(); i++)
    {
        auto& entry1 = matchQueue[i];
        auto waitTime = std::chrono::duration_cast<std::chrono::seconds>(now - entry1.queueTime).count();
        
        // 根据等待时间扩大匹配范围
        int maxDiff = 200 + static_cast<int>(waitTime * 10);
        
        for (size_t j = i + 1; j < matchQueue.size(); j++)
        {
            auto& entry2 = matchQueue[j];
            int trophyDiff = std::abs(entry1.trophies - entry2.trophies);
            
            if (trophyDiff <= maxDiff)
            {
                // 匹配成功
                std::cout << "[Match] Matched: " << entry1.playerId << " vs " << entry2.playerId << std::endl;
                
                // 通知双方
                std::string msg1 = entry2.playerId + "|" + std::to_string(entry2.trophies);
                std::string msg2 = entry1.playerId + "|" + std::to_string(entry1.trophies);
                
                sendPacket(entry1.socket, PACKET_MATCH_FOUND, msg1);
                sendPacket(entry2.socket, PACKET_MATCH_FOUND, msg2);
                
                // 更新状态
                {
                    std::lock_guard<std::mutex> dataLock(dataMutex);
                    auto player1It = onlinePlayers.find(entry1.socket);
                    auto player2It = onlinePlayers.find(entry2.socket);
                    if (player1It != onlinePlayers.end()) player1It->second.isSearchingMatch = false;
                    if (player2It != onlinePlayers.end()) player2It->second.isSearchingMatch = false;
                }
                
                // 从队列中移除
                matchQueue.erase(matchQueue.begin() + j);
                matchQueue.erase(matchQueue.begin() + i);
                
                return;
            }
        }
    }
}

PlayerContext* Server::findMatchForPlayer(const PlayerContext& player)
{
    // 这个函数目前未使用，保留作为备用
    return nullptr;
}

