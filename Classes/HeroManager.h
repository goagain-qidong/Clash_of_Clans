#pragma once
#ifndef __HERO_MANAGER_H__
#define __HERO_MANAGER_H__

#include "cocos2d.h"
#include "ui/CocosGUI.h"
#include "Hero.h"

class HeroManager : public cocos2d::Node
{
public:
    static HeroManager* create();
    virtual bool init() override;

    // UI 设置
    void setupHeroUI(cocos2d::Node* parent, const cocos2d::Size& visibleSize);
    void showHeroList();
    void hideHeroList();
    bool isHeroListVisible() const { return _isHeroListVisible; }

    // 英雄操作
    void selectHero(const std::string& heroName);
    void placeHero(const cocos2d::Vec2& worldPosition, cocos2d::Node* mapNode);
    void moveSelectedHero(const cocos2d::Vec2& worldPosition, cocos2d::Node* mapNode);
    void updateHeroesScale(float mapScale);

    // 获取选中的英雄
    Hero* getSelectedHero() const { return _selectedHero; }
    const std::string& getSelectedHeroName() const { return _selectedHeroName; }

    // 地图切换时处理
    void onMapSwitched(cocos2d::Node* newMapNode);

    // 新增：处理英雄点击
    // 在 HeroManager.h 的 public 部分修改方法声明
    void handleHeroTouch(const cocos2d::Vec2& worldPosition, cocos2d::Node* mapNode, bool isPlacingNewHero = false);
    void deselectAllHeroes();

private:
    void createHeroList();
    void onHeroButtonClicked(cocos2d::Ref* sender);
    void onHeroItemClicked(cocos2d::Ref* sender);
    void loadHeroData();

    cocos2d::ui::Button* _heroButton;
    cocos2d::ui::ListView* _heroList;
    bool _isHeroListVisible;

    std::string _selectedHeroName;
    Hero* _selectedHero;
    cocos2d::Vector<Hero*> _placedHeroes;

    std::vector<std::string> _availableHeroes;
    cocos2d::Size _visibleSize;
};

#endif // __HERO_MANAGER_H__