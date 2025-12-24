/****************************************************************
 * Project Name:  Clash_of_Clans
 * File Name:     Server.cpp
 * File Function: 服务器主逻辑实现
 * Author:        赵崇治
 * Update Date:   2025/12/19
 * License:       MIT License
 ****************************************************************/
#define _CRT_SECURE_NO_WARNINGS
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#pragma comment(lib, "Ws2_32.lib")

#include "Server.h"
#include "NetworkUtils.h"
#include <algorithm>
#include <iostream>
#include <sstream>
#include <thread>

 // ==================== 构造与析构 ====================

Server::Server() : port(8888)
{
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
    {
        exit(EXIT_FAILURE);
    }

    // 初始化各模块
    playerRegistry = std::make_unique<PlayerRegistry>();
    clanHall = std::make_unique<ClanHall>(playerRegistry.get());
    clanWarRoom = std::make_unique<ClanWarRoom>(playerRegistry.get(), clanHall.get());
    matchmaker = std::make_unique<Matchmaker>();
    arenaSession = std::make_unique<ArenaSession>(playerRegistry.get());
    router = std::make_unique<Router>();

    registerRoutes();
}

Server::~Server()
{
    closesocket(serverSocket);
    WSACleanup();
}

// ==================== 路由注册 ====================

void Server::registerRoutes()
{
    // 登录
    router->Register(PACKET_LOGIN, [this](SOCKET client, const std::string& data) {
        std::istringstream iss(data);
        std::string playerId, playerName, trophiesStr;
        std::getline(iss, playerId, '|');
        std::getline(iss, playerName, '|');
        std::getline(iss, trophiesStr, '|');

        PlayerContext ctx;
        ctx.socket = client;
        ctx.playerId = playerId;
        ctx.playerName = playerName.empty() ? playerId : playerName;
        if (!trophiesStr.empty())
        {
            try { ctx.trophies = std::stoi(trophiesStr); }
            catch (...) { ctx.trophies = 0; }
        }

        playerRegistry->Register(client, ctx);

        std::cout << "[Login] User: " << playerId << " (Trophies: " << ctx.trophies << ")" << std::endl;
        sendPacket(client, PACKET_LOGIN, "Login Success");
        });

    // 上传地图
    router->Register(PACKET_UPLOAD_MAP, [this](SOCKET client, const std::string& data) {
        PlayerContext* player = playerRegistry->GetBySocket(client);
        if (player && !player->playerId.empty())
        {
            std::lock_guard<std::mutex> lock(dataMutex);
            savedMaps[player->playerId] = data;
            player->mapData = data;
            std::cout << "[Map] Saved for: " << player->playerId << " (Size: " << data.size() << ")" << std::endl;
        }
        });

    // 查询地图
    router->Register(PACKET_QUERY_MAP, [this](SOCKET client, const std::string& data) {
        std::lock_guard<std::mutex> lock(dataMutex);
        auto it = savedMaps.find(data);
        if (it != savedMaps.end())
        {
            sendPacket(client, PACKET_QUERY_MAP, it->second);
            std::cout << "[Query] Sent map of " << data << " to client." << std::endl;
        }
        else
        {
            sendPacket(client, PACKET_QUERY_MAP, "");
        }
        });

    // 攻击数据
    router->Register(PACKET_ATTACK_DATA, [](SOCKET, const std::string&) {
        std::cout << "[Attack] Received replay data." << std::endl;
        });

    // 用户列表
    router->Register(REQ_USER_LIST, [this](SOCKET client, const std::string&) {
        PlayerContext* player = playerRegistry->GetBySocket(client);
        if (!player || player->playerId.empty())
        {
            sendPacket(client, RESP_USER_LIST, "");
            return;
        }
        std::string userList = getUserListJson(player->playerId);
        sendPacket(client, RESP_USER_LIST, userList);
        std::cout << "[UserList] Sent to: " << player->playerId << std::endl;
        });

    // 匹配系统
    router->Register(PACKET_FIND_MATCH, [this](SOCKET client, const std::string&) {
        PlayerContext* player = playerRegistry->GetBySocket(client);
        if (!player) return;

        MatchQueueEntry entry;
        entry.socket = client;
        entry.playerId = player->playerId;
        entry.trophies = player->trophies;
        entry.queueTime = std::chrono::steady_clock::now();

        matchmaker->Enqueue(entry);
        std::cout << "[Match] " << player->playerId << " joined queue" << std::endl;

        // 处理匹配
        auto matches = matchmaker->ProcessQueue();
        for (auto& match : matches)
        {
            std::string msg1 = match.second.playerId + "|" + std::to_string(match.second.trophies);
            std::string msg2 = match.first.playerId + "|" + std::to_string(match.first.trophies);
            sendPacket(match.first.socket, PACKET_MATCH_FOUND, msg1);
            sendPacket(match.second.socket, PACKET_MATCH_FOUND, msg2);
            std::cout << "[Match] Matched: " << match.first.playerId << " vs " << match.second.playerId << std::endl;
        }
        });

    router->Register(PACKET_MATCH_CANCEL, [this](SOCKET client, const std::string&) {
        matchmaker->Remove(client);
        std::cout << "[Match] Player cancelled matchmaking." << std::endl;
        });

    // 开始攻击
    router->Register(PACKET_ATTACK_START, [this](SOCKET client, const std::string& data) {
        std::lock_guard<std::mutex> lock(dataMutex);
        auto it = savedMaps.find(data);
        if (it != savedMaps.end())
        {
            sendPacket(client, PACKET_ATTACK_START, it->second);
            PlayerContext* player = playerRegistry->GetBySocket(client);
            if (player)
            {
                std::cout << "[Battle] " << player->playerId << " attacking " << data << std::endl;
            }
        }
        });

    // 战斗状态列表
    router->Register(PACKET_BATTLE_STATUS_LIST, [this](SOCKET client, const std::string&) {
        std::string statusJson = arenaSession->GetBattleStatusListJson();
        sendPacket(client, PACKET_BATTLE_STATUS_LIST, statusJson);
        });

    // 攻击结果
    router->Register(PACKET_ATTACK_RESULT, [this](SOCKET client, const std::string& data) {
        try {
            AttackResult result = deserializeAttackResult(data);

            PlayerContext* attacker = playerRegistry->GetBySocket(client);
            if (attacker)
            {
                attacker->gold += result.goldLooted;
                attacker->elixir += result.elixirLooted;
                attacker->trophies += result.trophyChange;
            }

            PlayerContext* defender = playerRegistry->GetById(result.defenderId);
            if (defender)
            {
                defender->gold -= result.goldLooted;
                defender->elixir -= result.elixirLooted;
                defender->trophies -= result.trophyChange;
                sendPacket(defender->socket, PACKET_ATTACK_RESULT, data);
            }

            std::cout << "[Battle] Result - Stars: " << result.starsEarned
                << ", Gold: " << result.goldLooted << std::endl;
        }
        catch (const std::exception& e) {
            std::cout << "[Battle] Error: " << e.what() << std::endl;
        }
        });

    // 部落系统
    router->Register(PACKET_CREATE_CLAN, [this](SOCKET client, const std::string& data) {
        PlayerContext* player = playerRegistry->GetBySocket(client);
        if (!player) return;

        if (clanHall->CreateClan(player->playerId, data))
        {
            sendPacket(client, PACKET_CREATE_CLAN, "OK|" + player->clanId);
        }
        else
        {
            sendPacket(client, PACKET_CREATE_CLAN, "FAIL");
        }
        });

    router->Register(PACKET_JOIN_CLAN, [this](SOCKET client, const std::string& data) {
        PlayerContext* player = playerRegistry->GetBySocket(client);
        if (!player) return;

        if (clanHall->JoinClan(player->playerId, data))
        {
            sendPacket(client, PACKET_JOIN_CLAN, "OK");
        }
        else
        {
            sendPacket(client, PACKET_JOIN_CLAN, "FAIL");
        }
        });

    router->Register(PACKET_LEAVE_CLAN, [this](SOCKET client, const std::string&) {
        PlayerContext* player = playerRegistry->GetBySocket(client);
        if (!player) return;

        if (clanHall->LeaveClan(player->playerId))
        {
            sendPacket(client, PACKET_LEAVE_CLAN, "OK");
        }
        else
        {
            sendPacket(client, PACKET_LEAVE_CLAN, "FAIL");
        }
        });

    router->Register(PACKET_CLAN_LIST, [this](SOCKET client, const std::string&) {
        std::string clanList = clanHall->GetClanListJson();
        sendPacket(client, PACKET_CLAN_LIST, clanList);
        });

    router->Register(PACKET_CLAN_MEMBERS, [this](SOCKET client, const std::string& data) {
        std::string members = clanHall->GetClanMembersJson(data);
        sendPacket(client, PACKET_CLAN_MEMBERS, members);
        });

    // 部落战争
    router->Register(PACKET_CLAN_WAR_SEARCH, [this](SOCKET client, const std::string&) {
        PlayerContext* player = playerRegistry->GetBySocket(client);
        if (!player || player->clanId.empty())
        {
            sendPacket(client, PACKET_CLAN_WAR_SEARCH, "NO_CLAN");
            return;
        }
        clanWarRoom->AddToQueue(player->clanId);
        sendPacket(client, PACKET_CLAN_WAR_SEARCH, "SEARCHING");
        });

    router->Register(PACKET_CLAN_WAR_ATTACK, [this](SOCKET client, const std::string& data) {
        std::istringstream iss(data);
        std::string warId, targetId;
        std::getline(iss, warId, '|');
        std::getline(iss, targetId, '|');

        std::lock_guard<std::mutex> lock(dataMutex);
        auto it = savedMaps.find(targetId);
        if (it != savedMaps.end())
        {
            sendPacket(client, PACKET_CLAN_WAR_ATTACK, warId + "|" + it->second);
        }
        });

    router->Register(PACKET_CLAN_WAR_RESULT, [this](SOCKET, const std::string& data) {
        size_t pos = data.find('|');
        if (pos != std::string::npos)
        {
            std::string warId = data.substr(0, pos);
            std::string resultData = data.substr(pos + 1);
            try {
                AttackResult result = deserializeAttackResult(resultData);
                // 可以在此处理部落战攻击结果
                std::cout << "[ClanWar] Attack result for " << warId << std::endl;
            }
            catch (...) {}
        }
        });

    // PVP系统
    router->Register(PACKET_PVP_REQUEST, [this](SOCKET client, const std::string& data) {
        arenaSession->HandlePvpRequest(client, data);
        });

    router->Register(PACKET_PVP_ACTION, [this](SOCKET client, const std::string& data) {
        arenaSession->HandlePvpAction(client, data);
        });

    router->Register(PACKET_PVP_END, [this](SOCKET client, const std::string&) {
        PlayerContext* player = playerRegistry->GetBySocket(client);
        if (player && !player->playerId.empty())
        {
            arenaSession->EndSession(player->playerId);
        }
        });

    router->Register(PACKET_SPECTATE_REQUEST, [this](SOCKET client, const std::string& data) {
        arenaSession->HandleSpectateRequest(client, data);
        });

    // 部落战争增强
    router->Register(PACKET_CLAN_WAR_MEMBER_LIST, [this](SOCKET client, const std::string& data) {
        PlayerContext* player = playerRegistry->GetBySocket(client);
        if (!player) return;
        std::string json = clanWarRoom->GetMemberListJson(data, player->playerId);
        sendPacket(client, PACKET_CLAN_WAR_MEMBER_LIST, json);
        });

    router->Register(PACKET_CLAN_WAR_ATTACK_START, [this](SOCKET client, const std::string& data) {
        size_t pos = data.find('|');
        if (pos != std::string::npos)
        {
            std::string warId = data.substr(0, pos);
            std::string targetId = data.substr(pos + 1);
            clanWarRoom->HandleAttackStart(client, warId, targetId);
        }
        });

    router->Register(PACKET_CLAN_WAR_ATTACK_END, [this](SOCKET, const std::string& data) {
        try {
            AttackRecord record;
            std::istringstream iss(data);
            std::string warId, starsStr, destructionStr;
            std::getline(iss, warId, '|');
            std::getline(iss, record.attackerId, '|');
            std::getline(iss, record.attackerName, '|');
            std::getline(iss, starsStr, '|');
            std::getline(iss, destructionStr, '|');

            if (!starsStr.empty()) record.starsEarned = std::stoi(starsStr);
            if (!destructionStr.empty()) record.destructionRate = std::stof(destructionStr);
            record.attackTime = std::chrono::steady_clock::now();

            clanWarRoom->HandleAttackEnd(warId, record);
        }
        catch (const std::exception& e) {
            std::cout << "[ClanWar] Error parsing attack end: " << e.what() << std::endl;
        }
        });

    router->Register(PACKET_CLAN_WAR_SPECTATE, [this](SOCKET client, const std::string& data) {
        size_t pos = data.find('|');
        if (pos != std::string::npos)
        {
            std::string warId = data.substr(0, pos);
            std::string targetId = data.substr(pos + 1);
            clanWarRoom->HandleSpectate(client, warId, targetId);
        }
        });
}

