/****************************************************************
 * Project Name:  Clash_of_Clans
 * File Name:     ClanWarRoom.cpp
 * File Function: 部落战争系统实现
 * Author:        赵崇治
 * Update Date:   2025/12/19
 * License:       MIT License
 ****************************************************************/
#include "ClanWarRoom.h"
#include "Protocol.h"
#include <algorithm>
#include <iostream>
#include <sstream>

 // 前向声明（需要在头文件或此处声明 sendPacket）
extern bool sendPacket(SOCKET socket, uint32_t type, const std::string& data);

ClanWarRoom::ClanWarRoom(PlayerRegistry* registry, ClanHall* hall)
    : playerRegistry(registry), clanHall(hall)
{
}

std::string ClanWarRoom::GenerateWarId()
{
    static int counter = 0;
    return "WAR_" + std::to_string(++counter);
}

void ClanWarRoom::AddToQueue(const std::string& clanId)
{
    std::lock_guard<std::mutex> lock(warMutex);

    if (std::find(warQueue.begin(), warQueue.end(), clanId) != warQueue.end())
        return;

    warQueue.push_back(clanId);
    std::cout << "[ClanWar] Clan " << clanId << " added to queue" << std::endl;

    ProcessQueue();
}

void ClanWarRoom::ProcessQueue()
{
    // 假设已持有 warMutex
    if (warQueue.size() < 2)
        return;

    std::string clan1Id = warQueue[0];
    std::string clan2Id = warQueue[1];

    warQueue.erase(warQueue.begin());
    warQueue.erase(warQueue.begin());

    StartWar(clan1Id, clan2Id);
}

void ClanWarRoom::StartWar(const std::string& clan1Id, const std::string& clan2Id)
{
    std::string warId = GenerateWarId();

    {
        std::lock_guard<std::mutex> lock(sessionMutex);

        ClanWarSession session;
        session.warId = warId;
        session.clan1Id = clan1Id;
        session.clan2Id = clan2Id;
        session.startTime = std::chrono::steady_clock::now();
        session.isActive = true;

        // 初始化双方成员
        auto clan1Members = clanHall->GetClanMemberIds(clan1Id);
        for (const auto& memberId : clan1Members)
        {
            ClanWarMember member;
            member.memberId = memberId;

            PlayerContext* player = playerRegistry->GetById(memberId);
            if (player)
            {
                member.memberName = player->playerName;
                member.mapData = player->mapData;
            }

            session.clan1Members.push_back(member);
        }

        auto clan2Members = clanHall->GetClanMemberIds(clan2Id);
        for (const auto& memberId : clan2Members)
        {
            ClanWarMember member;
            member.memberId = memberId;

            PlayerContext* player = playerRegistry->GetById(memberId);
            if (player)
            {
                member.memberName = player->playerName;
                member.mapData = player->mapData;
            }

            session.clan2Members.push_back(member);
        }

        activeWars[warId] = session;

        std::cout << "[ClanWar] Started: " << warId << " between " << clan1Id << " and " << clan2Id << std::endl;
    }

    // 通知所有成员
    std::string msg = warId + "|" + clan1Id + "|" + clan2Id;

    auto notifyMembers = [&](const std::vector<std::string>& memberIds) {
        for (const auto& memberId : memberIds)
        {
            PlayerContext* player = playerRegistry->GetById(memberId);
            if (player && player->socket != INVALID_SOCKET)
            {
                sendPacket(player->socket, PACKET_CLAN_WAR_MATCH, msg);
            }
        }
        };

    notifyMembers(clanHall->GetClanMemberIds(clan1Id));
    notifyMembers(clanHall->GetClanMemberIds(clan2Id));
}

void ClanWarRoom::HandleAttackStart(SOCKET clientSocket, const std::string& warId, const std::string& targetId)
{
    PlayerContext* attacker = playerRegistry->GetBySocket(clientSocket);
    if (!attacker)
    {
        sendPacket(clientSocket, PACKET_CLAN_WAR_ATTACK_START, "FAIL|NOT_LOGGED_IN|");
        return;
    }

    std::string attackerId = attacker->playerId;
    std::string targetMapData;

    {
        std::lock_guard<std::mutex> lock(sessionMutex);

        auto it = activeWars.find(warId);
        if (it == activeWars.end())
        {
            sendPacket(clientSocket, PACKET_CLAN_WAR_ATTACK_START, "FAIL|WAR_NOT_FOUND|");
            return;
        }

        ClanWarSession& session = it->second;

        // 查找目标成员
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

        // 创建战斗会话
        PvpSession pvpSession;
        pvpSession.attackerId = attackerId;
        pvpSession.defenderId = targetId;
        pvpSession.mapData = targetMapData;
        pvpSession.isActive = true;

        session.activeBattles[attackerId] = pvpSession;
    }

    std::cout << "[ClanWar] Attack started: " << attackerId << " -> " << targetId << " in " << warId << std::endl;

    std::string response = "ATTACK|" + targetId + "|" + targetMapData;
    sendPacket(clientSocket, PACKET_CLAN_WAR_ATTACK_START, response);
}

