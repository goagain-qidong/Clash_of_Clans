/****************************************************************
 * Project Name:  Clash_of_Clans
 * File Name:     UnitHealthBarUI.cpp
 * File Function: 单位（小兵）血条UI显示组件实现
 * Author:        薛毓哲
 * Update Date:   2025/01/10
 * License:       MIT License
 ****************************************************************/

#include "UnitHealthBarUI.h"

#include "Unit/BaseUnit.h"

USING_NS_CC;

UnitHealthBarUI* UnitHealthBarUI::create(BaseUnit* unit)
{
    UnitHealthBarUI* ui = new (std::nothrow) UnitHealthBarUI();
    if (ui && ui->init(unit))
    {
        ui->autorelease();
        return ui;
    }
    CC_SAFE_DELETE(ui);
    return nullptr;
}

bool UnitHealthBarUI::init(BaseUnit* unit)
{
    if (!Node::init() || !unit)
    {
        return false;
    }

    _unit            = unit;
    _lastHealthValue = unit->getCurrentHP();

    // ==================== 创建血条背景（深灰色 - 已损伤部分） ====================
    _healthBarBg = LayerColor::create(Color4B(80, 80, 80, 255), BAR_WIDTH, BAR_HEIGHT);
    _healthBarBg->setPosition(Vec2(-BAR_WIDTH / 2.0f, OFFSET_Y));
    _healthBarBg->setAnchorPoint(Vec2(0.0f, 0.5f));
    this->addChild(_healthBarBg, 1);

    // ==================== 创建血条填充（绿色 - 剩余生命值） ====================
    _healthBarFill = LayerColor::create(Color4B(50, 200, 50, 255), BAR_WIDTH, BAR_HEIGHT);
    _healthBarFill->setPosition(Vec2(-BAR_WIDTH / 2.0f, OFFSET_Y));
    _healthBarFill->setAnchorPoint(Vec2(0.0f, 0.5f));
    this->addChild(_healthBarFill, 2);

    // ==================== 初始状态设置 ====================
    int currentHP = unit->getCurrentHP();
    int maxHP     = unit->getMaxHP();

    // 战斗中始终显示血条
    this->setVisible(true);
    _isVisible = true;

    // 启用每帧更新
    this->scheduleUpdate();

    return true;
}

void UnitHealthBarUI::update(float dt)
{
    // 🔴 修复：更安全的空指针和死亡检查
    // 先检查 _unit 是否为空，然后检查它是否已死亡
    if (_unit == nullptr)
    {
        // 单位指针已无效，停止更新并移除自己
        this->unscheduleUpdate();
        this->removeFromParent();
        return;
    }
    
    // 检查单位是否死亡
    if (_unit->isDead())
    {
        // 单位死亡，清除引用并移除自己
        _unit = nullptr;
        this->unscheduleUpdate();
        this->removeFromParent();
        return;
    }

    int currentHP = _unit->getCurrentHP();
    int maxHP     = _unit->getMaxHP();

    // ==================== 检测生命值变化 ====================
    if (currentHP != _lastHealthValue)
    {
        _lastHealthValue = currentHP;
        _hideTimer       = 0.0f; // 重置隐藏计时器

        // 显示血条
        if (!_isVisible)
        {
            this->setVisible(true);
            _isVisible = true;

            // 播放血条出现动画
            this->setOpacity(0);
            auto fadeIn = FadeIn::create(0.2f);
            this->runAction(fadeIn);
        }

        // ==================== 更新血条填充宽度 ====================
        if (maxHP > 0)
        {
            float healthPercent = static_cast<float>(currentHP) / maxHP;
            _healthBarFill->setContentSize(Size(BAR_WIDTH * healthPercent, BAR_HEIGHT));

            // 根据生命值百分比改变血条颜色
            if (healthPercent > 0.5f)
            {
                // 绿色：血量充足
                _healthBarFill->setColor(Color3B(50, 200, 50));
            }
            else if (healthPercent > 0.25f)
            {
                // 黄色：血量不足
                _healthBarFill->setColor(Color3B(255, 200, 50));
            }
            else
            {
                // 红色：血量严重不足
                _healthBarFill->setColor(Color3B(255, 50, 50));
            }
        }
    }

    // ==================== 血量恢复满后自动隐藏（不是战斗状态时） ====================
    if (!_alwaysVisible && currentHP >= maxHP)
    {
        _hideTimer += dt;

        if (_hideTimer >= HIDE_DELAY && _isVisible)
        {
            // 播放血条消失动画
            auto fadeOut  = FadeOut::create(0.3f);
            auto callback = CallFunc::create([this]() {
                this->setVisible(false);
                _isVisible = false;
            });
            this->runAction(Sequence::create(fadeOut, callback, nullptr));
        }
    }
}

void UnitHealthBarUI::show()
{
    if (!_isVisible)
    {
        this->setVisible(true);
        _isVisible = true;
        this->setOpacity(255);
    }
}

void UnitHealthBarUI::hide()
{
    if (_isVisible)
    {
        this->setVisible(false);
        _isVisible = false;
    }
}

bool UnitHealthBarUI::isUnitDead() const
{
    // 🔴 修复：只检查空指针，不在空指针上调用方法
    if (_unit == nullptr)
    {
        return true;
    }

    return _unit->isDead();
}
