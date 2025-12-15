/****************************************************************
 * Project Name:  Clash_of_Clans
 * File Name:     ServerMain.cpp
 * File Function: 服务器端主程序入口
 * Author:        赵崇治
 * Update Date:   2025/12/14
 * License:       MIT License
 ****************************************************************/
#include <iostream>

#include "Server.h"

int main()
{
    try
    {
        Server server;

        server.run();
    }

    catch (const std::exception& e)
    {
        std::cerr << "Error: " << e.what() << std::endl;

        return -1;
    }

    return 0;
}