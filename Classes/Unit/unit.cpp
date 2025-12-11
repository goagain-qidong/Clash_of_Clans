#include "unit.h"

USING_NS_CC; // 使用 Cocos2d 命名空间，免去每次写 cocos2d::

// --------------------------------------------------------------------------
// 标准的 Cocos2d-x "create" 模式实现
// 作用：new 一个对象 -> init 初始化 -> autorelease 加入自动内存管理池
// --------------------------------------------------------------------------
Unit* Unit::create(UnitType type)
{
    Unit* unit = new (std::nothrow) Unit();
    if (unit && unit->init(type))
    {
        unit->autorelease(); // 关键：交出所有权，防止忘记 delete 导致内存泄漏
        return unit;
    }
    CC_SAFE_DELETE(unit); // 如果初始化失败，安全删除
    return nullptr;
}

// --------------------------------------------------------------------------
// 初始化函数：相当于构造函数的实际逻辑
// --------------------------------------------------------------------------
bool Unit::init(UnitType type)
{
    if (!Node::init()) // 必须先初始化父类
        return false;

    // ==================== 步骤1：保存兵种类型 ====================
    type_ = type;

    // ==================== 步骤2：根据兵种设置差异化移动速度 ====================
    switch (type)
    {
    case UnitType::kGoblin:
        move_speed_ = 150.0f;  // 哥布林：快速，血少但机动性强
        break;
    case UnitType::kGiant:
        move_speed_ = 60.0f;   // 巨人：慢速，血厚但移动慢
        break;
    case UnitType::kWallBreaker:
        move_speed_ = 120.0f;  // 炸弹人：较快，需要冲向目标
        break;
    case UnitType::kBarbarian:
    case UnitType::kArcher:
    default:
        move_speed_ = 100.0f;  // 野蛮人/弓箭手：标准速度
        break;
    }

    // ==================== 步骤3：初始化战斗属性 ⭐ 新增 ====================
    unit_level_ = 1; // 默认1级
    switch (type)
    {
    case UnitType::kBarbarian:
        combat_stats_ = UnitConfig::getBarbarian(unit_level_);
        break;
    case UnitType::kArcher:
        combat_stats_ = UnitConfig::getArcher(unit_level_);
        break;
    case UnitType::kGiant:
        combat_stats_ = UnitConfig::getGiant(unit_level_);
        break;
    case UnitType::kGoblin:
        combat_stats_ = UnitConfig::getGoblin(unit_level_);
        break;
    case UnitType::kWallBreaker:
        combat_stats_ = UnitConfig::getWallBreaker(unit_level_);
        break;
    default:
        combat_stats_ = UnitConfig::getBarbarian(unit_level_);
        break;
    }

    // 1. 根据类型加载资源 (比如加载野蛮人的 plist 文件)
    LoadConfig(type);

    // 2. 创建初始 Sprite (精灵) - 根据类型选择不同的初始帧
    // 注意：必须确保 plist 已经被加载，且图片名正确，否则这里会崩溃或显示为空
    std::string initialFrame;
    if (type == UnitType::kBarbarian)
    {
        initialFrame = "barbarian25.0.png";  // 野蛮人待机帧
    }
    else if (type == UnitType::kArcher)
    {
        initialFrame = "archer27.0.png";  // 弓箭手待机帧（右方向中间帧）
    }
    else if (type == UnitType::kGiant)
    {
        initialFrame = "giant38.0.png";  // 巨人待机帧（右方向）
    }
    else if (type == UnitType::kGoblin)
    {
        initialFrame = "goblin26.0.png";  // 哥布林待机帧（右方向）
    }
    else if (type == UnitType::kWallBreaker)
    {
        initialFrame = "wall_breaker21.0.png";  // 炸弹人待机帧（右方向）
    }
    else
    {
        CCLOG("ERROR: Unknown unit type!");
        return false;
    }

    sprite_ = Sprite::createWithSpriteFrameName(initialFrame);



    if (sprite_)
    {

        this->addChild(sprite_); // 把精灵作为子节点添加到 Unit 节点上

        // 3. 设置初始状态：待机，朝右
        PlayAnimation(UnitAction::kIdle, UnitDirection::kRight);

        // 4. 设置锚点在脚底 (0.5, 0.0)
        // 这样设置坐标 (x, y) 时，(x, y) 就在人物的脚下，方便对齐地图格子
        sprite_->setAnchorPoint(Vec2(0.5f, 0.0f));
    }
    else
    {
        cocos2d::log("Error: Failed to create sprite. Check plist path.");
    }

    // 5. 【关键】开启 update 调度
    // 这行代码告诉引擎：“请每一帧调用一次我的 update(float dt) 函数”
    this->scheduleUpdate();

    return true;
}

