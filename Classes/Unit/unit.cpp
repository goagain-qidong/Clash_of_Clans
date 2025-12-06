// 2453619 薛毓哲

#include "unit.h"

USING_NS_CC;

Unit* Unit::create(UnitType type)
{
    Unit* unit = new (std::nothrow) Unit();
    if (unit && unit->init(type))
    {
        unit->autorelease();
        return unit;
    }
    CC_SAFE_DELETE(unit);
    return nullptr;
}

bool Unit::init(UnitType type)
{
    if (!Node::init())
        return false;

    // 1. 加载配置 (路径已修正)
    LoadConfig(type);

    // 2. 创建 Sprite
    // 使用 plist 中存在的图片名来创建，确保不是透明的
    sprite_ = Sprite::createWithSpriteFrameName("barbarian25.0.png");

    if (sprite_ == nullptr)
    {
        // 如果这里打印了，说明 LoadConfig 里的路径还是不对
        cocos2d::log("Error: 找不到图片帧，请检查 LoadConfig 里的路径是否正确");
        sprite_ = Sprite::create();  // 防崩溃
    }

    if (sprite_)
    {
        // 【关键修复】这里只调用一次 addChild
        this->addChild(sprite_);

        // 默认播放一个待机动作防止空白
        PlayAnimation(UnitAction::Idle, UnitDirection::Right);

        // 脚底对齐
        sprite_->setAnchorPoint(Vec2(0.5, 0));
    }

    return true;
}

// =========================================================
// 核心配置区
// =========================================================
void Unit::LoadConfig(UnitType type)
{
    if (type == UnitType::Barbarian)
    {
        // 【关键修复】路径修正为 units/barbarian/barbarian.plist
        // 注意大小写必须和你的文件夹完全一致
        SpriteFrameCache::getInstance()->addSpriteFramesWithFile("units/barbarian/barbarian.plist");

        // 2. 填入你的数字
        // 格式：AddAnim("内部代号", 开始号, 结束号, 速度);

        // --- 跑步 ---
        AddAnim("run_down_right", 1, 8, 0.1f);
        AddAnim("run_right", 9, 16, 0.1f);
        AddAnim("run_up_right", 17, 24, 0.1f);

        // --- 站立 ---
        AddAnim("idle_down_right", 25, 25, 1.0f);
        AddAnim("idle_right", 26, 26, 1.0f);
        AddAnim("idle_up_right", 27, 27, 1.0f);

        // --- 攻击 ---
        AddAnim("attack_down_right", 31, 38, 0.1f);
        AddAnim("attack_right", 39, 46, 0.1f);
        AddAnim("attack_up_right", 47, 54, 0.1f);
    }
    else if (type == UnitType::Archer)
    {
        // 以后这里填弓箭手的 plist 和数字
    }
}

void Unit::AddAnim(const std::string& key, int start, int end, float delay)
{
    Vector<SpriteFrame*> frames;
    for (int i = start; i <= end; ++i)
    {
        // 拼接文件名 barbarianX.0.png
        // 注意：如果 plist 里是 barbarian_01.png 这种格式，这里要对应修改
        std::string name = StringUtils::format("barbarian%d.0.png", i);
        auto frame = SpriteFrameCache::getInstance()->getSpriteFrameByName(name);
        if (frame)
            frames.pushBack(frame);
    }
    if (!frames.empty())
    {
        auto anim = Animation::createWithSpriteFrames(frames, delay);
        anim->retain();  // 防止被释放
        anim_cache_[key] = anim;
    }
}

// =========================================================
// 播放逻辑：处理 8 方向 -> 3 素材的翻转
// =========================================================
void Unit::PlayAnimation(UnitAction action, UnitDirection dir)
{
    std::string anim_key = "";
    bool flip_x = false;

    // 1. 简单的方向映射表
    // 只有 Right, UpRight, DownRight 是真的，其他都是借用的
    switch (dir)
    {
        case UnitDirection::Right:
            anim_key = "right";
            flip_x = false;
            break;
        case UnitDirection::UpRight:
            anim_key = "up_right";
            flip_x = false;
            break;
        case UnitDirection::DownRight:
            anim_key = "down_right";
            flip_x = false;
            break;

        // 左侧全是翻转
        case UnitDirection::Left:
            anim_key = "right";
            flip_x = true;
            break;
        case UnitDirection::UpLeft:
            anim_key = "up_right";
            flip_x = true;
            break;
        case UnitDirection::DownLeft:
            anim_key = "down_right";
            flip_x = true;
            break;

        // 上下暂时借用
        case UnitDirection::Up:
            anim_key = "up_right";
            flip_x = false;
            break;
        case UnitDirection::Down:
            anim_key = "down_right";
            flip_x = false;
            break;
    }

    // 2. 拼接动作前缀
    std::string prefix = "";
    if (action == UnitAction::Run)
        prefix = "run_";
    if (action == UnitAction::Idle)
        prefix = "idle_";
    if (action == UnitAction::Attack)
        prefix = "attack_";

    std::string final_key = prefix + anim_key;

    // 3. 播放
    if (anim_cache_.count(final_key))
    {
        sprite_->stopAllActions();
        sprite_->setFlippedX(flip_x);
        sprite_->runAction(RepeatForever::create(Animate::create(anim_cache_[final_key])));
    }
}