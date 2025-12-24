/****************************************************************
 * Project Name:  Clash_of_Clans
 * File Name:     NetworkUtils.cpp
 * File Function: 网络工具函数实现
 * Author:        赵崇治
 * Update Date:   2025/12/19
 * License:       MIT License
 ****************************************************************/
#include "NetworkUtils.h"
#include <vector>

bool recvFixedAmount(SOCKET socket, char* buffer, int totalBytes)
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

bool sendPacket(SOCKET socket, uint32_t type, const std::string& data)
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

bool recvPacket(SOCKET socket, uint32_t& outType, std::string& outData)
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