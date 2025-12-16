/****************************************************************
 * Project Name:  Clash_of_Clans
 * File Name:     CommandDispatcher.h
 * File Function: 
 * Author:        赵崇治
 * Update Date:   2025/12/17
 * License:       MIT License
 ****************************************************************/
#pragma once
#include <functional>
#include <map>
#include <string>
#include <WinSock2.h>

using PacketHandler = std::function<void(SOCKET, const std::string&)>;

class Router {
public:
    void Register(uint32_t packetType, PacketHandler handler);
    void Route(SOCKET client, uint32_t packetType, const std::string& data);

private:
    std::map<uint32_t, PacketHandler> routes;
};