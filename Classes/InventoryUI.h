#ifndef __INVENTORY_UI_H__
#define __INVENTORY_UI_H__

#include "cocos2d.h"
#include "InventoryManager.h"
#include <functional>

/**
 * @brief 背包UI层
 *
 * 职责：
 * - 显示背包界面
 * - 处理格子点击、拖拽
 * - 显示物品信息和数量
 * - 提供关闭回调
 */
class InventoryUI : public cocos2d::Layer
{
public:
    /**
     * @brief 创建背包UI
     * @param inventory 背包管理器引用
     */
    static InventoryUI* create(InventoryManager* inventory);

    /**
     * @brief 初始化
     * @param inventory 背包管理器引用
     */
    bool init(InventoryManager* inventory);

    /**
     * @brief 设置关闭回调
     * @param callback 关闭时调用的回调函数
     */
    void setCloseCallback(const std::function<void()>& callback);

    /**
     * @brief 刷新显示
     */
    void refresh();

    /**
     * @brief 显示动画
     */
    void show();

    /**
     * @brief 关闭动画
     */
    void close();

private:
    InventoryManager* inventory_;                    // 背包管理器引用
    std::function<void()> closeCallback_;            // 关闭回调

    cocos2d::LayerColor* background_;                // 半透明背景
    cocos2d::LayerColor* panel_;                     // 背包面板
    cocos2d::Label* titleLabel_;                     // 标题
    cocos2d::Label* moneyLabel_;                     // 金币显示
    cocos2d::Label* infoLabel_;                      // 物品信息显示

    static const int ROWS = 5;                       // 行数
    static const int COLS = 6;                       // 列数
    static const float SLOT_SIZE;                    // 格子大小
    static const float SLOT_SPACING;                 // 格子间距

    struct SlotSprite
    {
        cocos2d::Sprite* background;                 // 格子背景
        cocos2d::Sprite* icon;                       // 物品图标
        cocos2d::Label* countLabel;                  // 数量标签
        int slotIndex;                               // 对应的槽位索引
    };

    std::vector<SlotSprite> slotSprites_;            // 所有格子精灵

    /**
     * @brief 初始化背景
     */
    void initBackground();

    /**
     * @brief 初始化面板
     */
    void initPanel();

    /**
     * @brief 初始化格子
     */
    void initSlots();

    /**
     * @brief 初始化控制按钮
     */
    void initControls();

    /**
     * @brief 更新格子显示
     * @param slotIndex 槽位索引
     */
    void updateSlot(int slotIndex);

    /**
     * @brief 创建格子背景
     * @return 格子背景精灵
     */
    cocos2d::Sprite* createSlotBackground();

    /**
     * @brief 创建物品图标（占位符）
     * @param itemType 物品类型
     * @return 物品图标精灵
     */
    cocos2d::Sprite* createItemIcon(ItemType itemType);

    /**
     * @brief 处理格子点击
     * @param slotIndex 槽位索引
     */
    void onSlotClicked(int slotIndex);

    /**
     * @brief 处理键盘事件
     */
    void onKeyPressed(cocos2d::EventKeyboard::KeyCode keyCode, cocos2d::Event* event);

    /**
     * @brief 获取物品颜色
     * @param itemType 物品类型
     * @return 颜色
     */
    cocos2d::Color3B getItemColor(ItemType itemType) const;
};

#endif // __INVENTORY_UI_H__
