#include "InventoryManager.h"

USING_NS_CC;

static InventoryManager* s_instance = nullptr;

InventoryManager* InventoryManager::create(int slotCount)
{
    InventoryManager* ret = new (std::nothrow) InventoryManager();
    if (ret && ret->init(slotCount))
    {
        ret->autorelease();
        return ret;
    }
    CC_SAFE_DELETE(ret);
    return nullptr;
}

InventoryManager* InventoryManager::getInstance()
{
    if (!s_instance)
    {
        s_instance = new (std::nothrow) InventoryManager();
        if (s_instance && s_instance->init(30))
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

bool InventoryManager::init(int slotCount)
{
    if (!Node::init())
        return false;

    // 初始化所有槽位
    slots_.clear();
    slots_.resize(slotCount);
    for (auto& slot : slots_)
    {
        slot.clear();
    }

    // 初始化默认金币和物品
    money_ = 500;
    selectedSlotIndex_ = 0;
    
    // 只有主背包（30格）才初始化默认物品
    if (slotCount == 30) {
        initDefaultItems();
    }

    CCLOG("InventoryManager initialized with %d slots and %d gold", (int)slots_.size(), money_);

    return true;
}

void InventoryManager::initDefaultItems()
{
    // 初始工具（前5个槽位），设置满耐久
    setSlot(0, ItemType::Hoe, 1);
    setSlot(1, ItemType::WateringCan, 1);
    setSlot(2, ItemType::Scythe, 1);
    setSlot(3, ItemType::Axe, 1);
    setSlot(4, ItemType::Pickaxe, 1);
    setSlot(5, ItemType::FishingRod, 1);
    setSlot(6, ItemType::ITEM_WoodenSword, 1); // Added Wooden Sword

    // 为工具设置初始满耐久
    for (int i = 0; i <= 6; ++i) { // Adjusted loop to include the new sword
        slots_[i].maxDurability = getDefaultMaxDurability(slots_[i].type);
        slots_[i].durability = slots_[i].maxDurability;
    }

    // 初始种子
    setSlot(7, ItemType::SeedTurnip, 10);
    setSlot(8, ItemType::SeedPotato, 10);
    setSlot(9, ItemType::SeedCorn, 5);
    setSlot(10, ItemType::SeedTomato, 5);
    setSlot(11, ItemType::SeedPumpkin, 3);
    setSlot(12, ItemType::SeedBlueberry, 3);

    // 初始木材
    setSlot(13, ItemType::Wood, 20);
}

bool InventoryManager::addItem(ItemType itemType, int count)
{
    if (itemType == ItemType::ITEM_NONE || count <= 0)
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

bool InventoryManager::removeItem(ItemType itemType, int count /*= 1*/)
{
    if (itemType == ItemType::ITEM_NONE || count <= 0)
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

bool InventoryManager::removeItemFromSlot(int slotIndex, int count)
{
    if (slotIndex < 0 || slotIndex >= (int)slots_.size() || count <= 0)
        return false;

    auto& slot = slots_[slotIndex];
    if (slot.isEmpty() || slot.count < count)
        return false;

    slot.count -= count;
    if (slot.count <= 0)
    {
        slot.clear();
    }
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
            total += slot.count;
    }
    return total;
}

const InventoryManager::ItemSlot& InventoryManager::getSlot(int slotIndex) const
{
    static ItemSlot emptySlot;
    if (slotIndex < 0 || slotIndex >= (int)slots_.size())
        return emptySlot;
    return slots_[slotIndex];
}

void InventoryManager::setSlot(int slotIndex, ItemType itemType, int count)
{
    if (slotIndex < 0 || slotIndex >= (int)slots_.size())
        return;

    slots_[slotIndex].type = itemType;
    slots_[slotIndex].count = count;
    
    // 如果是工具，初始化耐久度
    if (isTool(itemType)) {
        slots_[slotIndex].maxDurability = getDefaultMaxDurability(itemType);
        slots_[slotIndex].durability = slots_[slotIndex].maxDurability;
    } else {
        slots_[slotIndex].maxDurability = -1;
        slots_[slotIndex].durability = -1;
    }
}

void InventoryManager::swapSlots(int index1, int index2)
{
    if (index1 < 0 || index1 >= (int)slots_.size() || index2 < 0 || index2 >= (int)slots_.size())
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

bool InventoryManager::decreaseDurability(int slotIndex, int amount)
{
    if (slotIndex < 0 || slotIndex >= (int)slots_.size())
        return false;

    auto& slot = slots_[slotIndex];
    if (slot.isEmpty()) return false;

    // Check if it should be a tool but has invalid durability (Legacy Save Fix)
    if (!slot.isTool()) {
        int defMax = getDefaultMaxDurability(slot.type);
        if (defMax > 0) {
            slot.maxDurability = defMax;
            slot.durability = defMax;
            CCLOG("Restored legacy tool durability for %s", getItemName(slot.type).c_str());
        } else {
            return false; // Not a tool
        }
    }

    slot.durability -= amount;
    CCLOG("Tool %s durability: %d/%d", getItemName(slot.type).c_str(), slot.durability, slot.maxDurability);

    if (slot.durability <= 0)
    {
        CCLOG("Tool %s broke!", getItemName(slot.type).c_str());
        slot.clear();
        return true; // Broke
    }
    return false;
}

bool InventoryManager::repairSlot(int slotIndex)
{
    if (slotIndex < 0 || slotIndex >= (int)slots_.size())
        return false;

    auto& slot = slots_[slotIndex];
    if (slot.isEmpty() || !slot.isTool())
        return false;

    slot.durability = slot.maxDurability;
    CCLOG("Repaired tool in slot %d", slotIndex);
    return true;
}

int InventoryManager::getDefaultMaxDurability(ItemType type)
{
    switch (type)
    {
    case ItemType::Hoe:
    case ItemType::WateringCan:
    case ItemType::Scythe:
    case ItemType::Axe:
    case ItemType::Pickaxe:
    case ItemType::FishingRod:
        return 50; // 初始工具 50 次耐久
    case ItemType::ITEM_WoodenSword:
        return 100;
    case ItemType::ITEM_IronSword:
        return 200;
    case ItemType::ITEM_GoldSword:
        return 300;
    case ItemType::ITEM_DiamondSword:
        return 1000;
    default:
        return -1;
    }
}

bool InventoryManager::isTool(ItemType type)
{
    return getDefaultMaxDurability(type) != -1;
}

int InventoryManager::findEmptySlot() const
{
    for (int i = 0; i < (int)slots_.size(); ++i)
    {
        if (slots_[i].isEmpty())
            return i;
    }
    return -1;
}

int InventoryManager::findItemSlot(ItemType itemType) const
{
    for (int i = 0; i < (int)slots_.size(); ++i)
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
    case ItemType::ITEM_NONE: return "Empty";
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
    case ItemType::IronOre: return "Iron Ore";
    case ItemType::SilverOre: return "Silver Ore";
    case ItemType::GoldOre: return "Gold Ore";
    case ItemType::DiamondOre: return "Diamond Ore";
    // 武器
    case ItemType::ITEM_WoodenSword: return "Wooden Sword";
    case ItemType::ITEM_IronSword: return "Iron Sword";
    case ItemType::ITEM_GoldSword: return "Gold Sword";
    case ItemType::ITEM_DiamondSword: return "Diamond Sword";
    // 鱼类
            case ItemType::ITEM_Anchovy: return "Anchovy";
            case ItemType::ITEM_Carp: return "Carp";
            case ItemType::ITEM_Eel: return "Eel";
            case ItemType::ITEM_Flounder: return "Flounder";
            case ItemType::ITEM_Largemouth_Bass: return "Largemouth Bass";
            case ItemType::ITEM_Pufferfish: return "Pufferfish";
            case ItemType::ITEM_Rainbow_Trout: return "Rainbow Trout";
            case ItemType::ITEM_Sturgeon: return "Sturgeon";
            case ItemType::ITEM_Tilapia: return "Tilapia";
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
    case ItemType::IronOre: return "Sturdy ore, worth 100 gold";
    case ItemType::SilverOre: return "Valuable ore, worth 150 gold";
    case ItemType::GoldOre: return "Precious ore, worth 500 gold";
    case ItemType::DiamondOre: return "Rare gem, worth 1000 gold";
    // 武器
    case ItemType::ITEM_WoodenSword: return "Basic sword, 10 attack";
    case ItemType::ITEM_IronSword: return "Sturdy sword, 25 attack";
    case ItemType::ITEM_GoldSword: return "Elegant sword, 40 attack";
    case ItemType::ITEM_DiamondSword: return "Legendary sword, 60 attack";
    // 鱼类
            case ItemType::ITEM_Anchovy: return "A small saltwater fish.";
            case ItemType::ITEM_Carp: return "A common pond fish.";
            case ItemType::ITEM_Eel: return "A long, slippery fish.";
            case ItemType::ITEM_Flounder: return "A flat sea fish.";
            case ItemType::ITEM_Largemouth_Bass: return "A popular freshwater game fish.";
            case ItemType::ITEM_Pufferfish: return "Can inflate itself when threatened.";
            case ItemType::ITEM_Rainbow_Trout: return "A colorful freshwater fish.";
            case ItemType::ITEM_Sturgeon: return "An ancient, valuable fish.";
            case ItemType::ITEM_Tilapia: return "A common tropical fish.";
    default: return "";
    }
}

std::string InventoryManager::getItemIconPath(ItemType itemType)
{
    switch (itemType)
    {
    case ItemType::Hoe: return "tools/hoe.png";
    case ItemType::WateringCan: return "tools/kettle.png";
    case ItemType::Scythe: return "tools/scythe.png";
    case ItemType::Axe: return "tools/axe.png";
    case ItemType::Pickaxe: return "tools/pickaxe.png";
    case ItemType::Fish: return "tools/fish.png";
    // 鱼类
        case ItemType::ITEM_Anchovy: return "fish/Anchovy.png";
        case ItemType::ITEM_Carp: return "fish/Carp.png";
        case ItemType::ITEM_Eel: return "fish/Eel.png";
        case ItemType::ITEM_Flounder: return "fish/Flounder.png";
        case ItemType::ITEM_Largemouth_Bass: return "fish/Largemouth_Bass.png";
        case ItemType::ITEM_Pufferfish: return "fish/Pufferfish.png";
        case ItemType::ITEM_Rainbow_Trout: return "fish/Rainbow_Trout.png";
        case ItemType::ITEM_Sturgeon: return "fish/Sturgeon.png";
        case ItemType::ITEM_Tilapia: return "fish/Tilapia.png";
    case ItemType::CopperOre: return "";
    case ItemType::IronOre: return "";
    case ItemType::SilverOre: return "";
    case ItemType::GoldOre: return "";
    case ItemType::DiamondOre: return "";
    case ItemType::FishingRod: return "tools/fishingRod.png";
    case ItemType::SeedTurnip: return "tools/carrotSeed.png";
    case ItemType::SeedPotato: return "tools/dogbaneSeed.png";
    case ItemType::SeedCorn: return "tools/cornSeed.png";
    case ItemType::SeedTomato: return "tools/carrotSeed.png";
    case ItemType::SeedPumpkin: return "tools/dogbaneSeed.png";
    case ItemType::SeedBlueberry: return "tools/cornSeed.png";
    default: return "";
    }
}

bool InventoryManager::isStackable(ItemType itemType)
{
    // 工具不可堆叠
    if (itemType == ItemType::Hoe || itemType == ItemType::WateringCan ||
        itemType == ItemType::Scythe || itemType == ItemType::Axe ||
        itemType == ItemType::Pickaxe || itemType == ItemType::FishingRod)
    {
        return false;
    }

    // 武器不可堆叠
    if (itemType == ItemType::ITEM_WoodenSword || itemType == ItemType::ITEM_IronSword ||
        itemType == ItemType::ITEM_GoldSword || itemType == ItemType::ITEM_DiamondSword)
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

    // 其他物品最多 64 个
    return 64;
}

int InventoryManager::getSelectedSlotIndex() const
{
    return selectedSlotIndex_;
}

void InventoryManager::setSelectedSlotIndex(int index)
{
    selectedSlotIndex_ = index;
}
