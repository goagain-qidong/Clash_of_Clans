/****************************************************************
 * Project Name:  Clash_of_Clans
 * File Name:     BuildingManager.h
 * File Function: 建筑管理器 - 管理所有建筑的创建、放置、升级和更新
 * Author:        赵崇治
 * Update Date:   2025/12/14
 * License:       MIT License
 ****************************************************************/
#ifndef BUILDING_MANAGER_H_
#define BUILDING_MANAGER_H_
#include "BuildingData.h"
#include "Buildings/BaseBuilding.h"
#include "cocos2d.h"
#include "GridMap.h"
#include <functional>
#include <vector>

// Forward declaration
struct BuildingSerialData;
struct AccountGameData;
class OccupiedGridOverlay; // 🆕 新增前向声明

/**

 * @class BuildingManager

 * @brief 建筑管理器，负责建筑的创建、放置和生命周期管理

 */
class BuildingManager : public cocos2d::Node
{
public:
    CREATE_FUNC(BuildingManager);
    virtual bool init() override;
    // ==================== 初始化 ====================
    /**

     * @brief 设置地图和网格引用

     * @param mapSprite 地图精灵节点

     * @param gridMap 网格地图对象

     */
    void setup(cocos2d::Sprite* mapSprite, GridMap* gridMap);
    // ==================== 建造模式 ====================
    /**

     * @brief 进入建造模式

     * @param buildingData 要建造的建筑数据

     */
    void startPlacing(const BuildingData& buildingData);
    /** @brief 退出建造模式 */
    void cancelPlacing();
    /** @brief 是否处于建造模式 */
    bool isInBuildingMode() const { return _isBuildingMode; }
    /** @brief 是否正在拖拽建筑 */
    bool isDraggingBuilding() const { return _isDraggingBuilding; }
    /** @brief 是否在等待确认 */
    bool isWaitingConfirm() const { return _isWaitingConfirm; }
    // ==================== 触摸处理（由场景转发） ====================
    /**

     * @brief 处理触摸开始事件

     * @param touchPos 世界坐标系中的触摸位置

     * @return 是否处理了此触摸

     */
    bool onTouchBegan(const cocos2d::Vec2& touchPos);
    /**

     * @brief 处理触摸移动事件

     * @param touchPos 世界坐标系中的触摸位置

     */
    void onTouchMoved(const cocos2d::Vec2& touchPos);
    /**

     * @brief 处理触摸结束事件

     * @param touchPos 世界坐标系中的触摸位置

     */
    void onTouchEnded(const cocos2d::Vec2& touchPos);
    // ==================== 确认/取消建造 ====================
    /** @brief 确认放置建筑 */
    void confirmBuilding();
    /** @brief 取消放置建筑 */
    void cancelBuilding();
    /** @brief 获取待确认建筑的世界坐标位置 */
    cocos2d::Vec2 getPendingBuildingWorldPos() const;
    /** @brief 结束建造模式并清理状态 */
    void endPlacing();
    // ==================== 建筑管理 ====================
    /**

     * @brief 每帧更新，调用所有建筑的 tick 方法

     * @param dt 路上一帧的时间间隔（秒）

     */
    void update(float dt) override;
    /** @brief 获取所有已放置的建筑 */
    const cocos2d::Vector<BaseBuilding*>& getBuildings() const { return _buildings; }
    
    /** @brief 获取所有已放置的建筑（非const版本，用于修改） */
    cocos2d::Vector<BaseBuilding*>& getBuildings() { return _buildings; }
    /**

     * @brief 通过触摸位置查找建筑

     * @param touchPos 世界坐标系中的触摸位置

     * @return 触摸到的建筑，如果没有则返回 nullptr

     */
    BaseBuilding* getBuildingAtPosition(const cocos2d::Vec2& touchPos);
    
    // ==================== Serialization / Multiplayer Support ====================
    
    /**
     * @brief 将所有建筑序列化为数据列表
     * @return 建筑序列化数据列表
     */
    std::vector<BuildingSerialData> serializeBuildings() const;
    
    /**
     * @brief 从序列化数据快速加载建筑（用于加载自己的存档或加载对手的基地）
     * @param buildingsData 建筑序列化数据列表
     * @param isReadOnly 是否为只读模式（true=攻击模式，false=编辑模式）
     */
    void loadBuildingsFromData(const std::vector<BuildingSerialData>& buildingsData, bool isReadOnly = false);
    
    /**
     * @brief 清空所有建筑（切换账号或加载新地图前调用）
     * @param clearTroops 是否同时清空士兵库存（默认true，攻击别人时设为false）
     */
    void clearAllBuildings(bool clearTroops = true);
    
    /**
     * @brief 保存当前建筑状态到当前账号
     */
    void saveCurrentState();
    
    /**
     * @brief 从当前账号加载建筑状态
     */
    void loadCurrentAccountState();
    
    /**
     * @brief 加载指定玩家的建筑布局（用于攻击）
     * @param userId 玩家ID
     * @return 是否加载成功
     */
    bool loadPlayerBase(const std::string& userId);
    
    /**
     * @brief 恢复军营的小兵显示（从士兵库存）
     */
    void restoreArmyCampTroopDisplays();

    // ==================== 回调设置 ====================
    using BuildingPlacedCallback = std::function<void(BaseBuilding*)>;
    using HintCallback = std::function<void(const std::string&)>;
    using BuildingClickedCallback = std::function<void(BaseBuilding*)>;
    using BuildingMovedCallback = std::function<void(BaseBuilding*, const cocos2d::Vec2&)>;
    
