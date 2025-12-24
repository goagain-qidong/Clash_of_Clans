/****************************************************************
* Project Name:  Clash_of_Clans
* File Name:     ResourceCollectionUI.cpp
* File Function: 资源收集UI类
* Author:        刘相成、薛毓哲
* Update Date:   2025/12/24
* License:       MIT License
****************************************************************/
#include "ResourceCollectionUI.h"
#include "../Buildings/ResourceBuilding.h"
#include "cocos2d.h"

USING_NS_CC;

ResourceCollectionUI* ResourceCollectionUI::create(ResourceBuilding* building)
{
    ResourceCollectionUI* ui = new (std::nothrow) ResourceCollectionUI();
    if (ui && ui->init(building))
    {
        ui->autorelease();
        return ui;
    }
    CC_SAFE_DELETE(ui);
    return nullptr;
}

bool ResourceCollectionUI::init(ResourceBuilding* building)
{
    if (!Node::init()) return false;

    _building = building;
    if (!_building) return false;

    // 创建容器节点，方便整体控制显隐和动画
    _iconContainer = Node::create();
    _iconContainer->setPosition(Vec2(0, 80)); // 初始位置在建筑上方
    _iconContainer->setVisible(false);        // 默认隐藏
    this->addChild(_iconContainer, 10);

    // 1. 加载背景板 (可选，让图标更清晰)
    auto bg = LayerColor::create(Color4B(0, 0, 0, 150), 80, 40);
    bg->setPosition(Vec2(-40, -20));
    _iconContainer->addChild(bg, 0);

    // 2. 根据类型加载资源图标
    ResourceType rType = _building->getResourceType();
    std::string iconPath = (rType == ResourceType::kGold) ? "icon/Gold.png" : "icon/Elixir.png";

    _resourceIcon = Sprite::create(iconPath);
    if (!_resourceIcon)
    {
        // 容错：如果没有图片，画个色块
        _resourceIcon = Sprite::create();
        auto draw = DrawNode::create();
        draw->drawSolidCircle(Vec2::ZERO, 20, 0, 20, (rType == kGold ? Color4F::YELLOW : Color4F::MAGENTA));
        _resourceIcon->addChild(draw);
    }
    else
    {
        _resourceIcon->setScale(0.8f); // 适当缩放
    }
    _resourceIcon->setPosition(Vec2(-20, 0)); // 图标在左
    _iconContainer->addChild(_resourceIcon, 1);

    // 3. 数量标签
    _amountLabel = Label::createWithSystemFont("0", "Arial", 18);
    _amountLabel->setAnchorPoint(Vec2(0, 0.5f));
    _amountLabel->setPosition(Vec2(5, 0)); // 文字在右
    _amountLabel->setTextColor(Color4B::WHITE);
    _amountLabel->enableOutline(Color4B::BLACK, 1);
    _iconContainer->addChild(_amountLabel, 1);

    return true;
}
void ResourceCollectionUI::updateReadyStatus(int amount)
{
    if (!_iconContainer) return;

    if (amount <= 0)
    {
        _iconContainer->setVisible(false);
        _isReadyToCollect = false;
        return;
    }

    // 更新状态
    _isReadyToCollect = true;
    _amountLabel->setString(std::to_string(amount));

    // 确保显示在正确位置，且没有正在播放的“飘走”动画
    _iconContainer->stopAllActions();
    _iconContainer->setPosition(Vec2(0, 80)); // 恢复原位
    _iconContainer->setOpacity(255);
    _iconContainer->setScale(1.0f);
    _iconContainer->setVisible(true);

    // 可以加一个轻微的呼吸动作提示可点击
    if (_iconContainer->getNumberOfRunningActions() == 0)
    {
        auto scaleUp = ScaleTo::create(0.5f, 1.1f);
        auto scaleDown = ScaleTo::create(0.5f, 1.0f);
        _iconContainer->runAction(RepeatForever::create(Sequence::create(scaleUp, scaleDown, nullptr)));
    }
}
void ResourceCollectionUI::playCollectionAnimation(int amount)
{
    if (!_iconContainer) return;

    // 停止呼吸动画
    _iconContainer->stopAllActions();

    // 更新显示的文字为 "+数量"
    _amountLabel->setString(StringUtils::format("+%d", amount));

    // 播放飘字动画：向上移动 + 淡出
    auto moveUp = MoveBy::create(1.0f, Vec2(0, 60));
    auto fadeOut = FadeOut::create(1.0f);
    auto spawn = Spawn::create(moveUp, fadeOut, nullptr);
    auto done = CallFunc::create([this]() {
        _iconContainer->setVisible(false);
        _isReadyToCollect = false;
        });

    _iconContainer->runAction(Sequence::create(spawn, done, nullptr));
}


bool ResourceCollectionUI::checkTouchInside(const cocos2d::Vec2& touchWorldPos)
{
    // 1. 基本检查：如果还没准备好收集，或者容器被隐藏了，直接返回 false
    if (!_isReadyToCollect || !_iconContainer || !_iconContainer->isVisible())
        return false;

    // 2. 获取图标容器在“世界坐标系”中的绝对位置
    // AR (Anchor Relative) 确保基于锚点计算
    Vec2 iconWorldPos = _iconContainer->convertToWorldSpaceAR(Vec2::ZERO);

    // 3. 计算点击距离
    float distance = touchWorldPos.distance(iconWorldPos);

    // 4. 判定半径（建议设大一点，比如 60-80 像素，方便手指点击）
    float clickRadius = 60.0f;

    // 调试日志：如果点不中，看这里的输出坐标差距有多大
    /*
    CCLOG("点击测试: 图标世界坐标(%.1f, %.1f) vs 触摸点(%.1f, %.1f) | 距离: %.1f",
          iconWorldPos.x, iconWorldPos.y,
          touchWorldPos.x, touchWorldPos.y,
          distance);
    */

    return distance <= clickRadius;
}

void ResourceCollectionUI::performCollection()
{
    if (!_isReadyToCollect || !_building) return;

    // 1. 获取收集前的资源状态
    auto& resMgr = ResourceManager::getInstance();
    ResourceType resType = _building->getResourceType();
    int beforeCount = resMgr.getResourceCount(resType);
    int capacity = resMgr.getResourceCapacity(resType);

    // 2. 执行建筑收集逻辑 (返回实际收集量)
    int collectedAmount = _building->collect();

    if (collectedAmount > 0)
    {
        // 3. 将资源加入全局管理器
        int actualAdded = resMgr.addResource(resType, collectedAmount);
        int afterCount = resMgr.getResourceCount(resType);

        // 4. 播放收集反馈动画
        playCollectionAnimation(collectedAmount);

        // 5. 详细日志
        std::string resName = (resType == ResourceType::kGold) ? "金币" : "圣水";
        CCLOG("💰 收集完成: %s", _building->getDisplayName().c_str());
        CCLOG("   资源类型: %s", resName.c_str());
        CCLOG("   收集前: %d / %d", beforeCount, capacity);
        CCLOG("   尝试增加: %d", collectedAmount);
        CCLOG("   实际增加: %d", actualAdded);
        CCLOG("   收集后: %d / %d", afterCount, capacity);
        
        if (actualAdded < collectedAmount)
        {
            CCLOG("⚠️ 资源仓库已满！溢出: %d", collectedAmount - actualAdded);
        }
    }
    else
    {
        CCLOG("⚠️ 收集量为0，建筑: %s", _building->getDisplayName().c_str());
    }
}