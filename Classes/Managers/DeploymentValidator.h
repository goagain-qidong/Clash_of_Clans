/****************************************************************
 * Project Name:  Clash_of_Clans
 * File Name:     DeploymentValidator.h
 * File Function: 部署验证器 - 验证单位部署位置的有效性
 * Author:        GitHub Copilot
 * Update Date:   2025/12/26
 * License:       MIT License
 ****************************************************************/

#ifndef DEPLOYMENT_VALIDATOR_H_
#define DEPLOYMENT_VALIDATOR_H_

#include "Buildings/BaseBuilding.h"
#include "GridMap.h"
#include "cocos2d.h"

#include <vector>

/**
 * @class DeploymentValidator
 * @brief 部署验证器 - 检查单位部署位置是否有效
 *
 * 根据 Clash of Clans 的规则，单位只能在建筑物周围一圈网格之外的区域部署。
 * 该类负责：
 * 1. 计算建筑物的禁止部署区域（建筑区域 + 周围一圈）
 * 2. 验证指定位置是否可以部署
 * 3. 提供可部署区域的信息用于可视化
 */
class DeploymentValidator {
 public:
  /**
   * @brief 创建部署验证器实例
   * @param grid_map 网格地图指针
   * @return DeploymentValidator* 验证器实例
   */
  static DeploymentValidator* Create(GridMap* grid_map);

  /**
   * @brief 初始化验证器
   * @param grid_map 网格地图指针
   * @return bool 初始化是否成功
   */
  bool Init(GridMap* grid_map);

  /**
   * @brief 设置需要考虑的建筑列表
   * @param buildings 建筑列表
   * @note 调用此方法后会重新计算禁止部署区域
   */
  void SetBuildings(const std::vector<BaseBuilding*>& buildings);

  /**
   * @brief 检查世界坐标位置是否可以部署单位
   * @param world_position 世界坐标
   * @return bool 该位置是否可以部署
   */
  bool CanDeployAtWorldPosition(const cocos2d::Vec2& world_position) const;

  /**
   * @brief 检查网格坐标位置是否可以部署单位
   * @param grid_position 网格坐标
   * @return bool 该位置是否可以部署
   */
  bool CanDeployAtGridPosition(const cocos2d::Vec2& grid_position) const;

  /**
   * @brief 检查网格坐标位置是否可以部署单位
   * @param grid_x 网格X坐标
   * @param grid_y 网格Y坐标
   * @return bool 该位置是否可以部署
   */
  bool CanDeployAtGrid(int grid_x, int grid_y) const;

  /**
   * @brief 获取所有可部署的网格位置列表
   * @return std::vector<cocos2d::Vec2> 可部署的网格位置列表
   */
  std::vector<cocos2d::Vec2> GetDeployableGridPositions() const;

  /**
   * @brief 获取所有禁止部署的网格位置列表
   * @return std::vector<cocos2d::Vec2> 禁止部署的网格位置列表
   */
  std::vector<cocos2d::Vec2> GetForbiddenGridPositions() const;

  /**
   * @brief 获取网格地图指针
   * @return GridMap* 网格地图指针
   */
  GridMap* GetGridMap() const { return grid_map_; }

  /**
   * @brief 获取网格宽度
   * @return int 网格宽度
   */
  int GetGridWidth() const;

  /**
   * @brief 获取网格高度
   * @return int 网格高度
   */
  int GetGridHeight() const;

 private:
  DeploymentValidator() = default;

  /**
   * @brief 重新计算禁止部署区域
   */
  void RecalculateForbiddenZones();

  /**
   * @brief 标记建筑及其周围区域为禁止部署
   * @param building 建筑指针
   */
  void MarkBuildingForbiddenZone(BaseBuilding* building);

  GridMap* grid_map_ = nullptr;  ///< 网格地图指针

  // 禁止部署地图：true = 禁止部署, false = 可以部署
  std::vector<std::vector<bool>> forbidden_map_;

  int grid_width_ = 0;   ///< 网格宽度
  int grid_height_ = 0;  ///< 网格高度

  static constexpr int kForbiddenRadius = 1;  ///< 建筑周围禁止部署的网格圈数
};

#endif  // DEPLOYMENT_VALIDATOR_H_