// --------------------------------------------------------------------------
// 析构函数：清理手动管理的内存
// --------------------------------------------------------------------------
Unit::~Unit()
{
    // 因为在 AddAnim 里我们对其调用了 retain() (引用计数+1)
    // 所以这里必须调用 release() (引用计数-1)，否则这些动画对象永远不会从内存删除
    for (auto& pair : anim_cache_)
    {
        pair.second->release();
    }
    anim_cache_.clear();
}

// --------------------------------------------------------------------------
// 加载配置：这是“硬编码”数据的地方，定义了每个动作对应哪几张图
// --------------------------------------------------------------------------
void Unit::LoadConfig(UnitType type)
{
    if (type == UnitType::kBarbarian)
    {
        // 加载图集信息到缓存
        SpriteFrameCache::getInstance()->addSpriteFramesWithFile("units/barbarian/barbarian.plist");

        // ========== 跑步动画 ==========
        AddAnim("barbarian", "run_down_right", 1, 8, 0.1f);   // 1~8 downright run
        AddAnim("barbarian", "run_right", 9, 16, 0.1f);       // 9~16 right run
        AddAnim("barbarian", "run_up_right", 17, 24, 0.1f);   // 17~24 upright run

        // ========== 待机动画 ==========
        AddAnim("barbarian", "idle_down_right", 25, 25, 1.0f); // 25 downright stand
        AddAnim("barbarian", "idle_right", 26, 26, 1.0f);      // 26 right stand
        AddAnim("barbarian", "idle_up_right", 27, 27, 1.0f);   // 27 upright stand

        // ========== 攻击动画 1 ==========
        AddAnim("barbarian", "attack_down_right", 31, 38, 0.1f); // 31~38 downright attack 1
        AddAnim("barbarian", "attack_right", 39, 46, 0.1f);      // 39~46 right attack 1
        AddAnim("barbarian", "attack_up_right", 47, 54, 0.1f);   // 47~54 upright attack 1

        // ========== 攻击动画 2 ==========
        AddAnim("barbarian", "attack2_down_right", 55, 65, 0.09f); // 55~65 downright attack 2
        AddAnim("barbarian", "attack2_right", 66, 76, 0.09f);      // 66~76 right attack 2
        AddAnim("barbarian", "attack2_up_right", 77, 87, 0.09f);   // 77~87 upright attack 2

        // ========== 死亡动画 ==========
        // 注意：死亡动画不区分方向，只有光环和墓碑两帧
        AddAnim("barbarian", "death", 175, 176, 0.5f); // 175 死亡光环, 176 墓碑
    }
    else if (type == UnitType::kArcher)
    {
        // 加载弓箭手图集
        SpriteFrameCache::getInstance()->addSpriteFramesWithFile("units/archer/archer.plist");

        // ========== 跑步动画 ==========
        AddAnim("archer", "run_down_right", 1, 8, 0.1f);   // 1~8 downright run
        AddAnim("archer", "run_right", 9, 16, 0.1f);        // 9~16 right run
        AddAnim("archer", "run_up_right", 17, 24, 0.1f);    // 17~24 upright run

        // ========== 待机动画 ==========
        AddAnim("archer", "idle_down_right", 25, 26, 0.5f); // 25,26 downright stand
        AddAnim("archer", "idle_right", 27, 29, 0.3f);      // 27,28,29 right stand
        AddAnim("archer", "idle_up_right", 30, 31, 0.5f);   // 30,31 upright stand

        // ========== 攻击动画（完整射箭流程）==========
        // downright: 32~34(拿箭) + 41~44(射箭) - 跳过35-40
        // 注意：这里为了动画连贯，我们分两段加载
        AddAnim("archer", "attack_down_right", 32, 44, 0.08f);
        // right: 35~37(拿箭) + 45~48(射箭) - 跳过38-44
        AddAnim("archer", "attack_right", 35, 48, 0.08f);
        // upright: 38~40(拿箭) + 49~52(射箭)
        AddAnim("archer", "attack_up_right", 38, 52, 0.08f);

        // ========== 死亡动画 ==========
        AddAnim("archer", "death", 53, 54, 0.5f); // 53 死亡光环, 54 墓碑
    }
    else if (type == UnitType::kGiant)
    {
        // 加载巨人图集
        SpriteFrameCache::getInstance()->addSpriteFramesWithFile("units/giant/giant.plist");

        // ========== 跑步动画（根据分类.txt：1~12 downright, 13~24 right, 25~36 upright）==========
        AddAnim("giant", "run_down_right", 1, 12, 0.12f);   // 1~12 downright run
        AddAnim("giant", "run_right", 13, 24, 0.12f);       // 13~24 right run
        AddAnim("giant", "run_up_right", 25, 36, 0.12f);    // 25~36 upright run

        // ========== 待机动画（根据分类.txt：37 downright, 38 right, 39 upright）==========
        AddAnim("giant", "idle_down_right", 37, 37, 1.0f);  // 37 downright stand main
        AddAnim("giant", "idle_right", 38, 38, 1.0f);       // 38 right stand main
        AddAnim("giant", "idle_up_right", 39, 39, 1.0f);    // 39 upright stand main

        // ========== 攻击动画 1 ==========
        AddAnim("giant", "attack_down_right", 46, 54, 0.11f); // 46~54 downright attack 1 (9帧)
        AddAnim("giant", "attack_right", 55, 63, 0.11f);      // 55~63 right attack 1 (9帧)
        AddAnim("giant", "attack_up_right", 64, 71, 0.11f);   // 64~71 upright attack 1 (8帧)

        // ========== 攻击动画 2 ==========
        AddAnim("giant", "attack2_down_right", 72, 80, 0.11f); // 72~80 downright attack 2 (9帧)
        AddAnim("giant", "attack2_right", 81, 89, 0.11f);      // 81~89 right attack 2 (9帧)
        AddAnim("giant", "attack2_up_right", 90, 97, 0.11f);   // 90~97 upright attack 2 (8帧)

        // ========== 死亡动画 ==========
        AddAnim("giant", "death", 99, 100, 0.5f); // 99 死亡光环, 100 墓碑
    }
    else if (type == UnitType::kGoblin)
    {
        // 加载哥布林图集
        SpriteFrameCache::getInstance()->addSpriteFramesWithFile("units/goblin/goblin.plist");

        // ========== 跑步动画 ==========
        AddAnim("goblin", "run_down_right", 1, 8, 0.09f);   // 1~8 downright run (快速)
        AddAnim("goblin", "run_right", 9, 16, 0.09f);       // 9~16 right run
        AddAnim("goblin", "run_up_right", 17, 24, 0.09f);   // 17~24 upright run

        // ========== 待机动画 ==========
        AddAnim("goblin", "idle_down_right", 25, 25, 1.0f); // 25 downright stand
        AddAnim("goblin", "idle_right", 26, 26, 1.0f);      // 26 right stand
        AddAnim("goblin", "idle_up_right", 27, 27, 1.0f);   // 27 upright stand

        // ========== 攻击动画（只有一套，快速攻击）==========
        AddAnim("goblin", "attack_down_right", 28, 31, 0.08f); // 28~31 downright attack (4帧)
        AddAnim("goblin", "attack_right", 32, 36, 0.08f);      // 32~36 right attack (5帧)
        AddAnim("goblin", "attack_up_right", 37, 40, 0.08f);   // 37~40 upright attack (4帧)

        // ========== 死亡动画 ==========
        AddAnim("goblin", "death", 41, 42, 0.5f); // 41 死亡光环, 42 墓碑
    }
    else if (type == UnitType::kWallBreaker)
    {
        // 加载炸弹人图集
        SpriteFrameCache::getInstance()->addSpriteFramesWithFile("units/wall_breaker/wall_breaker.plist");

        // ========== 跑步动画（抱着炸弹跑）==========
        AddAnim("wall_breaker", "run_down_right", 49, 56, 0.10f); // 49~56 downright run
        AddAnim("wall_breaker", "run_right", 41, 48, 0.10f);      // 41~48 right run
        AddAnim("wall_breaker", "run_up_right", 33, 40, 0.10f);   // 33~40 upright run

        // ========== 待机动画 ==========
        AddAnim("wall_breaker", "idle_down_right", 27, 27, 1.0f); // 27 downright stand
        AddAnim("wall_breaker", "idle_right", 21, 21, 1.0f);      // 21 right stand
        AddAnim("wall_breaker", "idle_up_right", 20, 20, 1.0f);   // 20 upright stand

        // ========== 死亡动画（爆炸）==========
        // 注意：炸弹人的死亡顺序特殊，先光环后墓碑
        AddAnim("wall_breaker", "death", 2, 1, 0.5f); // 2 死亡光环, 1 墓碑
    }
}

