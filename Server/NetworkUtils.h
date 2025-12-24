/****************************************************************
 * Project Name:  Clash_of_Clans
 * File Name:     NetworkUtils.h
 * File Function: 网络工具函数
 * Author:        赵崇治
 * Update Date:   2025/12/19
 * License:       MIT License
 ****************************************************************/
#pragma once
#include <WinSock2.h>
#include <string>
#include <cstdint>
#include "Protocol.h"

bool sendPacket(SOCKET socket, uint32_t type, const std::string& data);
bool recvPacket(SOCKET socket, uint32_t& outType, std::string& outData);
bool recvFixedAmount(SOCKET socket, char* buffer, int totalBytes);