    /** @brief 设置建筑放置成功的回调 */
    void setOnBuildingPlaced(const BuildingPlacedCallback& callback) { _onBuildingPlaced = callback; }
    /** @brief 设置显示提示信息的回调 */
    void setOnHint(const HintCallback& callback) { _onHint = callback; }
    /** @brief 设置建筑被左键点击的回调 */
    void setOnBuildingClicked(const BuildingClickedCallback& callback) { _onBuildingClicked = callback; }
    /** @brief 设置建筑移动完成的回调 */
    void setOnBuildingMoved(const BuildingMovedCallback& callback) { _onBuildingMoved = callback; }
    
    // ==================== 建筑移动相关 ====================
    /**
     * @brief 进入建筑移动模式
     * @param building 要移动的建筑
     */
    void startMovingBuilding(BaseBuilding* building);
    /** @brief 退出建筑移动模式 */
    void cancelMovingBuilding();
    /** @brief 确认建筑新位置 */
    void confirmBuildingMove();
    /** @brief 是否正在移动建筑 */
    bool isMovingBuilding() const { return _isMovingBuilding; }
    
    /** @brief 获取正在移动的建筑 */
    BaseBuilding* getMovingBuilding() const { return _movingBuilding; }
    
    // 🆕 显示占用网格覆盖层
    /**
     * @brief 显示所有已有建筑的占用网格（含周围一格）
     * @param autoFadeOut 是否自动淡出（已废弃，保留兼容性）
     */
    void showOccupiedGrids(bool autoFadeOut = true);
    
    /**
     * @brief 淡出并隐藏占用网格覆盖层
     */
    void hideOccupiedGrids();
    
    /**
     * @brief 更新草坪图层（常态显示）
     */
    void updateGrassLayer();
    
    // ==================== 内部方法 ====================
    /**
     * @brief 在指定网格位置放置建筑
     * @param gridPos 网格坐标
     */
    void placeBuilding(const cocos2d::Vec2& gridPos);
    /**
     * @brief 计算建筑在地图上的实际位置
     * @param gridPos 网格坐标
     * @return 地图节点坐标系下的实际位置
     */
    cocos2d::Vec2 calculateBuildingPosition(const cocos2d::Vec2& gridPos) const;
    /**
     * @brief 计算移动建筑时的位置
     * @param gridPos 网格坐标
     * @return 实际位置
     */
    cocos2d::Vec2 calculateBuildingPositionForMoving(const cocos2d::Vec2& gridPos) const;
    /**
     * @brief 处理建筑触摸移动
     * @param touchPos 触摸位置
     */
    void onBuildingTouchMoved(const cocos2d::Vec2& touchPos);
    /**
     * @brief 处理建筑触摸结束
     * @param touchPos 触摸位置
     * @param building 被触摸的建筑指针
     */
    void onBuildingTouchEnded(const cocos2d::Vec2& touchPos, BaseBuilding* building);
    /**
     * @brief 创建建筑实体
     * @param buildingData 建筑数据
     * @return 创建的建筑实体
     */
    BaseBuilding* createBuildingEntity(const BuildingData& buildingData);
    
    /**
     * @brief 从序列化数据创建建筑实体
     * @param data 序列化数据
     * @return 创建的建筑实体
     */
    BaseBuilding* createBuildingFromSerialData(const BuildingSerialData& data);
    
    /** @brief 显示提示信息 */
    void showHint(const std::string& hint);
    /**
     * @brief 为建筑添加点击监听器
     * @param building 要添加监听器的建筑
     */
    void setupBuildingClickListener(BaseBuilding* building);

    // ==================== 成员变量 ====================
    cocos2d::Sprite* _mapSprite = nullptr; // 地图精灵引用
    GridMap* _gridMap = nullptr;           // 网格地图引用
    // 建造模式状态
    bool _isBuildingMode = false;            // 是否在建造模式
    bool _isDraggingBuilding = false;        // 是否正在拖拽
    bool _isWaitingConfirm = false;          // 是否等待确认
    bool _isMovingBuilding = false;          // 是否正在移动建筑
    bool _isReadOnlyMode = false;            // 是否为只读模式（攻击模式）
    cocos2d::Sprite* _ghostSprite = nullptr; // 建筑预览精灵
    BuildingData _selectedBuilding;          // 当前选中的建筑数据
    cocos2d::Vec2 _pendingGridPos;           // 待确认的网格位置
    // 已放置的建筑列表
    cocos2d::Vector<BaseBuilding*> _buildings;
    // 回调函数
    BuildingPlacedCallback _onBuildingPlaced = nullptr;
    HintCallback _onHint = nullptr;
    BuildingClickedCallback _onBuildingClicked = nullptr;
    BuildingMovedCallback _onBuildingMoved = nullptr;
    
    // ==================== 建筑移动状态 ====================
    BaseBuilding* _movingBuilding = nullptr;           // 当前移动的建筑
    cocos2d::Vec2 _buildingOriginalGridPos;            // 建筑原始网格位置
    cocos2d::Sprite* _movingGhostSprite = nullptr;     // 移动时的幽灵精灵
    
    // 🆕 占用网格覆盖层
    OccupiedGridOverlay* _occupiedGridOverlay = nullptr; // 占用网格覆盖层
};
#endif // BUILDING_MANAGER_H_