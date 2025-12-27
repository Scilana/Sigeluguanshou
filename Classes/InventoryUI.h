#ifndef __INVENTORY_UI_H__
#define __INVENTORY_UI_H__

#include "cocos2d.h"
#include "InventoryManager.h"
#include <functional>

class MarketState;
class StorageChest;

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
    static InventoryUI* create(InventoryManager* inventory, MarketState* marketState = nullptr);

    /**
     * @brief 初始化
     * @param inventory 背包管理器引用
     */
    bool init(InventoryManager* inventory, MarketState* marketState);

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
     * @brief 设置合作伙伴仓库（显示另一个窗口的数据以进行转账）
     */
    void setPartnerInventory(InventoryManager* partner, bool isShippingBin = false) { 
        partnerInventory_ = partner; 
        partnerIsShippingBin_ = isShippingBin;
    }

    /**
     * @brief 显示动画
     */
    void show();

    /**
     * @brief 关闭动画
     */
    void close();

    void setInventoryManager(InventoryManager* inventoryManager);
    void updateSlot(int index);
    
    // Selection Mode
    typedef std::function<void(int slotIndex, ItemType type, int count)> ItemSelectCallback;
    void setSelectionMode(bool enabled);
    void setOnItemSelectedCallback(const ItemSelectCallback& callback);
    bool isSelectionMode() const { return _selectionMode; }
    
private:
    InventoryManager* inventory_;
    std::vector<cocos2d::Sprite*> _slotSprites;
    std::vector<cocos2d::Label*> _countLabels;
    std::vector<cocos2d::Sprite*> _iconSprites;
    
    // Selection
    bool _selectionMode;
    ItemSelectCallback _onItemSelected;
    MarketState* marketState_{ nullptr };            // 市场状态，用于查询价格
    std::function<void()> closeCallback_;            // 关闭回调

    cocos2d::LayerColor* background_;                // 半透明背景
    cocos2d::LayerColor* panel_;                     // 背包面板
    cocos2d::Label* titleLabel_;                     // 标题
    cocos2d::Label* moneyLabel_;                     // 金币显示
    cocos2d::Label* infoLabel_;                      // 物品信息显示


    int selectedSlotIndex_;                          // 当前选中的槽位索引
    void updateSelection();                          // 更新选中状态显示

    static const int ROWS = 4;                       // 行数 (4x8=32)
    static const int COLS = 8;                       // 列数
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

    // Removed duplicate updateSlot declaration

    /**
     * @brief 创建格子背景
     * @return 格子背景精灵
     */
    cocos2d::Sprite* createSlotBackground();

    /**
     * @brief 创建物品图标（含耐久度条）
     * @param slot 物品槽位数据
     * @return 物品图标精灵
     */
    cocos2d::Sprite* createItemIcon(const InventoryManager::ItemSlot& slot);

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

    InventoryManager* partnerInventory_{ nullptr };
    bool partnerIsShippingBin_{ false };
    void handleTransfer();
};

#endif // __INVENTORY_UI_H__
