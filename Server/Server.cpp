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

std::string Server::generateClanId()

{
    static std::random_device rd;

    static std::mt19937 gen(rd());

    static std::uniform_int_distribution<> dis(100000, 999999);

    return "CLAN_" + std::to_string(dis(gen));
}

std::string Server::generateWarId()

{
    static std::random_device rd;

    static std::mt19937 gen(rd());

    static std::uniform_int_distribution<> dis(100000, 999999);

    return "WAR_" + std::to_string(dis(gen));
}

// ==================== 匹配系统 ====================

void Server::addToMatchQueue(SOCKET socket)

{
    std::lock_guard<std::mutex> lock(matchQueueMutex);

    auto& player = onlinePlayers[socket];

    player.isSearchingMatch = true;

    player.matchStartTime = std::chrono::steady_clock::now();

    MatchQueueEntry entry;

    entry.socket = socket;

    entry.playerId = player.playerId;

    entry.trophies = player.trophies;

    entry.queueTime = std::chrono::steady_clock::now();

    matchQueue.push_back(entry);

    std::cout << "[Match] Player " << player.playerId << " joined queue (Trophies: " << player.trophies << ")"

              << std::endl;

    // 尝试立即匹配

    processMatchQueue();
}

void Server::removeFromMatchQueue(SOCKET socket)

{
    std::lock_guard<std::mutex> lock(matchQueueMutex);

    matchQueue.erase(std::remove_if(matchQueue.begin(), matchQueue.end(),

                                    [socket](const MatchQueueEntry& e) { return e.socket == socket; }),

                     matchQueue.end());

    if (onlinePlayers.find(socket) != onlinePlayers.end())

    {
        onlinePlayers[socket].isSearchingMatch = false;
    }
}

void Server::processMatchQueue()

{
    if (matchQueue.size() < 2)

        return;

    // 按奖杯数排序

    std::sort(matchQueue.begin(), matchQueue.end(),

              [](const MatchQueueEntry& a, const MatchQueueEntry& b) { return a.trophies < b.trophies; });

    std::vector<std::pair<size_t, size_t>> matchedPairs;

    // 寻找匹配对（奖杯差距在200以内）

    for (size_t i = 0; i < matchQueue.size(); ++i)

    {
        for (size_t j = i + 1; j < matchQueue.size(); ++j)

        {
            int diff = std::abs(matchQueue[i].trophies - matchQueue[j].trophies);

            // 等待时间越长，匹配范围越宽

            auto now = std::chrono::steady_clock::now();

            auto waitTime = std::chrono::duration_cast<std::chrono::seconds>(now - matchQueue[i].queueTime).count();

            int maxDiff = 200 + static_cast<int>(waitTime * 10);  // 每秒增加10奖杯范围

            if (diff <= maxDiff)

            {
                matchedPairs.push_back({i, j});

                break;
            }
        }
    }

    // 处理匹配成功的玩家

    for (const auto& pair : matchedPairs)

    {
        auto& player1 = matchQueue[pair.first];

        auto& player2 = matchQueue[pair.second];

        // 发送匹配成功消息

        std::string matchData1 = player2.playerId + "|" + std::to_string(player2.trophies);

        std::string matchData2 = player1.playerId + "|" + std::to_string(player1.trophies);

        sendPacket(player1.socket, PACKET_MATCH_FOUND, matchData1);

        sendPacket(player2.socket, PACKET_MATCH_FOUND, matchData2);

        std::cout << "[Match] Matched: " << player1.playerId << " vs " << player2.playerId << std::endl;

        // 更新玩家状态

        if (onlinePlayers.find(player1.socket) != onlinePlayers.end())

        {
            onlinePlayers[player1.socket].isSearchingMatch = false;
        }

        if (onlinePlayers.find(player2.socket) != onlinePlayers.end())

        {
            onlinePlayers[player2.socket].isSearchingMatch = false;
        }
    }

    // 从队列中移除已匹配的玩家

    std::vector<MatchQueueEntry> newQueue;

    for (size_t i = 0; i < matchQueue.size(); ++i)

    {
        bool matched = false;

        for (const auto& pair : matchedPairs)

        {
            if (i == pair.first || i == pair.second)

            {
                matched = true;

                break;
            }
        }

        if (!matched)

        {
            newQueue.push_back(matchQueue[i]);
        }
    }

    matchQueue = std::move(newQueue);
}

// ==================== 部落系统 ====================

bool Server::createClan(const std::string& playerId, const std::string& clanName)

{
    std::lock_guard<std::mutex> lock(clanMutex);

    // 检查玩家是否已在部落中

    SOCKET socket = findSocketByPlayerId(playerId);

    if (socket != INVALID_SOCKET && !onlinePlayers[socket].clanId.empty())

    {
        return false;
    }

    ClanInfo clan;

    clan.clanId = generateClanId();

    clan.clanName = clanName;

    clan.leaderId = playerId;

    clan.memberIds.push_back(playerId);

    clan.clanTrophies = 0;

    clans[clan.clanId] = clan;

    // 更新玩家的部落ID

    if (socket != INVALID_SOCKET)

    {
        onlinePlayers[socket].clanId = clan.clanId;
    }

    std::cout << "[Clan] Created: " << clanName << " (ID: " << clan.clanId << ") by " << playerId << std::endl;

    return true;
}

