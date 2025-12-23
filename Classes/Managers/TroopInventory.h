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
 * @brief 士兵库存管理器 - 单例模式
 *
 * 功能：
 * 1. 存储各兵种的数量
 * 2. 提供增加/消耗士兵的接口
 * 3. 序列化/反序列化（保存到账号数据）
 * 4. 通知UI更新
 */
class TroopInventory
{
public:
    // 单例访问
    static TroopInventory& getInstance();

    // ==================== 士兵数量管理 ====================

    /**
     * @brief 获取指定兵种的数量
     * @param type 兵种类型
     * @return 数量
     */
    int getTroopCount(UnitType type) const;

    /**
     * @brief 添加士兵
     * @param type 兵种类型
     * @param count 数量
     * @return 实际添加的数量（受人口上限限制）
     */
    int addTroops(UnitType type, int count);

    /**
     * @brief 消耗士兵（用于战斗部署）
     * @param type 兵种类型
     * @param count 数量
     * @return 是否成功
     */
    bool consumeTroops(UnitType type, int count);

    /**
     * @brief 检查是否有足够的士兵
     * @param type 兵种类型
     * @param count 数量
     * @return 是否足够
     */
    bool hasEnoughTroops(UnitType type, int count) const;

    /**
     * @brief 获取所有士兵的总人口数
     * @return 总人口数
     */
    int getTotalPopulation() const;

    /**
     * @brief 清空所有士兵（谨慎使用）
     */
    void clearAll();

    /**
     * @brief 获取所有兵种的数量（用于UI显示）
     * @return map<兵种类型, 数量>
     */
    const std::map<UnitType, int>& getAllTroops() const { return _troops; }

    /**
     * @brief 设置所有兵种的数量（用于战斗后返还）
     * @param troops 新的部队库存
     */
    void setAllTroops(const std::map<UnitType, int>& troops);

    // ==================== 序列化/反序列化 ====================

    /**
     * @brief 导出为JSON字符串
     * @return JSON字符串
     */
    std::string toJson() const;

    /**
     * @brief 从JSON字符串导入
     * @param jsonStr JSON字符串
     * @return 是否成功
     */
    bool fromJson(const std::string& jsonStr);

    /**
     * @brief 保存到文件（按账号）
     */
    void save(const std::string& forceUserId = ""); // 🔥 新增可选参数

    /**
     * @brief 从文件加载（按账号）
     */
    void load();

    // ==================== 回调通知 ====================

    /**
     * @brief 设置士兵数量变化回调（用于更新UI）
     * @param callback 回调函数：void callback(UnitType type, int newCount)
     */
    void setOnTroopChangeCallback(const std::function<void(UnitType, int)>& callback);

private:
    TroopInventory();
    ~TroopInventory() = default;

    static TroopInventory* _instance;

    // 士兵库存：<兵种类型, 数量>
    std::map<UnitType, int> _troops;

    // 变化通知回调
    std::function<void(UnitType, int)> _onTroopChangeCallback;

    // 触发回调
    void notifyChange(UnitType type, int newCount);

    // 获取兵种人口数（辅助函数）
    int getUnitPopulation(UnitType type) const;
};

#endif // TROOP_INVENTORY_H_