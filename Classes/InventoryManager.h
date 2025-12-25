#ifndef __INVENTORY_MANAGER_H__
#define __INVENTORY_MANAGER_H__

#include "cocos2d.h"
#include <array>
#include <string>

/**
 * @brief 物品类型枚举（与 GameScene 中保持一致）
 */
enum class ItemType
{
    None = -1,
    Hoe = 0,
    WateringCan,
    Scythe,
    Axe,
    Pickaxe,
    FishingRod,
    SeedTurnip,
    SeedPotato,
    SeedCorn,
    SeedTomato,
    SeedPumpkin,
    SeedBlueberry,
    Wood,
    Turnip,
    Potato,
    Corn,
    Tomato,
    Pumpkin,
    Blueberry,
    Fish,
    // 矿石类型
    CopperOre,      // 铜矿石
    IronOre,        // 铁矿石 [New]
    SilverOre,      // 银矿石
    GoldOre,        // 金矿石
    DiamondOre,     // 钻石矿石 [New]
    // 武器类型
    WoodenSword,    // 木剑
    IronSword,      // 铁剑
    GoldSword,      // 金剑
    DiamondSword    // 钻石剑
};

/**
 * @brief 背包管理器类
 *
 * 职责：
 * - 管理玩家的物品存储
 * - 处理物品添加、移除、查询
 * - 管理金币数量
 * - 提供物品数据和元信息
 */
class InventoryManager : public cocos2d::Node
{
public:
    /**
     * @brief 物品槽位结构
     */
    struct ItemSlot
    {
        ItemType type = ItemType::None;
        int count = 0;

        bool isEmpty() const { return type == ItemType::None || count <= 0; }
        void clear() { type = ItemType::None; count = 0; }
    };

    static const int MAX_SLOTS = 30;  // 背包总槽位数
    
    /**
     * @brief 获取单例实例
     */
    static InventoryManager* getInstance();
    
    /**
     * @brief 销毁单例实例
     */
    static void destroyInstance();

    /**
     * @brief 初始化
     */
    virtual bool init() override;

    /**
     * @brief 添加物品
     * @param itemType 物品类型
     * @param count 数量
     * @return 是否成功添加
     */
    bool addItem(ItemType itemType, int count = 1);

    /**
     * @brief 移除物品
     * @param itemType 物品类型
     * @param count 数量
     * @return 是否成功移除
     */
    bool removeItem(ItemType itemType, int count = 1);

    /**
     * @brief 检查是否有指定物品
     * @param itemType 物品类型
     * @param count 数量
     * @return 是否拥有足够数量
     */
    bool hasItem(ItemType itemType, int count = 1) const;

    /**
     * @brief 获取指定物品的总数量
     * @param itemType 物品类型
     * @return 物品总数量
     */
    int getItemCount(ItemType itemType) const;

    /**
     * @brief 获取指定槽位的物品
     * @param slotIndex 槽位索引（0-29）
     * @return 物品槽位引用
     */
    const ItemSlot& getSlot(int slotIndex) const;

    /**
     * @brief 获取所有槽位
     * @return 槽位数组
     */
    const std::array<ItemSlot, MAX_SLOTS>& getAllSlots() const { return slots_; }

    /**
     * @brief 设置槽位物品
     * @param slotIndex 槽位索引
     * @param itemType 物品类型
     * @param count 数量
     */
    void setSlot(int slotIndex, ItemType itemType, int count);

    /**
     * @brief 交换两个槽位
     * @param index1 槽位1索引
     * @param index2 槽位2索引
     */
    void swapSlots(int index1, int index2);

    /**
     * @brief 获取金币数量
     */
    int getMoney() const { return money_; }

    /**
     * @brief 添加金币
     * @param amount 金币数量
     */
    void addMoney(int amount);

    /**
     * @brief 移除金币
     * @param amount 金币数量
     * @return 是否成功移除
     */
    bool removeMoney(int amount);

    /**
     * @brief 获取物品名称
     * @param itemType 物品类型
     * @return 物品名称
     */
    static std::string getItemName(ItemType itemType);

    /**
     * @brief 获取物品描述
     * @param itemType 物品类型
     * @return 物品描述
     */
    static std::string getItemDescription(ItemType itemType);

    /**
     * @brief 获取物品是否可堆叠
     * @param itemType 物品类型
     * @return 是否可堆叠
     */
    static bool isStackable(ItemType itemType);

    /**
     * @brief 获取物品最大堆叠数
     * @param itemType 物品类型
     * @return 最大堆叠数
     */
    static int getMaxStack(ItemType itemType);

    /**
     * @brief 清空背包
     */
    void clear();



private:
    std::array<ItemSlot, MAX_SLOTS> slots_;  // 物品槽位数组
    int money_;                               // 金币数量
    int selectedSlotIndex_ = 0;               // 当前选中的槽位

public:
    int getSelectedSlotIndex() const;
    void setSelectedSlotIndex(int index);

    /**
     * @brief 初始化默认物品
     */
    void initDefaultItems();

    /**
     * @brief 查找第一个空槽位
     * @return 槽位索引，-1 表示没有空槽位
     */
    int findEmptySlot() const;

    /**
     * @brief 查找指定物品的槽位
     * @param itemType 物品类型
     * @return 槽位索引，-1 表示未找到
     */
    int findItemSlot(ItemType itemType) const;
};

#endif // __INVENTORY_MANAGER_H__
