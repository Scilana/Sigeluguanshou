#ifndef __PLAYER_H__
#define __PLAYER_H__

#include "cocos2d.h"

/**
 * @brief 玩家类
 *
 * 负责：
 * - 玩家角色的显示
 * - WASD键盘控制
 * - 移动逻辑
 * - 碰撞检测
 */
class Player : public cocos2d::Sprite
{
public:
    /**
     * @brief 创建玩家
     * @return Player对象
     */
    static Player* create();

    /**
     * @brief 初始化
     */
    virtual bool init() override;

    /**
     * @brief 更新函数
     * @param delta 时间增量
     */
    virtual void update(float delta) override;

    /**
     * @brief 设置移动速度
     * @param speed 速度（像素/秒）
     */
    void setMoveSpeed(float speed);

    /**
     * @brief 获取移动速度
     */
    float getMoveSpeed() const { return moveSpeed_; }

    /**
     * @brief 设置地图层引用（用于碰撞检测）
     * @param mapLayer 地图层指针
     */
    void setMapLayer(class MapLayer* mapLayer);

    /**
     * @brief 启用键盘控制
     */
    void enableKeyboardControl();

    /**
     * @brief 禁用键盘控制
     */
    void disableKeyboardControl();

private:
    // 地图层引用（用于碰撞检测）
    class MapLayer* mapLayer_;

    // 移动速度（像素/秒）
    float moveSpeed_;

    // 当前移动方向
    cocos2d::Vec2 moveDirection_;

    // 键盘状态
    bool keyW_;
    bool keyA_;
    bool keyS_;
    bool keyD_;

    // 键盘监听器
    cocos2d::EventListenerKeyboard* keyboardListener_;

    /**
     * @brief 按键按下回调
     */
    void onKeyPressed(cocos2d::EventKeyboard::KeyCode keyCode, cocos2d::Event* event);

    /**
     * @brief 按键释放回调
     */
    void onKeyReleased(cocos2d::EventKeyboard::KeyCode keyCode, cocos2d::Event* event);

    /**
     * @brief 更新移动方向
     */
    void updateMoveDirection();

    /**
     * @brief 移动玩家
     * @param delta 时间增量
     */
    void movePlayer(float delta);
};

#endif // __PLAYER_H__