// --------------------------------------------------------------------------
// 辅助函数：创建 Animation 对象并缓存
// --------------------------------------------------------------------------
void Unit::AddAnim(const std::string& unitName, const std::string& key, int start, int end, float delay)
{
    Vector<SpriteFrame*> frames;
    // 循环获取每一帧图片
    for (int i = start; i <= end; ++i)
    {
        // 拼接文件名，例如 "barbarian1.0.png"
        std::string name  = StringUtils::format("%s%d.0.png", unitName.c_str(), i);
        auto  frame = SpriteFrameCache::getInstance()->getSpriteFrameByName(name);
        if (frame)
            frames.pushBack(frame);
        else
        {
            // 输出警告，帮助你排查具体缺哪张图
            CCLOG("WARN: SpriteFrame not found: %s", name.c_str());
        }
    }

    // 如果找到了帧，就创建动画对象
    if (!frames.empty())
    {
        auto anim = Animation::createWithSpriteFrames(frames, delay);

        // 【重要】retain() 防止动画被自动释放
        // Cocos 的对象默认是自动释放的，如果我们要存到 map 里长期使用，必须手动 retain
        anim->retain();

        anim_cache_[key] = anim;
    }
    else
    {
        CCLOG("ERROR: No frames found for animation key: %s. Check your .plist file!", key.c_str());
    }
}

