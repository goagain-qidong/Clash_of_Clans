/****************************************************************
 * Project Name:  Clash_of_Clans
 * File Name:     CommandDispatcher.cpp
 * File Function: 命令路由分发器实现
 * Author:        赵崇治
 * Update Date:   2025/12/19
 * License:       MIT License
 ****************************************************************/
#include "CommandDispatcher.h"
#include <iostream>

void Router::Register(uint32_t packetType, PacketHandler handler)
{
    routes[packetType] = handler;
}

void Router::Route(SOCKET client, uint32_t packetType, const std::string& data)
{
    auto it = routes.find(packetType);
    if (it != routes.end())
    {
        it->second(client, data);
    }
    else
    {
        std::cout << "[Router] Unknown packet type: " << packetType << std::endl;
    }
}