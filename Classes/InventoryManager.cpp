#include "InventoryManager.h"

USING_NS_CC;

static InventoryManager* s_instance = nullptr;

InventoryManager* InventoryManager::getInstance()
{
    if (!s_instance)
    {
        s_instance = new (std::nothrow) InventoryManager();
        if (s_instance && s_instance->init())
        {
            // 不调用 autorelease，保持引用计数为 1
            CCLOG("InventoryManager instance created");
        }
        else
        {
            CC_SAFE_DELETE(s_instance);
        }
    }
    return s_instance;
}

void InventoryManager::destroyInstance()
{
    if (s_instance)
    {
        s_instance->release();
        s_instance = nullptr;
        CCLOG("InventoryManager instance destroyed");
    }
}

bool InventoryManager::init()
{
    if (!Node::init())
        return false;

    // 初始化所有槽位为空
    for (auto& slot : slots_)
    {
        slot.clear();
    }

    // 初始化默认金币和物品
    money_ = 500;
    selectedSlotIndex_ = 0; // 默认选中第一个
    initDefaultItems();

    CCLOG("InventoryManager initialized with %d slots and %d gold", MAX_SLOTS, money_);

    return true;
}

void InventoryManager::initDefaultItems()
{
    // 初始工具（前5个槽位）
    setSlot(0, ItemType::Hoe, 1);
    setSlot(1, ItemType::WateringCan, 1);
    setSlot(2, ItemType::Scythe, 1);
    setSlot(3, ItemType::Axe, 1);
    setSlot(4, ItemType::Pickaxe, 1);
    setSlot(5, ItemType::FishingRod, 1);

    // 初始种子
    setSlot(6, ItemType::SeedTurnip, 10);
    setSlot(7, ItemType::SeedPotato, 10);
    setSlot(8, ItemType::SeedCorn, 5);
    setSlot(9, ItemType::SeedTomato, 5);
    setSlot(10, ItemType::SeedPumpkin, 3);
    setSlot(11, ItemType::SeedBlueberry, 3);

    // 初始木材
    setSlot(12, ItemType::Wood, 20);
}

bool InventoryManager::addItem(ItemType itemType, int count)
{
    if (itemType == ItemType::None || count <= 0)
        return false;

    // 如果物品可堆叠，先尝试堆叠到现有槽位
    if (isStackable(itemType))
    {
        int maxStack = getMaxStack(itemType);

        // 查找已有该物品的槽位
        for (auto& slot : slots_)
        {
            if (slot.type == itemType && slot.count < maxStack)
            {
                int spaceLeft = maxStack - slot.count;
                int addCount = std::min(spaceLeft, count);
                slot.count += addCount;
                count -= addCount;

                if (count <= 0)
                {
                    CCLOG("Added %s (stacked)", getItemName(itemType).c_str());
                    return true;
                }
            }
        }
    }

    // 如果还有剩余，放入空槽位
    while (count > 0)
    {
        int emptySlot = findEmptySlot();
        if (emptySlot == -1)
        {
            CCLOG("Inventory full! Cannot add %s", getItemName(itemType).c_str());
            return false;
        }

        int maxStack = getMaxStack(itemType);
        int addCount = std::min(maxStack, count);
        slots_[emptySlot].type = itemType;
        slots_[emptySlot].count = addCount;
        count -= addCount;

        CCLOG("Added %d x %s to slot %d", addCount, getItemName(itemType).c_str(), emptySlot);
    }

    return true;
}

bool InventoryManager::removeItem(ItemType itemType, int count)
{
    if (itemType == ItemType::None || count <= 0)
        return false;

    // 先检查是否有足够的物品
    if (!hasItem(itemType, count))
    {
        CCLOG("Not enough %s to remove", getItemName(itemType).c_str());
        return false;
    }

    // 从槽位中移除
    int remaining = count;
    for (auto& slot : slots_)
    {
        if (slot.type == itemType)
        {
            int removeCount = std::min(slot.count, remaining);
            slot.count -= removeCount;
            remaining -= removeCount;

            if (slot.count <= 0)
            {
                slot.clear();
            }

            if (remaining <= 0)
                break;
        }
    }

    CCLOG("Removed %d x %s", count, getItemName(itemType).c_str());
    return true;
}

