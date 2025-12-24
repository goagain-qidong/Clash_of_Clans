/****************************************************************
 * Project Name:  Clash_of_Clans
 * File Name:     TroopInventory.h
 * File Function: 士兵库存管理器 - 管理玩家拥有的所有士兵
 * Author:        薛毓哲
 * Update Date:   2025/01/10
 * License:       MIT License
 ****************************************************************/
#ifndef TROOP_INVENTORY_H_
#define TROOP_INVENTORY_H_

#include "Unit/UnitTypes.h"
#include "cocos2d.h"

#include <functional>
#include <map>

/**
 * @class TroopInventory
 * @brief 士兵库存管理器（单例）
 *
 * 功能：
 * - 存储各兵种的数量
 * - 提供增加/消耗士兵的接口
 * - 序列化/反序列化（保存到账号数据）
 * - 通知UI更新
 */
class TroopInventory
{
public:
    /**
     * @brief 获取单例实例
     * @return TroopInventory& 单例引用
     */
    static TroopInventory& getInstance();

    /**
     * @brief 获取指定兵种的数量
     * @param type 兵种类型
     * @return int 数量
     */
    int getTroopCount(UnitType type) const;

    /**
     * @brief 添加士兵
     * @param type 兵种类型
     * @param count 数量
     * @return int 实际添加的数量
     */
    int addTroops(UnitType type, int count);

    /**
     * @brief 消耗士兵
     * @param type 兵种类型
     * @param count 数量
     * @return bool 是否成功
     */
    bool consumeTroops(UnitType type, int count);

    /**
     * @brief 检查是否有足够的士兵
     * @param type 兵种类型
     * @param count 数量
     * @return bool 是否足够
     */
    bool hasEnoughTroops(UnitType type, int count) const;

    /**
     * @brief 获取所有士兵的总人口数
     * @return int 总人口数
     */
    int getTotalPopulation() const;

    /** @brief 清空所有士兵 */
    void clearAll();

    /** @brief 获取所有兵种的数量 */
    const std::map<UnitType, int>& getAllTroops() const { return _troops; }

    /**
     * @brief 设置所有兵种的数量
     * @param troops 新的部队库存
     */
    void setAllTroops(const std::map<UnitType, int>& troops);

    /**
     * @brief 导出为JSON字符串
     * @return std::string JSON字符串
     */
    std::string toJson() const;

    /**
     * @brief 从JSON字符串导入
     * @param jsonStr JSON字符串
     * @return bool 是否成功
     */
    bool fromJson(const std::string& jsonStr);

    /**
     * @brief 保存到文件
     * @param forceUserId 强制使用的用户ID
     */
    void save(const std::string& forceUserId = "");

    /** @brief 从文件加载 */
    void load();

    /**
     * @brief 设置士兵数量变化回调
     * @param callback 回调函数
     */
    void setOnTroopChangeCallback(const std::function<void(UnitType, int)>& callback);

private:
    TroopInventory();
    ~TroopInventory() = default;

    static TroopInventory* _instance;  ///< 单例实例

    std::map<UnitType, int> _troops;   ///< 士兵库存

    std::function<void(UnitType, int)> _onTroopChangeCallback;  ///< 变化回调

    void notifyChange(UnitType type, int newCount);  ///< 触发回调
    int getUnitPopulation(UnitType type) const;      ///< 获取兵种人口数
};

#endif // TROOP_INVENTORY_H_