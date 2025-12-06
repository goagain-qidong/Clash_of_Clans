/****************************************************************
 * Project Name:  Clash_of_Clans
 * File Name:     Building.h
 * File Function:
 * Author:        刘相成
 * Update Date:   2025/12/2
 * License:       MIT License
 ****************************************************************/
#include "HeroManager.h"
#include "cocos2d.h"

USING_NS_CC;
using namespace ui;

HeroManager* HeroManager::create()
{
    HeroManager* manager = new (std::nothrow) HeroManager();
    if (manager && manager->init())
    {
        manager->autorelease();
        return manager;
    }
    CC_SAFE_DELETE(manager);
    return nullptr;
}

bool HeroManager::init()
{
    if (!Node::init())
    {
        return false;
    }

    _isHeroListVisible = false;
    _selectedHero = nullptr;

    // 加载精灵帧缓存
    SpriteFrameCache::getInstance()->addSpriteFramesWithFile("units/archer/archer.plist", "units/archer/archer.png");

    // 加载英雄数据
    loadHeroData();

    return true;
}

void HeroManager::loadHeroData()
{
    // 可用的英雄 - 现在只有一个archer英雄
    _availableHeroes = {
        "archer"  // 只显示一个archer英雄选项

    };

    // 加载精灵帧缓存
    SpriteFrameCache::getInstance()->addSpriteFramesWithFile("units/archer/archer.plist", "units/archer/archer.png");
}

void HeroManager::setupHeroUI(cocos2d::Node* parent, const cocos2d::Size& visibleSize)
{
    _visibleSize = visibleSize;

    // 英雄选择按钮 - 增大尺寸
    _heroButton = Button::create();
    _heroButton->setTitleText("Hero");
    _heroButton->setTitleFontSize(24);
    _heroButton->setContentSize(Size(120, 60));
    _heroButton->setPosition(Vec2(visibleSize.width - 80, visibleSize.height - 120));
    _heroButton->addClickEventListener(CC_CALLBACK_1(HeroManager::onHeroButtonClicked, this));
    parent->addChild(_heroButton, 10);

    // 创建英雄列表
    createHeroList();
}

void HeroManager::createHeroList()
{
    _heroList = ListView::create();
    _heroList->setContentSize(Size(200, 100)); // 减小高度，因为只有一个英雄
    _heroList->setPosition(Vec2(_visibleSize.width - 210, _visibleSize.height - 220));
    _heroList->setBackGroundColor(Color3B(80, 80, 80));
    _heroList->setBackGroundColorType(ui::Layout::BackGroundColorType::SOLID);
    _heroList->setOpacity(200);
    _heroList->setVisible(false);
    _heroList->setScrollBarEnabled(true);
    _heroList->setBounceEnabled(true);

    for (const auto& heroName : _availableHeroes) {
        auto item = Layout::create();
        item->setContentSize(Size(180, 60));
        item->setTouchEnabled(true);

        // 创建英雄图标 - 使用第一帧作为图标，增大尺寸
        auto heroSprite = Sprite::createWithSpriteFrameName("archer1.0.png");
        if (heroSprite) {
            heroSprite->setScale(1.0f); // 增大预览图标
            heroSprite->setPosition(Vec2(40, 30));
            heroSprite->setName("sprite");
            item->addChild(heroSprite);
        }

        // 英雄名称显示
        auto label = Label::createWithSystemFont(heroName, "Arial", 16);
        label->setPosition(Vec2(120, 30));
        label->setTextColor(Color4B::WHITE);
        label->setName("label");
        item->addChild(label);

        // 添加背景色，便于区分项目
        auto itemBg = LayerColor::create(Color4B(60, 60, 60, 255));
        itemBg->setContentSize(Size(180, 60));
        itemBg->setPosition(Vec2::ZERO);
        item->addChild(itemBg, -1);

        // 添加点击事件
        item->addClickEventListener([this, heroName](Ref* sender) {
            this->onHeroItemClicked(sender);
            });

        _heroList->pushBackCustomItem(item);
    }

    this->getParent()->addChild(_heroList, 20);
}

void HeroManager::onHeroButtonClicked(cocos2d::Ref* sender)
{
    if (_isHeroListVisible) {
        hideHeroList();
    }
    else {
        showHeroList();
    }
}

void HeroManager::showHeroList()
{
    _isHeroListVisible = true;
    _heroList->setVisible(true);
}

void HeroManager::hideHeroList()
{
    _isHeroListVisible = false;
    _heroList->setVisible(false);
}

void HeroManager::onHeroItemClicked(cocos2d::Ref* sender)
{
    auto item = static_cast<Layout*>(sender);
    auto label = static_cast<Label*>(item->getChildByName("label"));
    std::string selectedHeroName = label->getString();

    CCLOG("Selected hero: %s", selectedHeroName.c_str());

    selectHero(selectedHeroName);
    hideHeroList();
}

