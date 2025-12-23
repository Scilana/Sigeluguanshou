#ifndef __ELEVATOR_UI_H__
#define __ELEVATOR_UI_H__

#include "cocos2d.h"
#include <functional>

/**
 * @brief 电梯楼层选择UI
 *
 * 职责：
 * - 显示可选楼层列表
 * - 处理楼层选择
 * - 通过回调通知选择结果
 */
class ElevatorUI : public cocos2d::Layer
{
public:
    /**
     * @brief 创建电梯UI
     */
    static ElevatorUI* create();

    /**
     * @brief 初始化
     */
    virtual bool init() override;

    /**
     * @brief 设置楼层选择回调
     * @param callback 回调函数，参数为选择的楼层号
     */
    void setFloorSelectCallback(const std::function<void(int)>& callback);

    /**
     * @brief 设置关闭回调
     */
    void setCloseCallback(const std::function<void()>& callback);

    /**
     * @brief 显示UI
     */
    void show();

    /**
     * @brief 关闭UI
     */
    void close();

private:
    std::function<void(int)> floorSelectCallback_;
    std::function<void()> closeCallback_;

    cocos2d::LayerColor* backgroundLayer_;
    cocos2d::Sprite* panelSprite_;
    cocos2d::Label* titleLabel_;
    std::vector<cocos2d::MenuItemLabel*> floorButtons_;

    /**
     * @brief 创建背景
     */
    void createBackground();

    /**
     * @brief 创建面板
     */
    void createPanel();

    /**
     * @brief 创建楼层按钮
     */
    void createFloorButtons();

    /**
     * @brief 楼层按钮点击回调
     */
    void onFloorButtonClicked(cocos2d::Ref* sender, int floor);

    /**
     * @brief 关闭按钮点击回调
     */
    void onCloseButtonClicked(cocos2d::Ref* sender);

    /**
     * @brief 处理键盘按键 (支持数字输入和确认)
     */
    void onKeyPressed(cocos2d::EventKeyboard::KeyCode keyCode, cocos2d::Event* event);

    // 输入相关
    cocos2d::Label* inputLabel_;
    std::string inputText_;
};

#endif // __ELEVATOR_UI_H__
