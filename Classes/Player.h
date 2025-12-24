#ifndef __PLAYER_H__
#define __PLAYER_H__

#include "cocos2d.h"

// 前向声明
class MapLayer;

/**
 * @brief 玩家类
 */
class Player : public cocos2d::Sprite
{
public:
    static Player* create();
    virtual bool init();
    virtual void update(float delta) override;

    /**
     * @brief 启用键盘控制
     */
    void enableKeyboardControl();

    /**
     * @brief 设置地图层（用于碰撞检测）
     */
    void setMapLayer(MapLayer* mapLayer);

    /**
     * @brief 设置移动速度
     */
    void setMoveSpeed(float speed) { moveSpeed_ = speed; }

    // ========== 战斗系统 ==========

    /**
     * @brief 受到伤害
     */
    void takeDamage(int damage);

    /**
     * @brief 恢复血量
     */
    void heal(int amount);

    /**
     * @brief 是否处于无敌状态
     */
    bool isInvulnerable() const { return isInvulnerable_; }

    /**
     * @brief 当前血量
     */
    int getHp() const { return hp_; }

    /**
     * @brief 获取玩家朝向
     */
    cocos2d::Vec2 getFacingDirection() const { return facingDirection_; }

private:
    float moveSpeed_;
    cocos2d::Vec2 moveDirection_;
    cocos2d::Vec2 facingDirection_;
    bool isMoving_;
    
    // 按键状态标志（解决场景切换导致的按键状态不一致问题）
    bool isUpPressed_;
    bool isDownPressed_;
    bool isLeftPressed_;
    bool isRightPressed_;

    // 地图引用
    MapLayer* mapLayer_;

    // 战斗属性
    int hp_;
    int maxHp_;
    bool isInvulnerable_;
    float invulnerableTimer_;

    /**
     * @brief 键盘按下
     */
    void onKeyPressed(cocos2d::EventKeyboard::KeyCode keyCode, cocos2d::Event* event);

    /**
     * @brief 键盘抬起
     */
    void onKeyReleased(cocos2d::EventKeyboard::KeyCode keyCode, cocos2d::Event* event);

    /**
     * @brief 更新移动
     */
    void updateMovement(float delta);
};

#endif // __PLAYER_H__