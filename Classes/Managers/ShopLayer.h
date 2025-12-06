#pragma once
/****************************************************************
 * Project Name:  Clash_of_Clans
 * File Name:     WallBuilding.cpp
 * File Function:  商店界面
 * Author:        刘相成
 * Update Date:   2025/12/06
 * License:       MIT License
 ****************************************************************/
#include "cocos2d.h"
#include "ui/CocosGUI.h"
#include "Managers/BuildingManager.h"

class ShopLayer : public cocos2d::Layer {
public:
    static ShopLayer* create();
    virtual bool init() override;

    void show();
    void hide();

private:
    void initUI();
    void createTabs();
    void loadCategory(const std::string& categoryName);

    // 创建单个商品项
    cocos2d::ui::Widget* createShopItem(const BuildingData& data);

    // 辅助检查
    int getCurrentTownHallLevel();
    int getBuildingCount(const std::string& name);

    cocos2d::ui::ListView* _scrollView;
    cocos2d::Node* _container;
    cocos2d::ui::Button* _closeBtn;
};