// --------------------------------------------------------------------------
// 核心动画控制：根据动作和方向选择动画，并处理翻转
// --------------------------------------------------------------------------
void Unit::PlayAnimation(UnitAction action, UnitDirection dir)
{
    std::string anim_key = "";
    bool        flip_x   = false; // 是否需要水平翻转图片

    // 资源复用逻辑：
    // 美术只画了右半边的图 (右、右上、右下)
    // 左半边的动作通过“水平翻转”右半边的图来实现
    switch (dir)
    {
    case UnitDirection::kRight:
        anim_key = "right";
        flip_x   = false;
        break;
    case UnitDirection::kUpRight:
        anim_key = "up_right";
        flip_x   = false;
        break;
    case UnitDirection::kDownRight:
        anim_key = "down_right";
        flip_x   = false;
        break;

    // 左侧方向：使用对应的右侧动画，但是开启翻转 (flip_x = true)
    case UnitDirection::kLeft:
        anim_key = "right";
        flip_x   = true;
        break;
    case UnitDirection::kUpLeft:
        anim_key = "up_right";
        flip_x   = true;
        break;
    case UnitDirection::kDownLeft:
        anim_key = "down_right";
        flip_x   = true;
        break;

    // 纯上/下：这里偷懒借用了右上/右下
    case UnitDirection::kUp:
        anim_key = "up_right";
        flip_x   = false;
        break;
    case UnitDirection::kDown:
        anim_key = "down_right";
        flip_x   = false;
        break;
    }

    // 拼接最终的 Key，例如 "run_" + "up_right" = "run_up_right"
    std::string prefix = "";
    if (action == UnitAction::kRun)
        prefix = "run_";
    else if (action == UnitAction::kIdle)
        prefix = "idle_";
    else if (action == UnitAction::kAttack)
        prefix = "attack_";
    else if (action == UnitAction::kAttack2)
        prefix = "attack2_";
    else if (action == UnitAction::kDeath)
    {
        // 死亡动画不区分方向，直接使用 "death"
        std::string final_key = "death";
        if (sprite_ && anim_cache_.count(final_key))
        {
            sprite_->stopAllActions();
            sprite_->setFlippedX(false); // 死亡不翻转
            // 死亡动画只播放一次，不循环
            sprite_->runAction(Animate::create(anim_cache_[final_key]));
        }
        return;
    }

    std::string final_key = prefix + anim_key;

    // 🔍 调试日志：检查动画是否存在
    if (type_ == UnitType::kGiant)
    {
        CCLOG("🎬 Giant PlayAnimation: action=%s, dir=%s, final_key=%s, exists=%d",
              prefix.c_str(), anim_key.c_str(), final_key.c_str(), 
              anim_cache_.count(final_key) ? 1 : 0);
    }

    // 只有当动画存在且 Sprite 有效时才播放
    if (sprite_ && anim_cache_.count(final_key))
    {
        sprite_->stopAllActions();    // 停止当前动作
        sprite_->setFlippedX(flip_x); // 设置翻转
        // 运行新动作：RepeatForever 表示无限循环播放
        sprite_->runAction(RepeatForever::create(Animate::create(anim_cache_[final_key])));
        
        if (type_ == UnitType::kGiant)
        {
            CCLOG("✅ Giant animation started: %s", final_key.c_str());
        }
    }
    else
    {
        if (type_ == UnitType::kGiant)
        {
            CCLOG("❌ Giant animation NOT found or sprite is null: %s (sprite=%p, count=%d)", 
                  final_key.c_str(), sprite_, anim_cache_.count(final_key));
        }
    }
}

