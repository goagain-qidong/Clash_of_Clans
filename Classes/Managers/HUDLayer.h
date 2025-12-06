#pragma once
/****************************************************************
 * Project Name:  Clash_of_Clans
 * File Name:     WallBuilding.cpp
 * File Function:  全新顶部资源栏
 * Author:        刘相成
 * Update Date:   2025/12/06
 * License:       MIT License
 ****************************************************************/
#include "cocos2d.h"
#include "Managers/ResourceManager.h"

class HUDLayer : public cocos2d::Layer {
public:
    static HUDLayer* create();
    virtual bool init() override;

    // 强制刷新显示
    void updateDisplay();

private:
    void createResourceNode(ResourceType type, const std::string& iconFile, int orderIndex);

    std::map<ResourceType, cocos2d::Label*> _amountLabels;
    std::map<ResourceType, cocos2d::Label*> _capacityLabels; // 仅金/水需要
};