bool InventoryManager::hasItem(ItemType itemType, int count) const
{
    return getItemCount(itemType) >= count;
}

int InventoryManager::getItemCount(ItemType itemType) const
{
    int total = 0;
    for (const auto& slot : slots_)
    {
        if (slot.type == itemType)
        {
            total += slot.count;
        }
    }
    return total;
}

const InventoryManager::ItemSlot& InventoryManager::getSlot(int slotIndex) const
{
    static ItemSlot emptySlot;
    if (slotIndex < 0 || slotIndex >= MAX_SLOTS)
        return emptySlot;
    return slots_[slotIndex];
}

void InventoryManager::setSlot(int slotIndex, ItemType itemType, int count)
{
    if (slotIndex < 0 || slotIndex >= MAX_SLOTS)
        return;

    slots_[slotIndex].type = itemType;
    slots_[slotIndex].count = count;
}

void InventoryManager::swapSlots(int index1, int index2)
{
    if (index1 < 0 || index1 >= MAX_SLOTS || index2 < 0 || index2 >= MAX_SLOTS)
        return;

    ItemSlot temp = slots_[index1];
    slots_[index1] = slots_[index2];
    slots_[index2] = temp;

    CCLOG("Swapped slots %d and %d", index1, index2);
}

void InventoryManager::addMoney(int amount)
{
    money_ += amount;
    CCLOG("Added %d gold. Total: %d", amount, money_);
}

bool InventoryManager::removeMoney(int amount)
{
    if (money_ < amount)
    {
        CCLOG("Not enough money! Have %d, need %d", money_, amount);
        return false;
    }

    money_ -= amount;
    CCLOG("Removed %d gold. Remaining: %d", amount, money_);
    return true;
}

void InventoryManager::clear()
{
    for (auto& slot : slots_)
    {
        slot.clear();
    }
    money_ = 0;
    CCLOG("Inventory cleared");
}

int InventoryManager::findEmptySlot() const
{
    for (int i = 0; i < MAX_SLOTS; ++i)
    {
        if (slots_[i].isEmpty())
            return i;
    }
    return -1;
}

int InventoryManager::findItemSlot(ItemType itemType) const
{
    for (int i = 0; i < MAX_SLOTS; ++i)
    {
        if (slots_[i].type == itemType)
            return i;
    }
    return -1;
}

// ========== 静态物品信息函数 ==========

std::string InventoryManager::getItemName(ItemType itemType)
{
    switch (itemType)
    {
    case ItemType::None: return "Empty";
    case ItemType::Hoe: return "Hoe";
    case ItemType::WateringCan: return "Watering Can";
    case ItemType::Scythe: return "Scythe";
    case ItemType::Axe: return "Axe";
    case ItemType::Pickaxe: return "Pickaxe";
    case ItemType::FishingRod: return "Fishing Rod";
    case ItemType::SeedTurnip: return "Turnip Seed";
    case ItemType::SeedPotato: return "Potato Seed";
    case ItemType::SeedCorn: return "Corn Seed";
    case ItemType::SeedTomato: return "Tomato Seed";
    case ItemType::SeedPumpkin: return "Pumpkin Seed";
    case ItemType::SeedBlueberry: return "Blueberry Seed";
    case ItemType::Wood: return "Wood";
    case ItemType::Turnip: return "Turnip";
    case ItemType::Potato: return "Potato";
    case ItemType::Corn: return "Corn";
    case ItemType::Tomato: return "Tomato";
    case ItemType::Pumpkin: return "Pumpkin";
    case ItemType::Blueberry: return "Blueberry";
    case ItemType::Fish: return "Fish";
    // 矿石
    case ItemType::CopperOre: return "Copper Ore";
    case ItemType::SilverOre: return "Silver Ore";
    case ItemType::GoldOre: return "Gold Ore";
    // 武器
    case ItemType::WoodenSword: return "Wooden Sword";
    case ItemType::IronSword: return "Iron Sword";
    case ItemType::GoldSword: return "Gold Sword";
    case ItemType::DiamondSword: return "Diamond Sword";
    case ItemType::Bow: return "Bow";
    default: return "Unknown";
    }
}