// --------------------------------------------------------------------------
// 移动指令：设置目标点，计算速度向量
// --------------------------------------------------------------------------
void Unit::MoveTo(const Vec2& target_pos)
{
    // 如果已经死亡，不能移动
    if (is_dead_)
        return;

    target_pos_ = target_pos;

    Vec2 current_pos = this->getPosition();
    Vec2 diff        = target_pos_ - current_pos; // 向量：从当前点指向目标点

    // 如果距离太近 (< 1像素)，视为已到达，不移动
    if (diff.getLength() < 1.0f)
        return;

    // 1. 计算方向并播放跑步动画
    current_dir_ = CalculateDirection(diff);
    
    // 🔍 调试日志
    if (type_ == UnitType::kGiant)
    {
        CCLOG("🏃 Giant MoveTo: pos=(%.1f,%.1f), target=(%.1f,%.1f), distance=%.1f, speed=%.1f", 
              current_pos.x, current_pos.y, target_pos.x, target_pos.y, 
              diff.getLength(), move_speed_);
    }
    
    PlayAnimation(UnitAction::kRun, current_dir_);

    // 2. 计算每帧速度向量
    // normalize() 将向量长度变为1 (单位向量)，保留方向
    // 然后乘以 speed，得到实际每秒移动的像素偏移量
    move_velocity_ = diff.getNormalized() * move_speed_;

    // 3. 标记状态为“正在移动”，update 函数会开始工作
    is_moving_ = true;
}

// --------------------------------------------------------------------------
// 帧循环：每一帧都会被引擎调用 (例如 60FPS 则每秒调用 60 次)
// dt (Delta Time): 距离上一帧过去的时间 (秒)，例如 0.016s
// --------------------------------------------------------------------------
void Unit::update(float dt)
{
    if (!is_moving_)
        return; // 如果没在移动，直接跳过

    Vec2  current_pos = this->getPosition();
    float distance    = current_pos.distance(target_pos_); // 离终点还有多远
    float step        = move_speed_ * dt;                  // 这一帧能走多远 (速度 * 时间)

    // 🔍 调试日志：每60帧输出一次（约1秒）
    static int frameCount = 0;
    if (type_ == UnitType::kGiant && ++frameCount % 60 == 0)
    {
        CCLOG("🎮 Giant update: pos=(%.1f,%.1f), target=(%.1f,%.1f), distance=%.1f, step=%.1f",
              current_pos.x, current_pos.y, target_pos_.x, target_pos_.y, distance, step);
    }

    // 如果 这一帧能走的距离 >= 剩余距离，说明到了
    if (step >= distance)
    {
        this->setPosition(target_pos_);                 // 直接修正到终点 (防止跑过头)
        is_moving_ = false;                             // 停止移动标记
        if (type_ == UnitType::kGiant)
        {
            CCLOG("🎯 Giant reached target!");
        }
        PlayAnimation(UnitAction::kIdle, current_dir_); // 播放待机动画
    }
    else
    {
        // 还没到，按照速度向量移动一步
        // 新位置 = 旧位置 + (速度向量 * 时间间隔)
        this->setPosition(current_pos + move_velocity_ * dt);
    }
}