bool Server::joinClan(const std::string& playerId, const std::string& clanId)

{
    std::lock_guard<std::mutex> lock(clanMutex);

    if (clans.find(clanId) == clans.end())

    {
        return false;
    }

    SOCKET socket = findSocketByPlayerId(playerId);

    if (socket == INVALID_SOCKET)

    {
        return false;
    }

    // 检查是否已在部落中

    if (!onlinePlayers[socket].clanId.empty())

    {
        return false;
    }

    // 检查奖杯要求

    auto& clan = clans[clanId];

    if (onlinePlayers[socket].trophies < clan.requiredTrophies)

    {
        return false;
    }

    clan.memberIds.push_back(playerId);

    onlinePlayers[socket].clanId = clanId;

    // 更新部落总奖杯

    clan.clanTrophies += onlinePlayers[socket].trophies;

    std::cout << "[Clan] " << playerId << " joined " << clan.clanName << std::endl;

    return true;
}

bool Server::leaveClan(const std::string& playerId)

{
    std::lock_guard<std::mutex> lock(clanMutex);

    SOCKET socket = findSocketByPlayerId(playerId);

    if (socket == INVALID_SOCKET)

    {
        return false;
    }

    std::string clanId = onlinePlayers[socket].clanId;

    if (clanId.empty())

    {
        return false;
    }

    auto& clan = clans[clanId];

    // 从成员列表中移除

    clan.memberIds.erase(std::remove(clan.memberIds.begin(), clan.memberIds.end(), playerId), clan.memberIds.end());

    // 更新部落总奖杯

    clan.clanTrophies -= onlinePlayers[socket].trophies;

    // 如果是首领且还有其他成员，转让首领

    if (clan.leaderId == playerId && !clan.memberIds.empty())

    {
        clan.leaderId = clan.memberIds[0];
    }

    // 如果部落没有成员了，删除部落

    if (clan.memberIds.empty())

    {
        clans.erase(clanId);

        std::cout << "[Clan] Dissolved: " << clanId << std::endl;
    }

    onlinePlayers[socket].clanId = "";

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
        if (!first)

            oss << ",";

        first = false;

        const auto& clan = pair.second;

        oss << "{\"id\":\"" << clan.clanId << "\""

            << ",\"name\":\"" << clan.clanName << "\""

            << ",\"members\":" << clan.memberIds.size() << ",\"trophies\":" << clan.clanTrophies

            << ",\"required\":" << clan.requiredTrophies << ",\"open\":" << (clan.isOpen ? "true" : "false") << "}";
    }

    oss << "]";

    return oss.str();
}

std::string Server::getClanMembersJson(const std::string& clanId)

{
    std::lock_guard<std::mutex> lock(clanMutex);

    if (clans.find(clanId) == clans.end())

    {
        return "[]";
    }

    const auto& clan = clans[clanId];

    std::ostringstream oss;

    oss << "{\"clanId\":\"" << clan.clanId << "\""

        << ",\"name\":\"" << clan.clanName << "\""

        << ",\"leader\":\"" << clan.leaderId << "\""

        << ",\"members\":[";

    bool first = true;

    for (const auto& memberId : clan.memberIds)

    {
        if (!first)

            oss << ",";

        first = false;

        SOCKET socket = findSocketByPlayerId(memberId);

        int trophies = 0;

        std::string name = memberId;

        if (socket != INVALID_SOCKET)

        {
            trophies = onlinePlayers[socket].trophies;

            name = onlinePlayers[socket].playerName;
        }

        oss << "{\"id\":\"" << memberId << "\""

            << ",\"name\":\"" << name << "\""

            << ",\"trophies\":" << trophies << ",\"online\":" << (socket != INVALID_SOCKET ? "true" : "false") << "}";
    }

    oss << "]}";

    return oss.str();
}

// ==================== 部落战争 ====================

void Server::addToClanWarQueue(const std::string& clanId)

{
    std::lock_guard<std::mutex> lock(warMutex);

    // 检查部落是否存在且未在战争中

    if (clans.find(clanId) == clans.end())

        return;

    for (const auto& war : activeWars)

    {
        if (war.second.clan1Id == clanId || war.second.clan2Id == clanId)

        {
            return;  // 已在战争中
        }
    }

    // 检查是否已在队列中

    if (std::find(clanWarQueue.begin(), clanWarQueue.end(), clanId) != clanWarQueue.end())

    {
        return;
    }

    clanWarQueue.push_back(clanId);

    std::cout << "[ClanWar] Clan " << clanId << " searching for war..." << std::endl;

    processClanWarQueue();
}

void Server::processClanWarQueue()