void HeroManager::selectHero(const std::string& heroName)
{
    _selectedHeroName = heroName;

    // 移除旧的预览
    if (_selectedHero) {
        _selectedHero->removeFromParent();
        _selectedHero = nullptr;
    }

    // 创建选中英雄的预览，增大尺寸
    _selectedHero = Hero::create("archer1.0.png");
    if (_selectedHero) {
        _selectedHero->setScale(0.8f); // 增大预览尺寸
        _selectedHero->setPosition(Vec2(_visibleSize.width - 120, _visibleSize.height - 180));
        this->getParent()->addChild(_selectedHero, 15);

        // 显示提示
        auto tip = Label::createWithSystemFont("Click on map to place " + heroName, "Arial", 16);
        tip->setPosition(Vec2(_visibleSize.width - 120, _visibleSize.height - 220));
        tip->setTextColor(Color4B::YELLOW);
        tip->setName("heroTip");
        this->getParent()->addChild(tip, 15);
    }
}

void HeroManager::placeHero(const cocos2d::Vec2& worldPosition, cocos2d::Node* mapNode)
{
    if (_selectedHeroName.empty() || !mapNode) {
        CCLOG("No hero selected or no map node");
        return;
    }

    // 移除预览的英雄
    if (_selectedHero) {
        _selectedHero->removeFromParent();
        _selectedHero = nullptr;
    }

    // 移除提示
    auto parent = this->getParent();
    if (parent) {
        auto tip = parent->getChildByName("heroTip");
        if (tip) {
            tip->removeFromParent();
        }
    }

    // 创建新的英雄实例并添加到地图中，增大基础尺寸
    auto hero = Hero::create("archer1.0.png");
    if (hero) {
        // 将世界坐标转换为地图本地坐标
        Vec2 localPos = mapNode->convertToNodeSpace(worldPosition);

        hero->setPosition(localPos);
        // 获取地图的当前缩放比例
        float mapScale = mapNode->getScale();
        hero->updateScale(mapScale);  // 使用地图缩放来更新英雄尺寸

        mapNode->addChild(hero, 2); // 添加到地图中，层级高于地图

        _placedHeroes.pushBack(hero);

        CCLOG("Hero placed at: %.1f, %.1f", localPos.x, localPos.y);

        // 清除选中的英雄名称，表示没有待放置的英雄
        _selectedHeroName.clear();
        deselectAllHeroes(); // 先取消所有选择
        hero->setSelected(true); // 选中新放置的英雄
    }
}

void HeroManager::handleHeroTouch(const cocos2d::Vec2& worldPosition, cocos2d::Node* mapNode, bool isPlacingNewHero)
{
    if (!mapNode) return;

    // 如果有待放置的新英雄，优先放置
    if (!_selectedHeroName.empty() && isPlacingNewHero) {
        placeHero(worldPosition, mapNode);
        return;
    }

    // 检查是否点击了已放置的英雄
    bool clickedOnHero = false;
    for (auto& hero : _placedHeroes) {
        if (hero && hero->containsTouch(worldPosition, mapNode)) {
            // 切换选中状态
            bool wasSelected = hero->isSelected();

            // 先取消所有英雄的选中状态
            deselectAllHeroes();

            // 如果这个英雄之前没有被选中，现在选中它
            if (!wasSelected) {
                hero->setSelected(true);
                CCLOG("Hero selected");
            }
            else {
                CCLOG("Hero deselected");
            }
            clickedOnHero = true;
            break;
        }
    }

    // 如果没有点击英雄且没有待放置的英雄，检查是否有选中的英雄需要移动
    if (!clickedOnHero && _selectedHeroName.empty()) { // 修改这里：确保没有待放置的英雄
        for (auto& hero : _placedHeroes) {
            if (hero && hero->isSelected()) {
                hero->moveTo(worldPosition, mapNode);
                CCLOG("Moving selected hero to new position");
                break;
            }
        }
    }
}

void HeroManager::deselectAllHeroes()
{
    for (auto& hero : _placedHeroes) {
        if (hero) {
            hero->setSelected(false);
        }
    }
}

void HeroManager::moveSelectedHero(const cocos2d::Vec2& worldPosition, cocos2d::Node* mapNode)
{
    // 这个方法现在被 handleHeroTouch 替代
    // 保留用于兼容性
    if (_placedHeroes.empty() || !mapNode) return;

    // 移动选中的英雄
    for (auto& hero : _placedHeroes) {
        if (hero && hero->isSelected()) {
            hero->moveTo(worldPosition, mapNode);
            return;
        }
    }
}

void HeroManager::updateHeroesScale(float mapScale)
{
    // 更新所有已放置英雄的缩放
    for (auto& hero : _placedHeroes) {
        if (hero) {
            hero->updateScale(mapScale);
        }
    }
}

void HeroManager::onMapSwitched(cocos2d::Node* newMapNode)
{
    if (!newMapNode) return;

    // 保存当前英雄的位置信息
    Vector<Hero*> oldHeroes = _placedHeroes;
    _placedHeroes.clear();
    float mapScale = newMapNode->getScale();
    // 将英雄转移到新地图中
    for (auto& hero : oldHeroes) {
        if (hero) {
            auto newHero = Hero::create("archer1.0.png");
            if (newHero) {
                newHero->setPosition(hero->getPosition());
                newHero->updateScale(mapScale);  // 使用新地图的缩放
                newMapNode->addChild(newHero, 2);
                _placedHeroes.pushBack(newHero);

                // 保持选中状态
                if (hero->isSelected()) {
                    newHero->setSelected(true);
                }
            }
        }
    }
}