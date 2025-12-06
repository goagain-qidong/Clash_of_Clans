#ifndef UNIT_H
#define UNIT_H

#include <map>
#include <string>

#include "cocos2d.h"

// 兵种类型
enum class UnitType
{
    Barbarian,
    Archer,
    // ... 以后其他兵种加这里
};

// 动作类型
enum class UnitAction
{
    Run,//跑步
    Idle,//站着
    Attack//攻击
};

// 方向 (8方向)
enum class UnitDirection
{
    Up,//朝上
    UpRight,//右上
    Right,//右
    DownRight,//右下
    Down,//下
    DownLeft,//左下
    Left,//左
    UpLeft//左上
};

class Unit : public cocos2d::Node
{
public:
    // 唯一的创建接口：传入类型（比如野蛮人）
    static Unit* create(UnitType type);
    virtual bool init(UnitType type);

    // 播放动画：只需告诉干什么、朝哪边
    void PlayAnimation(UnitAction action, UnitDirection dir);

private:
    cocos2d::Sprite* sprite_ = nullptr;

    // 记录当前兵种的动画配置 [动作名 -> 动画对象]
    std::map<std::string, cocos2d::Animation*> anim_cache_;

    // 加载配置（把你的数字填在这里面）
    void LoadConfig(UnitType type);

    // 辅助工具：根据起止帧创建动画
    void AddAnim(const std::string& key, int start, int end, float delay);
};

#endif