{
    if (clanWarQueue.size() < 2)

        return;

    // 按部落总奖杯排序

    std::sort(clanWarQueue.begin(), clanWarQueue.end(), [this](const std::string& a, const std::string& b) {
        return clans[a].clanTrophies < clans[b].clanTrophies;
    });

    // 匹配奖杯相近的部落

    for (size_t i = 0; i < clanWarQueue.size() - 1; ++i)

    {
        int diff = std::abs(clans[clanWarQueue[i]].clanTrophies - clans[clanWarQueue[i + 1]].clanTrophies);

        if (diff <= 500)

        {  // 部落战奖杯差距500以内

            startClanWar(clanWarQueue[i], clanWarQueue[i + 1]);

            // 从队列中移除

            clanWarQueue.erase(clanWarQueue.begin() + i + 1);

            clanWarQueue.erase(clanWarQueue.begin() + i);

            return;
        }
    }
}

void Server::startClanWar(const std::string& clan1Id, const std::string& clan2Id)

{
    ClanWarInfo war;

    war.warId = generateWarId();

    war.clan1Id = clan1Id;

    war.clan2Id = clan2Id;

    war.startTime = std::chrono::steady_clock::now();

    war.endTime = war.startTime + std::chrono::hours(24);  // 24小时战争

    war.isActive = true;

    activeWars[war.warId] = war;

    // 通知两个部落的所有在线成员

    std::string warData = war.warId + "|" + clan1Id + "|" + clan2Id;

    auto notifyClan = [this, &warData](const std::string& clanId) {
        if (clans.find(clanId) != clans.end())

        {
            for (const auto& memberId : clans[clanId].memberIds)

            {
                SOCKET socket = findSocketByPlayerId(memberId);

                if (socket != INVALID_SOCKET)

                {
                    sendPacket(socket, PACKET_CLAN_WAR_MATCH, warData);
                }
            }
        }
    };

    notifyClan(clan1Id);

    notifyClan(clan2Id);

    std::cout << "[ClanWar] Started: " << clans[clan1Id].clanName << " vs " << clans[clan2Id].clanName << std::endl;
}

void Server::processClanWarAttack(const std::string& warId, const AttackResult& result)

{
    std::lock_guard<std::mutex> lock(warMutex);

    if (activeWars.find(warId) == activeWars.end())

        return;

    auto& war = activeWars[warId];

    war.attacks.push_back(result);

    // 更新星星数

    SOCKET attackerSocket = findSocketByPlayerId(result.attackerId);

    if (attackerSocket != INVALID_SOCKET)

    {
        std::string attackerClanId = onlinePlayers[attackerSocket].clanId;

        if (attackerClanId == war.clan1Id)

        {
            war.clan1Stars += result.starsEarned;
        }

        else if (attackerClanId == war.clan2Id)

        {
            war.clan2Stars += result.starsEarned;
        }
    }

    // 广播战争状态更新

    std::ostringstream oss;

    oss << war.warId << "|" << war.clan1Stars << "|" << war.clan2Stars;

    std::string statusData = oss.str();

    auto broadcastToClan = [this, &statusData](const std::string& clanId) {
        if (clans.find(clanId) != clans.end())

        {
            for (const auto& memberId : clans[clanId].memberIds)

            {
                SOCKET socket = findSocketByPlayerId(memberId);

                if (socket != INVALID_SOCKET)

                {
                    sendPacket(socket, PACKET_CLAN_WAR_STATUS, statusData);
                }
            }
        }
    };

    broadcastToClan(war.clan1Id);

    broadcastToClan(war.clan2Id);
}

// ==================== 客户端处理 ====================

void clientHandler(SOCKET clientSocket, Server& server)

{
    uint32_t msgType;

    std::string msgData;

    while (Server::recvPacket(clientSocket, msgType, msgData))

    {
        std::lock_guard<std::mutex> lock(server.dataMutex);

        switch (msgType)

        {
                // ========== 基础功能 ==========

            case PACKET_LOGIN: {
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
                    server.onlinePlayers[clientSocket].trophies = std::stoi(trophiesStr);
                }

                std::cout << "[Login] User: " << playerId

                          << " (Trophies: " << server.onlinePlayers[clientSocket].trophies << ")" << std::endl;

                Server::sendPacket(clientSocket, PACKET_LOGIN, "Login Success");
            }

            break;

            case PACKET_UPLOAD_MAP: {
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

                // ========== 玩家匹配对战 ==========

            case PACKET_FIND_MATCH:

                server.addToMatchQueue(clientSocket);

                break;

            case PACKET_MATCH_CANCEL:

                server.removeFromMatchQueue(clientSocket);

                std::cout << "[Match] Player cancelled matchmaking." << std::endl;

                break;

            case PACKET_ATTACK_START: {
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

            case PACKET_ATTACK_RESULT: {
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
            }

            break;

                // ========== 部落系统 ==========

            case PACKET_CREATE_CLAN: {
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

                    AttackResult result = server.deserializeAttackResult(resultData);

                    server.processClanWarAttack(warId, result);
                }
            }

            break;

            default:

                break;
        }
    }

    // 玩家断开连接时的清理

    server.removeFromMatchQueue(clientSocket);

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