#ifndef __PLAYER_H__
#define __PLAYER_H__

#include "cocos2d.h"
#include "InventoryManager.h"
#include <functional>

// 前向声明，告诉编译器 MapLayer 类的存在
class MapLayer;
class FarmManager;

/**
 * @brief 玩家类
 * 包含移动逻辑、碰撞检测、动画状态机、战斗属性和能量系统
 */
class Player : public cocos2d::Sprite
{
public:
    static Player* create();
    virtual bool init();

    // 析构函数：用于清理 retain 的动画对象
    virtual ~Player();

    // 每帧更新
    virtual void update(float delta) override;

    /**
     * @brief 启用/禁用键盘控制
     */
    void enableKeyboardControl();
    void disableKeyboardControl();
    void resetKeyStates();

    /**
     * @brief 设置地图层（用于碰撞检测）
     */
    void setMapLayer(MapLayer* mapLayer);
    void setFarmManager(FarmManager* farmManager) { farmManager_ = farmManager; }

    /**
     * @brief 设置移动速度
     */
    void setMoveSpeed(float speed) { moveSpeed_ = speed; }

    // ========== 战斗系统 ==========

    void takeDamage(int damage);

    /**
     * @brief 恢复血量
     */
    void heal(int amount);

    /**
     * @brief 是否处于无敌状态
     */
    bool isInvulnerable() const { return isInvulnerable_; }
    int getHp() const { return hp_; }

    // 获取当前面朝方向（用于判定攻击方向）
    cocos2d::Vec2 getFacingDirection() const { return facingDirection_; }

    // 触发挥舞动作（对外接口）
    void playSwingAnimation();

    // Fishing animation helpers
    void startFishingCast();
    void startFishingWait();
    void startFishingReel(const std::function<void()>& onFinished = nullptr);
    void stopFishingAnimation();
    bool isFishingAnimationPlaying() const { return isFishingAnim_; }

    // ========== 能量系统 ==========
    float getMaxEnergy() const { return maxEnergy_; }
    float getCurrentEnergy() const { return currentEnergy_; }
    void consumeEnergy(float amount);
    void recoverEnergy(float amount);
    bool isExhausted() const { return isExhausted_; }
    void setExhausted(bool exhausted);

    void setCurrentTool(ItemType tool) { currentToolType_ = tool; }  //锄头系统

private:
    // ========== 动画相关定义 ==========

    // 动画状态枚举
    enum class PlayerState {
        IDLE,       // 站立
        WALK_UP,    // 向上走
        WALK_DOWN,  // 向下走
        WALK_LEFT,  // 向左走
        WALK_RIGHT, // 向右走
        // 预留状态，可以在之后扩展
        ATTACK,
        HURT
    };

    PlayerState currentState_;              // 当前状态
    PlayerState stateBeforeAttack_;         // 保存攻击前的状态，用于攻击结束后恢复
    cocos2d::Action* currentAnimAction_;    // 当前正在播放的动作（用于停止它）

    // 缓存的行走动画数据（使用 retain 保持引用）
    cocos2d::Animation* walkUpAnimation_;
    cocos2d::Animation* walkDownAnimation_;
    cocos2d::Animation* walkLeftAnimation_;
    cocos2d::Animation* walkRightAnimation_;

    // 缓存的挥斧动画数据
    cocos2d::Animation* useAxeUpAnimation_;
    cocos2d::Animation* useAxeDownAnimation_;
    cocos2d::Animation* useAxeLeftAnimation_;
    cocos2d::Animation* useAxeRightAnimation_;

    ItemType currentToolType_ = ItemType::None;

    // 【新增】锄头动画缓存
    cocos2d::Animation* useHoeUpAnimation_;
    cocos2d::Animation* useHoeDownAnimation_;
    cocos2d::Animation* useHoeLeftAnimation_;
    cocos2d::Animation* useHoeRightAnimation_;

    /**
     * @brief 加载所有动画资源到内存
     */
    void loadAnimations();

    /**
     * @brief 切换并播放指定状态的动画
     */
    void playAnimation(PlayerState state);

    /**
     * @brief 攻击动画播放结束后的回调函数
     */
    void onAttackAnimationFinished();

    // ========== 移动与属性 ==========

    float moveSpeed_;
    cocos2d::Vec2 moveDirection_;
    cocos2d::Vec2 facingDirection_;
    bool isMoving_;

    // 动作状态标志
    bool isAttacking_;      // 是否正在播放攻击动画（此时锁定移动）
    bool isAttackPressed_;  // 是否按下了攻击键（J键）

    // 按键状态标志
    bool isFishingAnim_ = false;
    cocos2d::Action* fishingAnimAction_ = nullptr;
    bool isUpPressed_;
    bool isDownPressed_;
    bool isLeftPressed_;
    bool isRightPressed_;

    // 地图引用
    MapLayer* mapLayer_;
    FarmManager* farmManager_ = nullptr;

    // 战斗属性
    int hp_;
    int maxHp_;
    bool isInvulnerable_;
    float invulnerableTimer_;

    // 能量属性
    float currentEnergy_;
    float maxEnergy_;
    bool isExhausted_;
    float baseMoveSpeed_; // 记录基础速度以便恢复

    /**
     * @brief 键盘按下/释放
     */
    void onKeyPressed(cocos2d::EventKeyboard::KeyCode keyCode, cocos2d::Event* event);
    void onKeyReleased(cocos2d::EventKeyboard::KeyCode keyCode, cocos2d::Event* event);

    // 物理移动计算
    void updateMovement(float delta);
};

#endif // __PLAYER_H__
