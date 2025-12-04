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