/****************************************************************
 * Project Name:  Clash_of_Clans
 * File Name:     ShopLayer.h
 * File Function: 商店界面
 * Author:        刘相成
 * Update Date:   2025/12/06
 * License:       MIT License
 ****************************************************************/
#pragma once

#include "cocos2d.h"
#include "ui/CocosGUI.h"
#include "Managers/BuildingManager.h"

/**
 * @class ShopLayer
 * @brief 商店界面层
 */
class ShopLayer : public cocos2d::Layer {
public:
    /**
     * @brief 创建商店层
     * @return ShopLayer* 商店层指针
     */
    static ShopLayer* create();

    virtual bool init() override;

    /** @brief 显示商店 */
    void show();

    /** @brief 隐藏商店 */
    void hide();

private:
    void initUI();          ///< 初始化UI
    void createTabs();      ///< 创建标签页
    void loadCategory(const std::string& categoryName);  ///< 加载分类

    /**
     * @brief 创建单个商品项
     * @param data 建筑数据
     * @return cocos2d::ui::Widget* 商品项控件
     */
    cocos2d::ui::Widget* createShopItem(const BuildingData& data);

    /** @brief 获取当前大本营等级 */
    int getCurrentTownHallLevel();

    /**
     * @brief 获取建筑数量
     * @param name 建筑名称
     * @return int 数量
     */
    int getBuildingCount(const std::string& name);

    cocos2d::ui::ListView* _scrollView;  ///< 滚动视图
    cocos2d::Node* _container;           ///< 容器
    cocos2d::ui::Button* _closeBtn;      ///< 关闭按钮
};