std::string InventoryManager::getItemDescription(ItemType itemType)
{
    switch (itemType)
    {
    case ItemType::Hoe: return "Tills soil for planting";
    case ItemType::WateringCan: return "Waters crops";
    case ItemType::Scythe: return "Harvests mature crops";
    case ItemType::Axe: return "Chops down trees";
    case ItemType::Pickaxe: return "Mines rocks";
    case ItemType::FishingRod: return "Catches fish";
    case ItemType::SeedTurnip: return "Grows into turnip";
    case ItemType::SeedPotato: return "Grows into potato";
    case ItemType::SeedCorn: return "Grows into corn";
    case ItemType::SeedTomato: return "Grows into tomato";
    case ItemType::SeedPumpkin: return "Grows into pumpkin";
    case ItemType::SeedBlueberry: return "Grows into blueberry";
    case ItemType::Wood: return "Crafting material";
    case ItemType::Turnip: return "Fresh turnip";
    case ItemType::Potato: return "Fresh potato";
    case ItemType::Corn: return "Fresh corn";
    case ItemType::Tomato: return "Fresh tomato";
    case ItemType::Pumpkin: return "Fresh pumpkin";
    case ItemType::Blueberry: return "Fresh blueberry";
    case ItemType::Fish: return "Fresh fish";
    // 矿石
    case ItemType::CopperOre: return "Common ore, worth 50 gold";
    case ItemType::SilverOre: return "Valuable ore, worth 150 gold";
    case ItemType::GoldOre: return "Precious ore, worth 500 gold";
    // 武器
    case ItemType::WoodenSword: return "Basic sword, 10 attack";
    case ItemType::IronSword: return "Sturdy sword, 25 attack";
    case ItemType::GoldSword: return "Elegant sword, 40 attack";
    case ItemType::DiamondSword: return "Legendary sword, 60 attack";
    case ItemType::Bow: return "Ranged weapon, shoots arrows";
    default: return "";
    }
}

bool InventoryManager::isStackable(ItemType itemType)
{
    // 工具不可堆叠
    if (itemType == ItemType::Hoe || itemType == ItemType::WateringCan ||
        itemType == ItemType::Scythe || itemType == ItemType::Axe ||
        itemType == ItemType::Pickaxe || itemType == ItemType::FishingRod ||
        itemType == ItemType::Bow)
    {
        return false;
    }

    // 武器不可堆叠
    if (itemType == ItemType::WoodenSword || itemType == ItemType::IronSword ||
        itemType == ItemType::GoldSword || itemType == ItemType::DiamondSword)
    {
        return false;
    }

    // 其他物品可堆叠
    return true;
}

int InventoryManager::getMaxStack(ItemType itemType)
{
    // 工具只能1个
    if (!isStackable(itemType))
        return 1;

    // 种子最多99
    if (itemType == ItemType::SeedTurnip || itemType == ItemType::SeedPotato ||
        itemType == ItemType::SeedCorn || itemType == ItemType::SeedTomato ||
        itemType == ItemType::SeedPumpkin || itemType == ItemType::SeedBlueberry)
    {
        return 99;
    }

    // 其他物品最多99
    return 99;
}

void InventoryManager::setSelectedSlotIndex(int index)
{
    if (index >= 0 && index < 10)
    {
        selectedSlotIndex_ = index;
    }
}
