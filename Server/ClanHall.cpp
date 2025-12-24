/****************************************************************
 * Project Name:  Clash_of_Clans
 * File Name:     ClanHall.cpp
 * File Function: 部落系统实现
 * Author:        赵崇治
 * Update Date:   2025/12/19
 * License:       MIT License
 ****************************************************************/
#include "ClanHall.h"
#include <algorithm>
#include <iostream>
#include <sstream>

ClanHall::ClanHall(PlayerRegistry* registry)
    : playerRegistry(registry)
{
}

std::string ClanHall::GenerateClanId()
{
    static int counter = 0;
    return "CLAN_" + std::to_string(++counter);
}

bool ClanHall::CreateClan(const std::string& playerId, const std::string& clanName)
{
    PlayerContext* player = playerRegistry->GetById(playerId);
    if (!player)
    {
        std::cout << "[Clan] createClan failed: player " << playerId << " not found" << std::endl;
        return false;
    }

    if (!player->clanId.empty())
    {
        std::cout << "[Clan] createClan failed: " << playerId << " already in a clan" << std::endl;
        return false;
    }

    std::string clanId = GenerateClanId();

    {
        std::lock_guard<std::mutex> lock(clanMutex);

        ClanInfo clan;
        clan.clanId = clanId;
        clan.clanName = clanName;
        clan.leaderId = playerId;
        clan.memberIds.push_back(playerId);
        clan.clanTrophies = player->trophies;
        clan.requiredTrophies = 0;
        clan.isOpen = true;

        clans[clanId] = clan;
    }

    player->clanId = clanId;

    std::cout << "[Clan] Created: " << clanName << " (ID: " << clanId << ") by " << playerId << std::endl;
    return true;
}

bool ClanHall::JoinClan(const std::string& playerId, const std::string& clanId)
{
    PlayerContext* player = playerRegistry->GetById(playerId);
    if (!player)
    {
        std::cout << "[Clan] joinClan failed: player " << playerId << " not found" << std::endl;
        return false;
    }

    if (!player->clanId.empty())
    {
        std::cout << "[Clan] joinClan failed: " << playerId << " already in clan " << player->clanId << std::endl;
        return false;
    }

    std::lock_guard<std::mutex> lock(clanMutex);

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

    if (player->trophies < it->second.requiredTrophies)
    {
        std::cout << "[Clan] joinClan failed: " << playerId << " has " << player->trophies
            << " trophies, required " << it->second.requiredTrophies << std::endl;
        return false;
    }

    it->second.memberIds.push_back(playerId);
    it->second.clanTrophies += player->trophies;
    player->clanId = clanId;

    std::cout << "[Clan] " << playerId << " joined " << it->second.clanName << " (ID: " << clanId << ")" << std::endl;
    return true;
}

bool ClanHall::LeaveClan(const std::string& playerId)
{
    PlayerContext* player = playerRegistry->GetById(playerId);
    if (!player)
        return false;

    std::string clanId = player->clanId;
    if (clanId.empty())
        return false;

    std::lock_guard<std::mutex> lock(clanMutex);

    auto it = clans.find(clanId);
    if (it == clans.end())
        return false;

    auto& members = it->second.memberIds;
    members.erase(std::remove(members.begin(), members.end(), playerId), members.end());
    it->second.clanTrophies -= player->trophies;
    player->clanId = "";

    if (members.empty())
    {
        clans.erase(it);
        std::cout << "[Clan] Deleted empty clan: " << clanId << std::endl;
    }

    std::cout << "[Clan] " << playerId << " left clan " << clanId << std::endl;
    return true;
}

std::string ClanHall::GetClanListJson()
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

std::string ClanHall::GetClanMembersJson(const std::string& clanId)
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

        PlayerContext* player = playerRegistry->GetById(memberId);
        bool online = (player != nullptr);
        int trophies = online ? player->trophies : 0;
        std::string name = online ? player->playerName : memberId;

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

bool ClanHall::IsPlayerInClan(const std::string& playerId, const std::string& clanId)
{
    std::lock_guard<std::mutex> lock(clanMutex);

    auto it = clans.find(clanId);
    if (it == clans.end())
        return false;

    auto& members = it->second.memberIds;
    return std::find(members.begin(), members.end(), playerId) != members.end();
}

std::vector<std::string> ClanHall::GetClanMemberIds(const std::string& clanId)
{
    std::lock_guard<std::mutex> lock(clanMutex);

    auto it = clans.find(clanId);
    if (it == clans.end())
        return {};

    return it->second.memberIds;
}