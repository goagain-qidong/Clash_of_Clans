/****************************************************************
 * Project Name:  Clash_of_Clans
 * File Name:     UnitHealthBarUI.h
 * File Function: 单位（小兵）血条UI显示组件
 * Author:        薛毓哲
 * Update Date:   2025/01/10
 * License:       MIT License
 ****************************************************************/
#ifndef UNIT_HEALTH_BAR_UI_H_
#define UNIT_HEALTH_BAR_UI_H_

#include "cocos2d.h"

class BaseUnit;

/**
 * @class UnitHealthBarUI
 * @brief 单位血条显示组件
 *
 * 功能：
 * - 显示单位的当前生命值和最大生命值
 * - 实时更新血条显示
 * - 血量满时隐藏，为零时单位消失
 * - 支持战斗状态显示
 */
class UnitHealthBarUI : public cocos2d::Node
{
public:
    /**
     * @brief 创建血条UI
     * @param unit 关联的单位对象
     * @return UnitHealthBarUI* UI指针
     */
    static UnitHealthBarUI* create(BaseUnit* unit);

    /**
     * @brief 初始化
     * @param unit 关联的单位
     * @return bool 是否成功
     */
    virtual bool init(BaseUnit* unit);

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

    /** @brief 检查关联的单位是否已死亡 */
    bool isUnitDead() const;

private:
    UnitHealthBarUI() = default;

    BaseUnit* _unit = nullptr;  ///< 关联的单位

    cocos2d::LayerColor* _healthBarBg = nullptr;    ///< 血条背景
    cocos2d::LayerColor* _healthBarFill = nullptr;  ///< 血条填充

    int _lastHealthValue = -1;    ///< 上一帧的生命值
    float _hideTimer = 0.0f;      ///< 隐藏计时器
    bool _isVisible = false;      ///< 血条是否可见
    bool _alwaysVisible = true;   ///< 是否始终可见

    static constexpr float BAR_WIDTH = 40.0f;   ///< 血条宽度
    static constexpr float BAR_HEIGHT = 4.0f;   ///< 血条高度
    static constexpr float HIDE_DELAY = 3.0f;   ///< 隐藏延迟
    static constexpr float OFFSET_Y = 30.0f;    ///< 高度偏移
};

#endif // UNIT_HEALTH_BAR_UI_H_
