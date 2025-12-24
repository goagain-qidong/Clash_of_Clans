/****************************************************************
 * Project Name:  Clash_of_Clans
 * File Name:     BuildingHealthBarUI.h
 * File Function: 建筑血条UI显示组件
 * Author:        刘相成
 * Update Date:   2025/12/14
 * License:       MIT License
 ****************************************************************/
#pragma once

#include "Buildings/BaseBuilding.h"
#include "cocos2d.h"

/**
 * @class BuildingHealthBarUI
 * @brief 建筑血条显示组件
 *
 * 功能：
 * - 显示建筑的当前生命值和最大生命值
 * - 实时更新血条显示
 * - 血量满时隐藏，为零时建筑消失
 * - 支持不同战斗状态
 */
class BuildingHealthBarUI : public cocos2d::Node
{
public:
    /**
     * @brief 创建血条UI
     * @param building 关联的建筑对象
     * @return BuildingHealthBarUI* UI指针
     */
    static BuildingHealthBarUI* create(BaseBuilding* building);

    /**
     * @brief 初始化
     * @param building 关联的建筑
     * @return bool 是否成功
     */
    virtual bool init(BaseBuilding* building);

    /**
     * @brief 更新血条显示
     * @param dt 时间增量
     */
    void update(float dt);

    /** @brief 显示血条 */
    void show();

    /** @brief 隐藏血条 */
    void hide();

    /**
     * @brief 设置血条是否始终可见
     * @param always 是否始终可见
     */
    void setAlwaysVisible(bool always) { _alwaysVisible = always; }

    /** @brief 检查关联的建筑是否已被销毁 */
    bool isBuildingDestroyed() const;

private:
    BuildingHealthBarUI() = default;

    BaseBuilding* _building = nullptr;  ///< 关联的建筑

    cocos2d::LayerColor* _healthBarBg = nullptr;    ///< 血条背景
    cocos2d::LayerColor* _healthBarFill = nullptr;  ///< 血条填充
    cocos2d::Label* _healthLabel = nullptr;         ///< 血量文字

    int _lastHealthValue = -1;    ///< 上一帧的生命值
    float _hideTimer = 0.0f;      ///< 隐藏计时器
    bool _isVisible = false;      ///< 血条是否可见
    bool _alwaysVisible = false;  ///< 是否始终可见

    static constexpr float BAR_WIDTH = 60.0f;   ///< 血条宽度
    static constexpr float BAR_HEIGHT = 8.0f;   ///< 血条高度
    static constexpr float HIDE_DELAY = 3.0f;   ///< 隐藏延迟
    static constexpr float OFFSET_Y = 20.0f;    ///< 高度偏移
};