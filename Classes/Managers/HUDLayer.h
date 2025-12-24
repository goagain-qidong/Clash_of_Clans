/****************************************************************
/****************************************************************
 * Project Name:  Clash_of_Clans
 * File Name:     HUDLayer.h
 * File Function: 全新顶部资源栏
 * Author:        刘相成
 * Update Date:   2025/12/06
 * License:       MIT License
 ****************************************************************/
#pragma once

#include "cocos2d.h"
#include "Managers/ResourceManager.h"

/**
 * @class HUDLayer
 * @brief 顶部资源栏显示层
 */
class HUDLayer : public cocos2d::Layer {
public:
    /**
     * @brief 创建HUD层
     * @return HUDLayer* HUD层指针
     */
    static HUDLayer* create();

    virtual bool init() override;
    virtual ~HUDLayer();

    /** @brief 强制刷新显示 */
    void updateDisplay();

private:
    /**
     * @brief 创建资源节点
     * @param type 资源类型
     * @param iconFile 图标文件
     * @param orderIndex 顺序索引
     */
    void createResourceNode(ResourceType type, const std::string& iconFile, int orderIndex);

    std::map<ResourceType, cocos2d::Label*> _amountLabels;    ///< 数量标签
    std::map<ResourceType, cocos2d::Label*> _capacityLabels;  ///< 容量标签
};