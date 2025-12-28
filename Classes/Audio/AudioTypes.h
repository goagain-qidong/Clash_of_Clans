/****************************************************************
 * Project Name:  Clash_of_Clans
 * File Name:     AudioTypes.h
 * File Function: 音频类型枚举定义
 * Author:        赵崇治
 * Update Date:   2025/12/28
 * License:       MIT License
 ****************************************************************/

#ifndef AUDIO_TYPES_H_
#define AUDIO_TYPES_H_

/**
 * @enum MusicId
 * @brief 背景音乐类型枚举
 *
 * 定义游戏中所有背景音乐的类型，便于统一管理和调用。
 */
enum class MusicId {
  kNone = 0,            ///< 无音乐

  // 战斗相关音乐
  kBattlePreparing,     ///< 战斗准备阶段
  kBattleGoing,         ///< 战斗进行中
  kBattleWin,           ///< 战斗胜利
  kBattleLose,          ///< 战斗失败

  kMusicCount           ///< 音乐总数（用于计数）
};

/**
 * @enum SoundEffectId
 * @brief 音效类型枚举
 *
 * 按照功能分类定义游戏中所有音效。
 * 命名规则：k + 类别 + 动作/状态
 */
enum class SoundEffectId {
  kNone = 0,            ///< 无音效

  // ==================== UI 音效 ====================
  kUiButtonClick,       ///< 按钮点击
  kUiPanelOpen,         ///< 面板打开
  kUiPanelClose,        ///< 面板关闭
  kUiLoadingEnter,      ///< 加载界面进入
  kUiSupercellLogo,     ///< Supercell Logo 进入

  // ==================== 资源采集音效 ====================
  kResourceGoldCollect,     ///< 金币采集
  kResourceElixirCollect,   ///< 圣水采集
  kResourceGemCollect,      ///< 宝石采集

  // ==================== 野蛮人音效 ====================
  kBarbarianDeploy,     ///< 野蛮人部署
  kBarbarianAttack,     ///< 野蛮人攻击（随机变体）
  kBarbarianDeath,      ///< 野蛮人死亡（随机变体）
  kBarbarianMove,       ///< 野蛮人移动

  // ==================== 弓箭手音效 ====================
  kArcherDeploy,        ///< 弓箭手部署
  kArcherAttack,        ///< 弓箭手攻击（随机变体）
  kArcherDeath,         ///< 弓箭手死亡

  // ==================== 巨人音效 ====================
  kGiantDeploy,         ///< 巨人部署
  kGiantAttack,         ///< 巨人攻击（随机变体）
  kGiantHit,            ///< 巨人攻击命中（随机变体）
  kGiantDeath,          ///< 巨人死亡

  // ==================== 哥布林音效 ====================
  kGoblinDeploy,        ///< 哥布林部署
  kGoblinAttack,        ///< 哥布林攻击（随机变体）
  kGoblinDeath,         ///< 哥布林死亡

  // ==================== 炸弹人音效 ====================
  kWallBreakerDeploy,   ///< 炸弹人部署
  kWallBreakerAttack,   ///< 炸弹人攻击（爆炸）
  kWallBreakerDeath,    ///< 炸弹人死亡

  // ==================== 防御建筑音效 ====================
  kCannonPickup,        ///< 加农炮拾取
  kCannonPlace,         ///< 加农炮放置
  kCannonFire,          ///< 加农炮开火
  kArcherTowerPickup,   ///< 箭塔拾取
  kArcherTowerPlace,    ///< 箭塔放置
  kWallPickup,          ///< 城墙拾取
  kWallPlace,           ///< 城墙放置

  // ==================== 资源建筑音效 ====================
  kGoldMinePickup,      ///< 金矿拾取
  kGoldMinePlace,       ///< 金矿放置
  kElixirStoragePickup, ///< 圣水存储拾取
  kElixirStoragePlace,  ///< 圣水存储放置

  // ==================== 通用建筑音效 ====================
  kBuildingPickup,      ///< 建筑拾取
  kBuildingConstruct,   ///< 建筑建造中
  kBuildingFinished,    ///< 建筑完成
  kBuildingDestroyed,   ///< 建筑被摧毁
  kBuilderHutPickup,    ///< 建筑工人小屋拾取
  kBuilderHutPlace,     ///< 建筑工人小屋放置

  kSoundEffectCount     ///< 音效总数（用于计数）
};

/**
 * @enum AudioState
 * @brief 音频播放状态
 */
enum class AudioState {
  kStopped,   ///< 已停止
  kPlaying,   ///< 正在播放
  kPaused     ///< 已暂停
};

#endif  // AUDIO_TYPES_H_
