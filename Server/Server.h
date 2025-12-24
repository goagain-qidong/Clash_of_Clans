/****************************************************************
 * Project Name:  Clash_of_Clans
 * File Name:     Server.h
 * File Function: 服务器主逻辑
 * Author:        赵崇治
 * Update Date:   2025/12/19
 * License:       MIT License
 ****************************************************************/
#pragma once

#ifndef _SERVER_H_
#define _SERVER_H_

#include <WinSock2.h>
#include <map>
#include <mutex>
#include <string>
#include <memory>

#include "Protocol.h"
#include "ClanInfo.h"
#include "WarModels.h"
#include "PlayerRegistry.h"
#include "ClanHall.h"
#include "ClanWarRoom.h"
#include "MatchMaker.h"
#include "ArenaSession.h"
#include "CommandDispatcher.h"

class Server
{
public:
    Server();
    ~Server();

    void run();

    friend void clientHandler(SOCKET clientSocket, Server& server);

private:
    // ==================== 网络基础 ====================
    WSADATA wsaData;
    SOCKET serverSocket;
    struct sockaddr_in serverAddr;
    int port;

    // ==================== 模块化组件 ====================
    std::unique_ptr<PlayerRegistry> playerRegistry;
    std::unique_ptr<ClanHall> clanHall;
    std::unique_ptr<ClanWarRoom> clanWarRoom;
    std::unique_ptr<Matchmaker> matchmaker;
    std::unique_ptr<ArenaSession> arenaSession;
    std::unique_ptr<Router> router;

    // ==================== 共享数据 ====================
    std::map<std::string, std::string> savedMaps;  // playerId -> mapData
    std::map<std::string, PlayerContext> playerDatabase;  // playerId -> PlayerContext (持久化)
    std::mutex dataMutex;

    // ==================== 网络函数 ====================
    void createAndBindSocket();
    void handleConnections();
    void closeClientSocket(SOCKET clientSocket);

    // ==================== 路由注册 ====================
    void registerRoutes();

    // ==================== 辅助函数 ====================
    std::string serializeAttackResult(const AttackResult& result);
    AttackResult deserializeAttackResult(const std::string& data);
    std::string getUserListJson(const std::string& requesterId);
};

#endif