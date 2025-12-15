#pragma once
/****************************************************************
 * Project Name:  Clash_of_Clans
 * File Name:     UnitHealthBarUI.h
 * File Function: 单位（小兵）血条UI显示组件
 * Author:        薛毓哲
 * Update Date:   2025/12/14
 * License:       MIT License
 ****************************************************************/

#include "Unit/unit.h"
#include "cocos2d.h"

/**
 * @class UnitHealthBarUI
 * @brief 单位血条显示组件
 *
 * 功能：
 * 1. 显示单位的当前生命值和最大生命值
 * 2. 实时更新血条显示（随着单位受伤而下降）
 * 3. 血量满时隐藏，为零时单位消失
 * 4. 支持战斗状态显示
 */
class UnitHealthBarUI : public cocos2d::Node
{
public:
    /**
     * @brief 创建血条UI
     * @param unit 关联的单位对象
     */
    static UnitHealthBarUI* create(Unit* unit);

    virtual bool init(Unit* unit);

    /**
     * @brief 更新血条显示
     */
    void update(float dt);

    /**
     * @brief 显示血条
     */
    void show();

    /**
     * @brief 隐藏血条
     */
    void hide();

    /**
     * @brief 设置血条是否始终可见（用于战斗时显示）
     */
    void setAlwaysVisible(bool always) { _alwaysVisible = always; }

    /**
     * @brief 检查关联的单位是否已死亡
     */
    bool isUnitDead() const;

private:
    UnitHealthBarUI() = default;

    // ==================== 关联的单位 ====================
    Unit* _unit = nullptr;

    // ==================== 血条UI组件 ====================
    cocos2d::LayerColor* _healthBarBg   = nullptr; // 血条背景（红色）
    cocos2d::LayerColor* _healthBarFill = nullptr; // 血条填充（绿色）

    // ==================== 血条状态 ====================
    int   _lastHealthValue = -1;    // 上一帧的生命值（用于检测变化）
    float _hideTimer       = 0.0f;  // 隐藏计时器：血量恢复满后，3秒内自动隐藏血条
    bool  _isVisible       = false; // 血条是否可见
    bool  _alwaysVisible   = true;  // 战斗中始终显示

    // ==================== 血条外观设置 ====================
    static constexpr float BAR_WIDTH  = 40.0f; // 血条宽度（小兵比建筑小）
    static constexpr float BAR_HEIGHT = 4.0f;  // 血条高度（小兵比建筑小）
    static constexpr float HIDE_DELAY = 3.0f;  // 血量满后多少秒隐藏血条
    static constexpr float OFFSET_Y   = 30.0f; // 血条相对于单位的高度偏移
};
