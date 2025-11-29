/****************************************************************
 * Project Name:  Clash_of_Clans
 * File Name:     Building.cpp
 * File Function:
 * Author:        ÕÔ³çÖÎ
 * Update Date:   2025/11/29
 * License:       MIT License
 ****************************************************************/

#include "Building.h"

Building* Building::create(const std::string& filename)
{
    Building* pRet = new(std::nothrow) Building();
    if (pRet && pRet->init(filename))
    {
        pRet->autorelease();
        return pRet;
    }
    else
    {
        delete pRet;
        pRet = nullptr;
        return nullptr;
    }
}

bool Building::init(const std::string& filename)
{
    if (!Sprite::initWithFile(filename))
    {
        return false;
    }
    return true;
}