// ==================== 客户端处理 ====================
void clientHandler(SOCKET clientSocket, Server& server)
{
    uint32_t    msgType;
    std::string msgData;

    while (recvPacket(clientSocket, msgType, msgData))
    {
        server.router->Route(clientSocket, msgType, msgData);
    }

    // 玩家断开连接清理
    PlayerContext* player = server.playerRegistry->GetBySocket(clientSocket);
    std::string    playerId;
    if (player)
    {
        playerId = player->playerId;
    }

    server.matchmaker->Remove(clientSocket);

    if (!playerId.empty())
    {
        // 清理所有PVP相关会话
        server.arenaSession->CleanupPlayerSessions(playerId);
    }

    server.closeClientSocket(clientSocket);
}

// ==================== 服务器生命周期 ====================

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

            PlayerContext ctx;
            ctx.socket = clientSocket;
            playerRegistry->Register(clientSocket, ctx);

            std::thread clientThread(clientHandler, clientSocket, std::ref(*this));
            clientThread.detach();
        }
    }
}

void Server::closeClientSocket(SOCKET clientSocket)
{
    PlayerContext* player = playerRegistry->GetBySocket(clientSocket);
    std::string playerId;
    if (player)
    {
        playerId = player->playerId;
    }

    playerRegistry->Unregister(clientSocket);
    closesocket(clientSocket);

    std::cout << "[Disconnect] Client: " << clientSocket;
    if (!playerId.empty())
    {
        std::cout << " (Player: " << playerId << ")";
    }
    std::cout << std::endl;
}

// ==================== 辅助函数 ====================

std::string Server::serializeAttackResult(const AttackResult& result)
{
    std::ostringstream oss;
    oss << result.attackerId << "|" << result.defenderId << "|"
        << result.starsEarned << "|" << result.goldLooted << "|"
        << result.elixirLooted << "|" << result.trophyChange << "|"
        << result.replayData;
    return oss.str();
}

AttackResult Server::deserializeAttackResult(const std::string& data)
{
    AttackResult result;
    std::istringstream iss(data);
    std::string token;

    std::getline(iss, result.attackerId, '|');
    std::getline(iss, result.defenderId, '|');
    std::getline(iss, token, '|'); result.starsEarned = std::stoi(token);
    std::getline(iss, token, '|'); result.goldLooted = std::stoi(token);
    std::getline(iss, token, '|'); result.elixirLooted = std::stoi(token);
    std::getline(iss, token, '|'); result.trophyChange = std::stoi(token);
    std::getline(iss, result.replayData);

    return result;
}

std::string Server::getUserListJson(const std::string& requesterId)
{
    auto allPlayers = playerRegistry->GetAllSnapshot();

    std::ostringstream oss;
    bool first = true;

    for (const auto& pair : allPlayers)
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