void ClanWarRoom::HandleAttackEnd(const std::string& warId, const AttackRecord& record)
{
    bool needBroadcast = false;

    {
        std::lock_guard<std::mutex> lock(sessionMutex);

        auto it = activeWars.find(warId);
        if (it == activeWars.end())
            return;

        ClanWarSession& session = it->second;

        auto battleIt = session.activeBattles.find(record.attackerId);
        if (battleIt == session.activeBattles.end())
        {
            std::cout << "[ClanWar] Error: No active battle for " << record.attackerId << std::endl;
            return;
        }

        std::string defenderId = battleIt->second.defenderId;

        // 判断攻击者所属部落
        bool isAttackerInClan1 = clanHall->IsPlayerInClan(record.attackerId, session.clan1Id);

        // 目标在敌方部落
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

        if (targetMember)
        {
            targetMember->attacksReceived.push_back(record);

            if (record.starsEarned > targetMember->bestStars ||
                (record.starsEarned == targetMember->bestStars && record.destructionRate > targetMember->bestDestructionRate))
            {
                targetMember->bestStars = record.starsEarned;
                targetMember->bestDestructionRate = record.destructionRate;
            }

            if (isAttackerInClan1)
            {
                session.clan1TotalStars += record.starsEarned;
            }
            else
            {
                session.clan2TotalStars += record.starsEarned;
            }

            std::cout << "[ClanWar] Attack ended: " << record.attackerId << " -> " << defenderId
                << " earned " << record.starsEarned << " stars" << std::endl;
        }

        session.activeBattles.erase(record.attackerId);
        needBroadcast = true;
    }

    if (needBroadcast)
    {
        BroadcastWarUpdate(warId);
    }
}

void ClanWarRoom::HandleSpectate(SOCKET clientSocket, const std::string& warId, const std::string& targetId)
{
    PlayerContext* spectator = playerRegistry->GetBySocket(clientSocket);
    if (!spectator)
    {
        sendPacket(clientSocket, PACKET_CLAN_WAR_SPECTATE, "0|||");
        return;
    }

    std::string spectatorId = spectator->playerId;
    std::string attackerId, defenderId, mapData;
    bool found = false;

    {
        std::lock_guard<std::mutex> lock(sessionMutex);

        auto it = activeWars.find(warId);
        if (it == activeWars.end())
        {
            sendPacket(clientSocket, PACKET_CLAN_WAR_SPECTATE, "0|||");
            return;
        }

        ClanWarSession& session = it->second;

        for (auto& pair : session.activeBattles)
        {
            if (pair.second.attackerId == targetId || pair.second.defenderId == targetId)
            {
                attackerId = pair.second.attackerId;
                defenderId = pair.second.defenderId;
                mapData = pair.second.mapData;
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

    std::cout << "[ClanWar] Spectator " << spectatorId << " watching " << attackerId << " vs " << defenderId << std::endl;

    std::string response = "1|" + attackerId + "|" + defenderId + "|" + mapData;
    sendPacket(clientSocket, PACKET_CLAN_WAR_SPECTATE, response);
}

std::string ClanWarRoom::GetMemberListJson(const std::string& warId, const std::string& requesterId)
{
    std::lock_guard<std::mutex> lock(sessionMutex);

    auto it = activeWars.find(warId);
    if (it == activeWars.end())
    {
        return "{\"error\":\"War not found\"}";
    }

    const ClanWarSession& session = it->second;

    bool isInClan1 = clanHall->IsPlayerInClan(requesterId, session.clan1Id);

    std::ostringstream oss;
    oss << "{";
    oss << "\"warId\":\"" << warId << "\",";
    oss << "\"clan1TotalStars\":" << session.clan1TotalStars << ",";
    oss << "\"clan2TotalStars\":" << session.clan2TotalStars << ",";
    oss << "\"enemyMembers\":[";

    const auto& enemyMembers = isInClan1 ? session.clan2Members : session.clan1Members;
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

void ClanWarRoom::BroadcastWarUpdate(const std::string& warId)
{
    std::string stateJson;
    std::string clan1Id, clan2Id;

    {
        std::lock_guard<std::mutex> lock(sessionMutex);

        auto it = activeWars.find(warId);
        if (it == activeWars.end())
            return;

        const ClanWarSession& session = it->second;
        clan1Id = session.clan1Id;
        clan2Id = session.clan2Id;

        std::ostringstream oss;
        oss << "{";
        oss << "\"warId\":\"" << warId << "\",";
        oss << "\"clan1Stars\":" << session.clan1TotalStars << ",";
        oss << "\"clan2Stars\":" << session.clan2TotalStars;
        oss << "}";

        stateJson = oss.str();
    }

    auto sendToMembers = [&](const std::vector<std::string>& memberIds) {
        for (const auto& memberId : memberIds)
        {
            PlayerContext* player = playerRegistry->GetById(memberId);
            if (player && player->socket != INVALID_SOCKET)
            {
                sendPacket(player->socket, PACKET_CLAN_WAR_STATE_UPDATE, stateJson);
            }
        }
        };

    sendToMembers(clanHall->GetClanMemberIds(clan1Id));
    sendToMembers(clanHall->GetClanMemberIds(clan2Id));
}