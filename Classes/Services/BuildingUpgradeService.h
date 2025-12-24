/****************************************************************
 * Project Name:  Clash_of_Clans
 * File Name:     BuildingUpgradeService.h
 * File Function: 建筑升级服务 - 统一处理升级业务逻辑
 * Author:        薛毓哲
 * Update Date:   2025/12/08
 * License:       MIT License
 ****************************************************************/
#pragma once

#include <string>

class BaseBuilding;
class ResourceManager;
class UpgradeManager;

/**
 * @enum UpgradeError
 * @brief 升级失败原因枚举
 */
enum class UpgradeError
{
    kSuccess,            ///< 成功
    kMaxLevel,           ///< 已达最高等级
    kAlreadyUpgrading,   ///< 已在升级中
    kNotEnoughGold,      ///< 金币不足
    kNotEnoughElixir,    ///< 圣水不足
    kNotEnoughGem,       ///< 宝石不足
    kNoAvailableBuilder, ///< 无空闲工人
    kStartUpgradeFailed, ///< 启动升级失败
    kUnknownError        ///< 未知错误
};

/**
 * @struct UpgradeResult
 * @brief 升级结果结构体
 */
struct UpgradeResult
{
    bool success;          ///< 是否成功
    UpgradeError error;    ///< 错误码
    std::string message;   ///< 错误信息

    UpgradeResult(bool s, UpgradeError e, const std::string& msg = "")
        : success(s), error(e), message(msg) {}

    /** @brief 创建成功结果 */
    static UpgradeResult Success() {
        return UpgradeResult(true, UpgradeError::kSuccess, "");
    }

    /**
     * @brief 创建失败结果
     * @param err 错误码
     * @param msg 错误信息
     * @return UpgradeResult 失败结果
     */
    static UpgradeResult Failure(UpgradeError err, const std::string& msg) {
        return UpgradeResult(false, err, msg);
    }
};

/**
 * @class BuildingUpgradeService
 * @brief 建筑升级服务类（单例）
 *
 * 职责：
 * - 统一处理建筑升级的业务逻辑
 * - 检查升级前置条件
 * - 执行升级流程
 * - 提供结构化的错误信息
 */
class BuildingUpgradeService
{
public:
    /**
     * @brief 获取单例实例
     * @return BuildingUpgradeService& 单例引用
     */
    static BuildingUpgradeService& getInstance();

    /**
     * @brief 检查建筑是否可以升级
     * @param building 要检查的建筑
     * @return bool 是否可以升级
     */
    bool canUpgrade(const BaseBuilding* building) const;

    /**
     * @brief 尝试升级建筑
     * @param building 要升级的建筑
     * @return UpgradeResult 升级结果
     */
    UpgradeResult tryUpgrade(BaseBuilding* building);

private:
    BuildingUpgradeService();
    BuildingUpgradeService(const BuildingUpgradeService&) = delete;
    BuildingUpgradeService& operator=(const BuildingUpgradeService&) = delete;

    bool checkLevel(const BaseBuilding* building) const;     ///< 检查等级
    bool checkUpgrading(const BaseBuilding* building) const; ///< 检查升级状态
    bool checkResource(const BaseBuilding* building) const;  ///< 检查资源
    bool checkBuilder(const BaseBuilding* building) const;   ///< 检查工人
};