// --------------------------------------------------------------------------
// 数学辅助：将向量角度映射到 8 个方向枚举
// --------------------------------------------------------------------------
UnitDirection Unit::CalculateDirection(const Vec2& diff)
{
    // 获取向量的角度 (弧度转角度)，范围 -180 到 180
    float angle = CC_RADIANS_TO_DEGREES(diff.getAngle());

    // 根据角度区间判断方向
    // 0度是正右方
    if (angle >= -22.5f && angle < 22.5f)
        return UnitDirection::kRight;
    if (angle >= 22.5f && angle < 67.5f)
        return UnitDirection::kUpRight;
    if (angle >= 67.5f && angle < 112.5f)
        return UnitDirection::kUp;
    if (angle >= 112.5f && angle < 157.5f)
        return UnitDirection::kUpLeft;
    // 157.5 ~ 180 和 -180 ~ -157.5 都是左边
    if (angle >= 157.5f || angle < -157.5f)
        return UnitDirection::kLeft;
    if (angle >= -157.5f && angle < -112.5f)
        return UnitDirection::kDownLeft;
    if (angle >= -112.5f && angle < -67.5f)
        return UnitDirection::kDown;

    return UnitDirection::kDownRight;
}

// --------------------------------------------------------------------------
// 播放攻击动画
// --------------------------------------------------------------------------
void Unit::Attack(bool useSecondAttack)
{
    // 如果已经死亡，不能攻击
    if (is_dead_)
        return;

    // 停止移动
    is_moving_ = false;

    // 播放攻击动画（根据参数选择第一套或第二套）
    UnitAction attackAction = useSecondAttack ? UnitAction::kAttack2 : UnitAction::kAttack;
    PlayAnimation(attackAction, current_dir_);

    CCLOG("Unit attacking with %s", useSecondAttack ? "Attack2" : "Attack1");
}

// --------------------------------------------------------------------------
// 播放死亡动画并标记为已死亡
// --------------------------------------------------------------------------
void Unit::Die()
{
    // 如果已经死亡，不重复播放
    if (is_dead_)
        return;

    // 标记为已死亡
    is_dead_ = true;

    // 停止移动
    is_moving_ = false;

    // 播放死亡动画（死亡动画不区分方向）
    PlayAnimation(UnitAction::kDeath, current_dir_);

    CCLOG("Unit died!");

    // 可选：3秒后移除单位
    auto removeAction = Sequence::create(
        DelayTime::create(3.0f),
        RemoveSelf::create(),
        nullptr
    );
    this->runAction(removeAction);
}

// ==================== 战斗系统实现 ⭐ 新增 ====================

bool Unit::takeDamage(int damage)
{
    if (is_dead_) return true;
    
    int actualDamage = combat_stats_.takeDamage(damage);
    
    CCLOG("Unit took %d damage, HP: %d/%d", 
          actualDamage, 
          combat_stats_.currentHitpoints, 
          combat_stats_.maxHitpoints);
    
    // TODO: 播放受击效果（红光闪烁）
    if (sprite_)
    {
        auto tint = TintTo::create(0.1f, 255, 0, 0);
        auto restore = TintTo::create(0.1f, 255, 255, 255);
        auto seq = Sequence::create(tint, restore, nullptr);
        sprite_->runAction(seq);
    }
    
    if (combat_stats_.currentHitpoints <= 0)
    {
        Die();
        return true;
    }
    
    return false;
}

void Unit::setTarget(BaseBuilding* target)
{
    current_target_ = target;
    
    if (target)
    {
        CCLOG("Unit targeting building");
    }
}

bool Unit::isInAttackRange(const cocos2d::Vec2& targetPos) const
{
    float distance = this->getPosition().distance(targetPos);
    return distance <= combat_stats_.attackRange;
}
