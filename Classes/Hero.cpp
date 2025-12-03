/****************************************************************
 * Project Name:  Clash_of_Clans
 * File Name:     Building.h
 * File Function:
 * Author:        刘相成
 * Update Date:   2025/12/2
 * License:       MIT License
 ****************************************************************/
#include "Hero.h"
#include "cocos2d.h"
#include "GridMap.h"

USING_NS_CC;

Hero* Hero::create(const std::string& frameName)
{
    Hero* hero = new (std::nothrow) Hero();
    if (hero && hero->init(frameName))
    {
        hero->autorelease();
        return hero;
    }
    CC_SAFE_DELETE(hero);
    return nullptr;
}

bool Hero::init(const std::string& frameName)
{
    if (!Sprite::initWithSpriteFrameName(frameName))
    {
        return false;
    }

    _heroName = "archer"; // 统一英雄名称
    _isSelected = false;
    _baseScale = 0.5f; // 增大基础尺寸
    _isMoving = false;

    // 设置名称
    this->setName("Hero");

    // 设置初始缩放
    this->setScale(_baseScale);

    return true;
}

void Hero::moveTo(const cocos2d::Vec2& worldPosition, cocos2d::Node* mapNode)
{
    if (!mapNode || !_isSelected) return; // 只有选中的英雄才能移动

    Vec2 localPos = mapNode->convertToNodeSpace(worldPosition);

    this->stopAllActions();

    float distance = this->getPosition().getDistance(localPos);
    float duration = distance / 100.0f;

    playWalkAnimation();
    _isMoving = true;

    auto moveTo = MoveTo::create(duration, localPos);
    auto callback = CallFunc::create([this]() {
        this->stopWalkAnimation();
        _isMoving = false;
    });

    auto sequence = Sequence::create(moveTo, callback, nullptr);
    this->runAction(sequence);

    CCLOG("Hero %s moving to: %.1f, %.1f, duration: %.2f", _heroName.c_str(), localPos.x, localPos.y, duration);
}

void Hero::moveAlongPath(const std::vector<cocos2d::Vec2>& gridPath, cocos2d::Node* mapNode, GridMap* grid)
{
    if (!mapNode || !grid || gridPath.empty()) return;

    this->stopAllActions();
    playWalkAnimation();
    _isMoving = true;

    Vector<FiniteTimeAction*> moves;
    Vec2 prev = this->getPosition();
    for (auto gp : gridPath) {
        Vec2 p = grid->getPositionFromGrid(gp);
        Vec2 local = p; // grid returns local position to grid parent; grid is child of mapSprite
        float distance = prev.getDistance(local);
        float duration = distance / 120.0f; // slightly faster for path following
        moves.pushBack(MoveTo::create(duration, local));
        prev = local;
    }

    auto finish = CallFunc::create([this]() {
        this->stopWalkAnimation();
        _isMoving = false;
    });

    auto seq = Sequence::create(moves);
    this->runAction(Sequence::create(seq, finish, nullptr));
}

void Hero::updateScale(float mapScale)
{
    this->setScale(_baseScale * mapScale);
}

void Hero::setSelected(bool selected)
{
    if (_isSelected == selected) return;

    _isSelected = selected;

    if (selected) {
        // 添加选中效果 - 黄色圆圈，增大尺寸
        auto glow = DrawNode::create();
        glow->drawCircle(Vec2::ZERO, 25, 0, 20, false, 1.0f, 1.0f, Color4F::YELLOW);
        glow->setName("selectionGlow");
        this->addChild(glow, -1);

        // 添加脉冲动画
        float currentScale = this->getScale();
        auto scaleUp = ScaleTo::create(0.5f, currentScale * 1.2f);
        auto scaleDown = ScaleTo::create(0.5f, currentScale);
        auto pulse = RepeatForever::create(Sequence::create(scaleUp, scaleDown, nullptr));
        pulse->setTag(999);
        this->runAction(pulse);

        CCLOG("Hero selected");

    }
    else {
        // 移除选中效果
        float currentScale = this->getScale();
        this->removeChildByName("selectionGlow");
        this->stopActionByTag(999);
        this->setScale(currentScale);

        CCLOG("Hero deselected - keeping scale: %.2f", this->getScale());
    }
}

void Hero::playWalkAnimation()
{
    // 如果已经有动画在运行，不重复创建
    if (this->getActionByTag(666)) return;

    // 创建行走动画帧序列
    Vector<SpriteFrame*> frames;
    for (int i = 1; i <= 52; i++) {
        std::string frameName = StringUtils::format("archer%d.0.png", i);
        auto frame = SpriteFrameCache::getInstance()->getSpriteFrameByName(frameName);
        if (frame) {
            frames.pushBack(frame);
        }
    }

    if (frames.size() > 0) {
        auto animation = Animation::createWithSpriteFrames(frames, 0.05f); // 每帧0.05秒
        auto animate = Animate::create(animation);
        auto repeat = RepeatForever::create(animate);
        repeat->setTag(666); // 设置tag以便停止
        this->runAction(repeat);
    }

    // 添加上下浮动效果
    auto floatUp = MoveBy::create(0.5f, Vec2(0, 5));
    auto floatDown = MoveBy::create(0.5f, Vec2(0, -5));
    auto floatSequence = Sequence::create(floatUp, floatDown, nullptr);
    auto floatRepeat = RepeatForever::create(floatSequence);
    floatRepeat->setTag(777);
    this->runAction(floatRepeat);
}

void Hero::stopWalkAnimation()
{
    this->stopActionByTag(666); // 停止动画
    this->stopActionByTag(777); // 停止浮动效果
    this->setRotation(0); // 恢复原始角度

    // 恢复到第一帧
    this->setSpriteFrame("archer1.0.png");
}

bool Hero::containsTouch(const cocos2d::Vec2& worldPosition, cocos2d::Node* mapNode)
{
    if (!mapNode) return false;

    // 将世界坐标转换为地图本地坐标
    Vec2 localPos = mapNode->convertToNodeSpace(worldPosition);

    // 获取英雄在地图坐标中的边界框
    Rect heroRect = this->getBoundingBox();
    Vec2 heroPos = this->getPosition();

    // 计算触摸点相对于英雄的位置
    Vec2 touchInHero = localPos - heroPos;

    // 英雄的碰撞检测半径，增大检测范围
    float radius = 40.0f * this->getScale();
    bool contains = touchInHero.length() <= radius;

    if (contains) {
        CCLOG("Hero touch detected at distance: %.1f, radius: %.1f", touchInHero.length(), radius);
    }

    return contains;
}