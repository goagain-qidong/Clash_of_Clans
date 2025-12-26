/****************************************************************
 * Project Name:  Clash_of_Clans
 * File Name:     DeploymentValidator.cpp
 * File Function: 部署验证器实现 - 验证单位部署位置的有效性
 * Author:        GitHub Copilot
 * Update Date:   2025/12/26
 * License:       MIT License
 ****************************************************************/

#include "DeploymentValidator.h"

USING_NS_CC;

DeploymentValidator* DeploymentValidator::Create(GridMap* grid_map) {
  DeploymentValidator* validator = new (std::nothrow) DeploymentValidator();
  if (validator && validator->Init(grid_map)) {
    return validator;
  }
  delete validator;
  return nullptr;
}

bool DeploymentValidator::Init(GridMap* grid_map) {
  if (!grid_map) {
    CCLOG("❌ DeploymentValidator::Init: grid_map 为空");
    return false;
  }

  grid_map_ = grid_map;
  grid_width_ = grid_map_->getGridWidth();
  grid_height_ = grid_map_->getGridHeight();

  // 初始化禁止部署地图，默认所有位置都可以部署
  forbidden_map_.resize(
      grid_width_, std::vector<bool>(grid_height_, false));

  CCLOG("✅ DeploymentValidator 初始化成功: 网格大小 %dx%d",
        grid_width_, grid_height_);

  return true;
}

void DeploymentValidator::SetBuildings(
    const std::vector<BaseBuilding*>& buildings) {
  // 重置禁止部署地图
  for (auto& row : forbidden_map_) {
    std::fill(row.begin(), row.end(), false);
  }

  // 标记每个建筑及其周围区域
  for (BaseBuilding* building : buildings) {
    if (building) {
      MarkBuildingForbiddenZone(building);
    }
  }

  // 统计禁止区域数量
  int forbidden_count = 0;
  for (int x = 0; x < grid_width_; ++x) {
    for (int y = 0; y < grid_height_; ++y) {
      if (forbidden_map_[x][y]) {
        ++forbidden_count;
      }
    }
  }

  CCLOG("📊 DeploymentValidator: 已设置 %zu 个建筑，禁止部署区域: %d 个网格",
        buildings.size(), forbidden_count);
}

void DeploymentValidator::MarkBuildingForbiddenZone(BaseBuilding* building) {
  if (!building) {
    return;
  }

  Vec2 grid_pos = building->getGridPosition();
  Size grid_size = building->getGridSize();

  int start_x = static_cast<int>(grid_pos.x);
  int start_y = static_cast<int>(grid_pos.y);
  int building_width = static_cast<int>(grid_size.width);
  int building_height = static_cast<int>(grid_size.height);

  // 计算禁止区域的范围（建筑区域 + 周围一圈）
  int forbidden_start_x = std::max(0, start_x - kForbiddenRadius);
  int forbidden_start_y = std::max(0, start_y - kForbiddenRadius);
  int forbidden_end_x =
      std::min(grid_width_ - 1, start_x + building_width - 1 + kForbiddenRadius);
  int forbidden_end_y =
      std::min(grid_height_ - 1, start_y + building_height - 1 + kForbiddenRadius);

  // 标记禁止区域
  for (int x = forbidden_start_x; x <= forbidden_end_x; ++x) {
    for (int y = forbidden_start_y; y <= forbidden_end_y; ++y) {
      forbidden_map_[x][y] = true;
    }
  }

  CCLOG("🏗️ 标记建筑 %s 禁止区域: (%d,%d) 到 (%d,%d)",
        building->getDisplayName().c_str(),
        forbidden_start_x, forbidden_start_y,
        forbidden_end_x, forbidden_end_y);
}

void DeploymentValidator::RecalculateForbiddenZones() {
  // 此方法在 SetBuildings 中被隐式调用
  // 保留以供将来需要单独重新计算时使用
}

bool DeploymentValidator::CanDeployAtWorldPosition(
    const Vec2& world_position) const {
  if (!grid_map_) {
    return false;
  }

  Vec2 grid_pos = grid_map_->getGridPosition(world_position);
  return CanDeployAtGridPosition(grid_pos);
}

bool DeploymentValidator::CanDeployAtGridPosition(
    const Vec2& grid_position) const {
  int grid_x = static_cast<int>(grid_position.x);
  int grid_y = static_cast<int>(grid_position.y);
  return CanDeployAtGrid(grid_x, grid_y);
}

bool DeploymentValidator::CanDeployAtGrid(int grid_x, int grid_y) const {
  // 边界检查
  if (grid_x < 0 || grid_x >= grid_width_ ||
      grid_y < 0 || grid_y >= grid_height_) {
    return false;
  }

  // 检查是否在禁止区域内
  return !forbidden_map_[grid_x][grid_y];
}

std::vector<Vec2> DeploymentValidator::GetDeployableGridPositions() const {
  std::vector<Vec2> deployable_positions;
  deployable_positions.reserve(grid_width_ * grid_height_ / 2);

  for (int x = 0; x < grid_width_; ++x) {
    for (int y = 0; y < grid_height_; ++y) {
      if (!forbidden_map_[x][y]) {
        deployable_positions.emplace_back(
            static_cast<float>(x), static_cast<float>(y));
      }
    }
  }

  return deployable_positions;
}

std::vector<Vec2> DeploymentValidator::GetForbiddenGridPositions() const {
  std::vector<Vec2> forbidden_positions;
  forbidden_positions.reserve(grid_width_ * grid_height_ / 2);

  for (int x = 0; x < grid_width_; ++x) {
    for (int y = 0; y < grid_height_; ++y) {
      if (forbidden_map_[x][y]) {
        forbidden_positions.emplace_back(
            static_cast<float>(x), static_cast<float>(y));
      }
    }
  }

  return forbidden_positions;
}

int DeploymentValidator::GetGridWidth() const {
  return grid_width_;
}

int DeploymentValidator::GetGridHeight() const {
  return grid_height_;
}
