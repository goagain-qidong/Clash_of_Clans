/****************************************************************
 * Project Name:  Clash_of_Clans
 * File Name:     BuildingHealthBarUI.cpp
 * File Function: 建筑血条UI显示组件实现
 * Author:        刘相成
 * Update Date:   2025/12/14
 * License:       MIT License
 ****************************************************************/

#include "BuildingHealthBarUI.h"

USING_NS_CC;

BuildingHealthBarUI* BuildingHealthBarUI::create(BaseBuilding* building)
{
    BuildingHealthBarUI* ui = new (std::nothrow) BuildingHealthBarUI();
    if (ui && ui->init(building))
    {
        ui->autorelease();
        return ui;
    }
    CC_SAFE_DELETE(ui);
    return nullptr;
}

bool BuildingHealthBarUI::init(BaseBuilding* building)
{
    if (!Node::init() || !building)
    {
        return false;
    }

    _building = building;
    _lastHealthValue = building->getHitpoints();

    // 🆕 核心修复 1：根据建筑实际高度计算血条位置，而不是固定 20
    // 确保血条显示在建筑“头顶”上方 15 像素处
    float buildingHeight = building->getContentSize().height;
    float posY = buildingHeight + 15.0f;

    // ==================== 创建血条背景（红色 - 已损伤部分） ====================
    _healthBarBg = LayerColor::create(Color4B(80, 0, 0, 255), BAR_WIDTH, BAR_HEIGHT); // 加深背景色，对比更明显
    _healthBarBg->setPosition(Vec2(-BAR_WIDTH / 2.0f, posY));
    _healthBarBg->setAnchorPoint(Vec2(0.0f, 0.5f));
    this->addChild(_healthBarBg, 1);

    // ==================== 创建血条填充（绿色 - 剩余生命值） ====================
    _healthBarFill = LayerColor::create(Color4B(50, 200, 50, 255), BAR_WIDTH, BAR_HEIGHT);
    _healthBarFill->setPosition(Vec2(-BAR_WIDTH / 2.0f, posY));
    _healthBarFill->setAnchorPoint(Vec2(0.0f, 0.5f));
    this->addChild(_healthBarFill, 2);

    // ==================== 创建血量文字标签 ====================
    int currentHP = building->getHitpoints();
    int maxHP = building->getMaxHitpoints();
    // 稍微调整文字位置，在血条上方一点点
    _healthLabel = Label::createWithSystemFont(StringUtils::format("%d/%d", currentHP, maxHP), "Arial", 12); // 字体调小一点，免得遮挡
    _healthLabel->setPosition(Vec2(0.0f, posY + 10.0f));
    _healthLabel->setTextColor(Color4B::WHITE);
    // 给文字加个描边，防止在浅色背景下看不清
    _healthLabel->enableOutline(Color4B::BLACK, 1);
    this->addChild(_healthLabel, 3);

    // ==================== 初始状态设置 ====================
    // 默认先隐藏，除非开启了 alwaysVisible (虽然 init 时通常还没开启，但逻辑上要严谨)
    if (_alwaysVisible || currentHP < maxHP)
    {
        this->setVisible(true);
        _isVisible = true;
    }
    else
    {
        this->setVisible(false);
        _isVisible = false;
    }

    // 启用每帧更新
    this->scheduleUpdate();

    return true;
}

void BuildingHealthBarUI::update(float dt)
{
    if (!_building || isBuildingDestroyed())
    {
        this->removeFromParent();
        return;
    }

    // 🆕 核心修复 2：战斗模式强制显示检查
    // 如果处于战斗模式（_alwaysVisible为true），但当前不可见，强制显示
    // 这解决了初始化时是满血（隐藏状态），进入战斗后未能及时显示的问题
    if (_alwaysVisible && !_isVisible)
    {
        this->setVisible(true);
        _isVisible = true;
        this->setOpacity(255);
    }

    int currentHP = _building->getHitpoints();
    int maxHP = _building->getMaxHitpoints();

    // ==================== 检测生命值变化 ====================
    if (currentHP != _lastHealthValue)
    {
        _lastHealthValue = currentHP;
        _hideTimer = 0.0f; // 重置隐藏计时器

        // 显示血条（如果是第一次受伤）
        if (!_isVisible)
        {
            this->setVisible(true);
            _isVisible = true;
            this->setOpacity(0);
            this->runAction(FadeIn::create(0.2f));
        }

        // ==================== 更新血条填充宽度 ====================
        if (maxHP > 0)
        {
            float healthPercent = static_cast<float>(currentHP) / maxHP;
            // 限制百分比在 0~1 之间
            healthPercent = std::max(0.0f, std::min(1.0f, healthPercent));

            _healthBarFill->setContentSize(Size(BAR_WIDTH * healthPercent, BAR_HEIGHT));

            // 根据生命值百分比改变血条颜色
            if (healthPercent > 0.5f)
                _healthBarFill->setColor(Color3B(50, 200, 50)); // 绿
            else if (healthPercent > 0.25f)
                _healthBarFill->setColor(Color3B(255, 200, 50)); // 黄
            else
                _healthBarFill->setColor(Color3B(255, 50, 50)); // 红
        }

        // ==================== 更新血量文字 ====================
        _healthLabel->setString(StringUtils::format("%d/%d", currentHP, maxHP));
    }

    // ==================== 只有在非战斗模式下，才会在满血时自动隐藏 ====================
    if (!_alwaysVisible && currentHP >= maxHP)
    {
        _hideTimer += dt;

        if (_hideTimer >= HIDE_DELAY && _isVisible)
        {
            _isVisible = false; // 先标记为不可见，防止 update 每一帧都创建 Action
            auto fadeOut = FadeOut::create(0.3f);
            auto callback = CallFunc::create([this]() {
                this->setVisible(false);
                });
            this->runAction(Sequence::create(fadeOut, callback, nullptr));
        }
    }
}

void BuildingHealthBarUI::show()
{
    _isVisible = true;
    this->setVisible(true);
    this->setOpacity(255);
    // 重置 timer 防止刚显示就被 update 里的逻辑隐藏
    _hideTimer = 0.0f;
}

void BuildingHealthBarUI::hide()
{
    _isVisible = false;
    this->setVisible(false);
}

bool BuildingHealthBarUI::isBuildingDestroyed() const
{
    if (!_building) return true;
    // 增加安全性检查：如果建筑已经被 cleanup 或者引用计数异常，视为销毁
    if (_building->getReferenceCount() <= 0) return true;
    return _building->isDestroyed();
}