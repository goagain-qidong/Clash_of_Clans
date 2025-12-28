/****************************************************************
 * Project Name:  Clash_of_Clans
 * File Name:     Server.cpp
 * File Function: 服务器主逻辑实现
 * Author:        赵崇治
 * Update Date:   2025/12/25
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

// ============================================================================
// 协议格式常量
// ============================================================================
namespace {
    constexpr char kFieldSeparator = '|';
}

// ============================================================================
// 构造与析构
// ============================================================================

Server::Server() : port(8888) {
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        std::cerr << "[Server] WSAStartup 失败" << std::endl;
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

Server::~Server() {
    closesocket(serverSocket);
    WSACleanup();
}

// ============================================================================
// 路由注册
// ============================================================================

void Server::registerRoutes() {
    // ======================== 登录处理 ========================
    router->Register(PACKET_LOGIN,
        [this](SOCKET client, const std::string& data) {
            std::istringstream iss(data);
            std::string playerId, playerName, trophiesStr, clanId;
            std::getline(iss, playerId, kFieldSeparator);
            std::getline(iss, playerName, kFieldSeparator);
            std::getline(iss, trophiesStr, kFieldSeparator);
            std::getline(iss, clanId, kFieldSeparator);

            std::cout << "[Login] 收到登录请求: playerId=" << playerId 
                      << ", playerName=" << playerName
                      << ", trophies=" << trophiesStr
                      << ", clanId=" << (clanId.empty() ? "(空)" : clanId) << std::endl;

            PlayerContext ctx;
            ctx.socket = client;
            ctx.playerId = playerId;
            ctx.playerName = playerName.empty() ? playerId : playerName;
            ctx.clanId = clanId;  // 恢复部落归属
            if (!trophiesStr.empty()) {
                try {
                    ctx.trophies = std::stoi(trophiesStr);
                } catch (...) {
                    ctx.trophies = 0;
                }
            }

            playerRegistry->Register(client, ctx);

            // 如果玩家有部落ID，确保部落记录中包含该玩家
            if (!clanId.empty()) {
                std::cout << "[Login] 调用 EnsurePlayerInClan: " << playerId << " -> " << clanId << std::endl;
                clanHall->EnsurePlayerInClan(playerId, clanId);
                
                // 再次获取玩家信息确认 clanId 是否设置成功
                PlayerContext* player = playerRegistry->GetById(playerId);
                if (player) {
                    std::cout << "[Login] 登录后玩家clanId=" << (player->clanId.empty() ? "(空)" : player->clanId) << std::endl;
                }
            }

            std::cout << "[Login] 用户: " << playerId
                      << " (奖杯: " << ctx.trophies 
                      << ", 部落: " << (clanId.empty() ? "无" : clanId) << ")" << std::endl;
            sendPacket(client, PACKET_LOGIN, "Login Success");
        });

    // ======================== 地图操作 ========================
    router->Register(PACKET_UPLOAD_MAP,
        [this](SOCKET client, const std::string& data) {
            PlayerContext* player = playerRegistry->GetBySocket(client);
            if (player != nullptr && !player->playerId.empty()) {
                std::lock_guard<std::mutex> lock(dataMutex);
                savedMaps[player->playerId] = data;
                player->mapData = data;
                std::cout << "[Map] 已保存玩家 " << player->playerId
                          << " 的地图 (大小: " << data.size() << ")" << std::endl;
            }
        });

    router->Register(PACKET_QUERY_MAP,
        [this](SOCKET client, const std::string& data) {
            std::lock_guard<std::mutex> lock(dataMutex);
            auto it = savedMaps.find(data);
            if (it != savedMaps.end()) {
                sendPacket(client, PACKET_QUERY_MAP, it->second);
                std::cout << "[Query] 已发送玩家 " << data << " 的地图" << std::endl;
            } else {
                sendPacket(client, PACKET_QUERY_MAP, "");
            }
        });

    // ======================== 用户列表 ========================
    router->Register(PACKET_USER_LIST_REQ,
        [this](SOCKET client, const std::string&) {
            PlayerContext* player = playerRegistry->GetBySocket(client);
            if (player == nullptr || player->playerId.empty()) {
                sendPacket(client, PACKET_USER_LIST_RESP, "");
                return;
            }
            std::string userList = getUserListJson(player->playerId);
            sendPacket(client, PACKET_USER_LIST_RESP, userList);
            std::cout << "[UserList] 已发送给: " << player->playerId << std::endl;
        });

    // ======================== 匹配系统 ========================
    router->Register(PACKET_MATCH_FIND,
        [this](SOCKET client, const std::string&) {
            PlayerContext* player = playerRegistry->GetBySocket(client);
            if (player == nullptr) {
                return;
            }

            MatchQueueEntry entry;
            entry.socket = client;
            entry.playerId = player->playerId;
            entry.trophies = player->trophies;
            entry.queueTime = std::chrono::steady_clock::now();

            matchmaker->Enqueue(entry);
            std::cout << "[Match] " << player->playerId << " 加入匹配队列" << std::endl;

            // 处理匹配
            auto matches = matchmaker->ProcessQueue();
            for (auto& match : matches) {
                std::string msg1 = match.second.playerId + kFieldSeparator +
                                   std::to_string(match.second.trophies);
                std::string msg2 = match.first.playerId + kFieldSeparator +
                                   std::to_string(match.first.trophies);
                sendPacket(match.first.socket, PACKET_MATCH_FOUND, msg1);
                sendPacket(match.second.socket, PACKET_MATCH_FOUND, msg2);
                std::cout << "[Match] 匹配成功: " << match.first.playerId
                          << " vs " << match.second.playerId << std::endl;
            }
        });

    router->Register(PACKET_MATCH_CANCEL,
        [this](SOCKET client, const std::string&) {
            matchmaker->Remove(client);
            std::cout << "[Match] 玩家取消匹配" << std::endl;
        });

    // ======================== 攻击处理 ========================
    router->Register(PACKET_ATTACK_START,
        [this](SOCKET client, const std::string& data) {
            std::lock_guard<std::mutex> lock(dataMutex);
            auto it = savedMaps.find(data);
            if (it != savedMaps.end()) {
                sendPacket(client, PACKET_ATTACK_START, it->second);
                PlayerContext* player = playerRegistry->GetBySocket(client);
                if (player != nullptr) {
                    std::cout << "[Battle] " << player->playerId
                              << " 攻击 " << data << std::endl;
                }
            }
        });

    router->Register(PACKET_ATTACK_RESULT,
        [this](SOCKET client, const std::string& data) {
            try {
                AttackResult result = deserializeAttackResult(data);

                PlayerContext* attacker = playerRegistry->GetBySocket(client);
                if (attacker != nullptr) {
                    attacker->gold += result.goldLooted;
                    attacker->elixir += result.elixirLooted;
                    attacker->trophies += result.trophyChange;
                }

                PlayerContext* defender = playerRegistry->GetById(result.defenderId);
                if (defender != nullptr) {
                    defender->gold -= result.goldLooted;
                    defender->elixir -= result.elixirLooted;
                    defender->trophies -= result.trophyChange;
                    sendPacket(defender->socket, PACKET_ATTACK_RESULT, data);
                }

                std::cout << "[Battle] 结果 - 星数: " << result.starsEarned
                          << ", 金币: " << result.goldLooted << std::endl;
            } catch (const std::exception& e) {
                std::cout << "[Battle] 错误: " << e.what() << std::endl;
            }
        });

    // ======================== 战斗状态 ========================
    router->Register(PACKET_BATTLE_STATUS_LIST,
        [this](SOCKET client, const std::string&) {
            std::string statusJson = arenaSession->GetBattleStatusListJson();
            sendPacket(client, PACKET_BATTLE_STATUS_LIST, statusJson);
        });

    // ======================== 部落系统 ========================
    router->Register(PACKET_CLAN_CREATE,
        [this](SOCKET client, const std::string& data) {
            PlayerContext* player = playerRegistry->GetBySocket(client);
            if (player == nullptr) {
                return;
            }

            if (clanHall->CreateClan(player->playerId, data)) {
                sendPacket(client, PACKET_CLAN_CREATE, 
                           "OK" + std::string(1, kFieldSeparator) + player->clanId);
            } else {
                sendPacket(client, PACKET_CLAN_CREATE, "FAIL");
            }
        });

    router->Register(PACKET_CLAN_JOIN,
        [this](SOCKET client, const std::string& data) {
            PlayerContext* player = playerRegistry->GetBySocket(client);
            if (player == nullptr) {
                return;
            }

            if (clanHall->JoinClan(player->playerId, data)) {
                sendPacket(client, PACKET_CLAN_JOIN, "OK");
            } else {
                sendPacket(client, PACKET_CLAN_JOIN, "FAIL");
            }
        });

    router->Register(PACKET_CLAN_LEAVE,
        [this](SOCKET client, const std::string&) {
            PlayerContext* player = playerRegistry->GetBySocket(client);
            if (player == nullptr) {
                return;
            }

            if (clanHall->LeaveClan(player->playerId)) {
                sendPacket(client, PACKET_CLAN_LEAVE, "OK");
            } else {
                sendPacket(client, PACKET_CLAN_LEAVE, "FAIL");
            }
        });

    router->Register(PACKET_CLAN_LIST,
        [this](SOCKET client, const std::string&) {
            std::string clanList = clanHall->GetClanListJson();
            sendPacket(client, PACKET_CLAN_LIST, clanList);
        });

    router->Register(PACKET_CLAN_MEMBERS,
        [this](SOCKET client, const std::string& data) {
            std::string members = clanHall->GetClanMembersJson(data);
            sendPacket(client, PACKET_CLAN_MEMBERS, members);
        });

    // ======================== 部落聊天 ========================
    router->Register(PACKET_CLAN_CHAT,
        [this](SOCKET client, const std::string& data) {
            PlayerContext* player = playerRegistry->GetBySocket(client);
            if (player == nullptr) {
                std::cout << "[Chat] 聊天失败: 玩家未找到 (socket=" << client << ")" << std::endl;
                return;
            }
            
            std::cout << "[Chat] 收到消息: playerId=" << player->playerId 
                      << ", clanId=" << player->clanId 
                      << ", msg=" << data << std::endl;
            
            if (player->clanId.empty()) {
                std::cout << "[Chat] 聊天失败: 玩家 " << player->playerId << " 未加入部落 (clanId为空)" << std::endl;
                return;
            }

            // 获取部落所有成员
            std::vector<std::string> memberIds = clanHall->GetClanMemberIds(player->clanId);
            
            // 构建聊天消息: sender|message
            std::string chatMessage = player->playerName + kFieldSeparator + data;
            
            std::cout << "[Chat] " << player->playerName << " 在部落 " 
                      << player->clanId << " 发送消息: " << data 
                      << " (成员数: " << memberIds.size() << ")" << std::endl;

            // 广播给部落所有在线成员（包括发送者自己，以便确认消息已发送）
            for (const auto& memberId : memberIds) {
                PlayerContext* member = playerRegistry->GetById(memberId);
                if (member != nullptr && member->socket != INVALID_SOCKET) {
                    sendPacket(member->socket, PACKET_CHAT_MESSAGE, chatMessage);
                }
            }
        });

    // ======================== 部落战争 ========================
    router->Register(PACKET_WAR_SEARCH,
        [this](SOCKET client, const std::string&) {
            PlayerContext* player = playerRegistry->GetBySocket(client);
            if (player == nullptr || player->clanId.empty()) {
                sendPacket(client, PACKET_WAR_SEARCH, "NO_CLAN");
                return;
            }
            clanWarRoom->AddToQueue(player->clanId);
            sendPacket(client, PACKET_WAR_SEARCH, "SEARCHING");
        });

    router->Register(PACKET_WAR_ATTACK,
        [this](SOCKET client, const std::string& data) {
            std::istringstream iss(data);
            std::string warId, targetId;
            std::getline(iss, warId, kFieldSeparator);
            std::getline(iss, targetId, kFieldSeparator);

            std::lock_guard<std::mutex> lock(dataMutex);
            auto it = savedMaps.find(targetId);
            if (it != savedMaps.end()) {
                sendPacket(client, PACKET_WAR_ATTACK, 
                           warId + kFieldSeparator + it->second);
            }
        });

    router->Register(PACKET_WAR_RESULT,
        [this](SOCKET, const std::string& data) {
            size_t pos = data.find(kFieldSeparator);
            if (pos != std::string::npos) {
                std::string warId = data.substr(0, pos);
                std::string resultData = data.substr(pos + 1);
                try {
                    AttackResult result = deserializeAttackResult(resultData);
                    std::cout << "[ClanWar] 攻击结果: 战争 " << warId << std::endl;
                } catch (...) {}
            }
        });

    // ======================== PVP 系统 ========================
    router->Register(PACKET_PVP_REQUEST,
        [this](SOCKET client, const std::string& data) {
            arenaSession->HandlePvpRequest(client, data);
        });

    router->Register(PACKET_PVP_ACTION,
        [this](SOCKET client, const std::string& data) {
            arenaSession->HandlePvpAction(client, data);
        });

    router->Register(PACKET_PVP_END,
        [this](SOCKET client, const std::string&) {
            PlayerContext* player = playerRegistry->GetBySocket(client);
            if (player != nullptr && !player->playerId.empty()) {
                arenaSession->EndSession(player->playerId);
            }
        });

    router->Register(PACKET_SPECTATE_REQUEST,
        [this](SOCKET client, const std::string& data) {
            arenaSession->HandleSpectateRequest(client, data);
        });

    // ======================== 部落战争增强 ========================
    router->Register(PACKET_WAR_MEMBER_LIST,
        [this](SOCKET client, const std::string& data) {
            PlayerContext* player = playerRegistry->GetBySocket(client);
            if (player == nullptr) {
                return;
            }
            std::string json = clanWarRoom->GetMemberListJson(data, player->playerId);
            sendPacket(client, PACKET_WAR_MEMBER_LIST, json);
        });

    router->Register(PACKET_WAR_ATTACK_START,
        [this](SOCKET client, const std::string& data) {
            size_t pos = data.find(kFieldSeparator);
            if (pos != std::string::npos) {
                std::string warId = data.substr(0, pos);
                std::string targetId = data.substr(pos + 1);
                clanWarRoom->HandleAttackStart(client, warId, targetId);
            }
        });

    router->Register(PACKET_WAR_ATTACK_END,
        [this](SOCKET, const std::string& data) {
            try {
                AttackRecord record;
                std::istringstream iss(data);
                std::string warId, starsStr, destructionStr;
                std::getline(iss, warId, kFieldSeparator);
                std::getline(iss, record.attackerId, kFieldSeparator);
                std::getline(iss, record.attackerName, kFieldSeparator);
                std::getline(iss, starsStr, kFieldSeparator);
                std::getline(iss, destructionStr, kFieldSeparator);

                if (!starsStr.empty()) {
                    record.starsEarned = std::stoi(starsStr);
                }
                if (!destructionStr.empty()) {
                    record.destructionRate = std::stof(destructionStr);
                }
                record.attackTime = std::chrono::steady_clock::now();

                clanWarRoom->HandleAttackEnd(warId, record);
            } catch (const std::exception& e) {
                std::cout << "[ClanWar] 解析攻击结束数据错误: " << e.what() << std::endl;
            }
        });

    router->Register(PACKET_WAR_SPECTATE,
        [this](SOCKET client, const std::string& data) {
            size_t pos = data.find(kFieldSeparator);
            if (pos != std::string::npos) {
                std::string warId = data.substr(0, pos);
                std::string targetId = data.substr(pos + 1);
                clanWarRoom->HandleSpectate(client, warId, targetId);
            }
        });

    router->Register(PACKET_WAR_END,
        [this](SOCKET client, const std::string& data) {
            PlayerContext* player = playerRegistry->GetBySocket(client);
            if (player == nullptr) {
                return;
            }

            if (!data.empty()) {
                clanWarRoom->EndWar(data);
            } else {
                std::string warId = clanWarRoom->GetActiveWarIdForPlayer(player->playerId);
                if (!warId.empty()) {
                    clanWarRoom->EndWar(warId);
                }
            }
        });
}

// ============================================================================
// 客户端连接处理
// ============================================================================

void clientHandler(SOCKET clientSocket, Server& server) {
    uint32_t msgType;
    std::string msgData;

    while (recvPacket(clientSocket, msgType, msgData)) {
        server.router->Route(clientSocket, msgType, msgData);
    }

    // 玩家断开连接时的清理工作
    PlayerContext* player = server.playerRegistry->GetBySocket(clientSocket);
    std::string playerId;
    if (player != nullptr) {
        playerId = player->playerId;
    }

    server.matchmaker->Remove(clientSocket);

    if (!playerId.empty()) {
        // 清理 PVP 相关会话
        server.arenaSession->CleanupPlayerSessions(playerId);
        // 清理部落战争相关会话
        server.clanWarRoom->CleanupPlayerSessions(playerId);
    }

    server.closeClientSocket(clientSocket);
}

// ============================================================================
// 服务器生命周期
// ============================================================================

void Server::run() {
    createAndBindSocket();
    handleConnections();
}

void Server::createAndBindSocket() {
    serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket == INVALID_SOCKET) {
        std::cerr << "[Server] 创建 socket 失败" << std::endl;
        exit(EXIT_FAILURE);
    }

    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(port);

    if (bind(serverSocket, 
             reinterpret_cast<struct sockaddr*>(&serverAddr),
             sizeof(serverAddr)) == SOCKET_ERROR) {
        closesocket(serverSocket);
        std::cerr << "[Server] 绑定端口失败" << std::endl;
        exit(EXIT_FAILURE);
    }
}

void Server::handleConnections() {
    listen(serverSocket, SOMAXCONN);

    std::cout << "=== Clash of Clans 服务器 ===" << std::endl;
    std::cout << "服务器已启动，端口: " << port << std::endl;
    std::cout << "等待玩家连接..." << std::endl;

    while (true) {
        sockaddr_in clientAddr;
        int clientAddrLen = sizeof(clientAddr);
        SOCKET clientSocket = accept(
            serverSocket, 
            reinterpret_cast<struct sockaddr*>(&clientAddr),
            &clientAddrLen);

        if (clientSocket != INVALID_SOCKET) {
            std::cout << "[Connect] 新客户端: " << clientSocket << std::endl;

            PlayerContext ctx;
            ctx.socket = clientSocket;
            playerRegistry->Register(clientSocket, ctx);

            std::thread clientThread(clientHandler, clientSocket, std::ref(*this));
            clientThread.detach();
        }
    }
}

void Server::closeClientSocket(SOCKET clientSocket) {
    PlayerContext* player = playerRegistry->GetBySocket(clientSocket);
    std::string playerId;
    if (player != nullptr) {
        playerId = player->playerId;
    }

    playerRegistry->Unregister(clientSocket);
    closesocket(clientSocket);

    std::cout << "[Disconnect] 客户端: " << clientSocket;
    if (!playerId.empty()) {
        std::cout << " (玩家: " << playerId << ")";
    }
    std::cout << std::endl;
}

// ============================================================================
// 辅助函数
// ============================================================================

std::string Server::serializeAttackResult(const AttackResult& result) {
    std::ostringstream oss;
    oss << result.attackerId << kFieldSeparator
        << result.defenderId << kFieldSeparator
        << result.starsEarned << kFieldSeparator
        << result.goldLooted << kFieldSeparator
        << result.elixirLooted << kFieldSeparator
        << result.trophyChange << kFieldSeparator
        << result.replayData;
    return oss.str();
}

AttackResult Server::deserializeAttackResult(const std::string& data) {
    AttackResult result;
    std::istringstream iss(data);
    std::string token;

    std::getline(iss, result.attackerId, kFieldSeparator);
    std::getline(iss, result.defenderId, kFieldSeparator);

    std::getline(iss, token, kFieldSeparator);
    if (!token.empty()) {
        result.starsEarned = std::stoi(token);
    }

    std::getline(iss, token, kFieldSeparator);
    if (!token.empty()) {
        result.goldLooted = std::stoi(token);
    }

    std::getline(iss, token, kFieldSeparator);
    if (!token.empty()) {
        result.elixirLooted = std::stoi(token);
    }

    std::getline(iss, token, kFieldSeparator);
    if (!token.empty()) {
        result.trophyChange = std::stoi(token);
    }

    std::getline(iss, result.replayData);

    return result;
}

std::string Server::getUserListJson(const std::string& requesterId) {
    auto allPlayers = playerRegistry->GetAllSnapshot();

    std::ostringstream oss;
    bool first = true;

    for (const auto& pair : allPlayers) {
        const auto& player = pair.second;
        if (player.playerId.empty() || player.playerId == requesterId) {
            continue;
        }

        if (!first) {
            oss << kFieldSeparator;
        }
        first = false;

        oss << player.playerId << ","
            << player.playerName << ","
            << player.trophies << ","
            << player.gold << ","
            << player.elixir;
    }

    return oss.str();
}