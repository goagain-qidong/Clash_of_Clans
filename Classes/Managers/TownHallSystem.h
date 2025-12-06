#pragma once
/****************************************************************
  * Project Name:  Clash_of_Clans
  * File Name:     WallBuilding.cpp
  * File Function: 大本营系统
  * Author:        刘相成
  * Update Date:   2025/12/06
  * License:       MIT License
  ****************************************************************/
#include "cocos2d.h"
#include "Managers/ResourceManager.h"
#include "ui/CocosGUI.h"
#include <functional>
#include <map>
#include <string>
// TownHallBuilding 现在定义在 Buildings/TownHallBuilding.h
class TownHallBuilding;
// ==================== TownHallUpgradeUI ====================
class TownHallUpgradeUI : public cocos2d::Node
{
public:
    static TownHallUpgradeUI* create(TownHallBuilding* building);
    virtual bool init(TownHallBuilding* building);
    void show();
    void hide();
    bool isVisible() const;
    void setPositionNearBuilding(cocos2d::Node* building);
    using UpgradeCallback = std::function<void(bool success, int newLevel)>;
    void setUpgradeCallback(const UpgradeCallback& callback) { _upgradeCallback = callback; }

private:
    TownHallBuilding* _building;
    cocos2d::ui::Button* _upgradeButton;
    cocos2d::Label* _infoLabel;
    UpgradeCallback _upgradeCallback;
    void setupUI();
    void onUpgradeClicked(cocos2d::Ref* sender);
    void updateInfo();
};
// ==================== ResourceDisplayUI ====================
class ResourceDisplayUI : public cocos2d::Node
{
public:
    static ResourceDisplayUI* create();
    virtual bool init() override;
    void updateDisplay();
    void setPositionAtTopLeft();
    void setPositionAtTopRight();
    void setCustomPosition(const cocos2d::Vec2& position);
    void showResource(ResourceType type, bool show);

private:
    struct ResourceDisplay
    {
        cocos2d::Label* icon;
        cocos2d::Label* amount;
        cocos2d::Node* container;
    };
    std::map<ResourceType, ResourceDisplay> _displays;
    void setupResource(ResourceType type, const std::string& icon, const cocos2d::Color4B& color);
};