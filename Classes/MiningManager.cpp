#include "MiningManager.h"
#include "SkillManager.h"
#include <algorithm>

USING_NS_CC;

MiningManager* MiningManager::create(MineLayer* mineLayer, InventoryManager* inventory)
{
    MiningManager* ret = new (std::nothrow) MiningManager();
    if (ret && ret->init(mineLayer, inventory))
    {
        ret->autorelease();
        return ret;
    }
    else
    {
        delete ret;
        ret = nullptr;
        return nullptr;
    }
}

bool MiningManager::init(MineLayer* mineLayer, InventoryManager* inventory)
{
    if (!Node::init())
        return false;

    mineLayer_ = mineLayer;
    inventory_ = inventory;
    miningExp_ = 0;

    if (!mineLayer_ || !inventory_)
    {
        CCLOG("ERROR: MiningManager requires valid MineLayer and InventoryManager!");
        return false;
    }

    // 初始化矿物定义
    initMineralDefs();

    CCLOG("MiningManager initialized");
    return true;
}

void MiningManager::initMineralDefs()
{
    // 注意：这里的 GID 需要根据你的 tileset 实际情况调整
    // 这里使用示例 GID，你需要在 Tiled 中查看实际的 GID 值

    // 石头（普通）- GID 示例：1001
    mineralDefs_[1001] = MineralDef{
        1001,
        "Stone",
        1,                      // 1次敲击
        ItemType::ITEM_NONE,         // 不掉落物品
        0, 0,
        1                       // 1点经验
    };

    // 铜矿石 - GID 示例：1002, 521-555 (mine_minerals.tsx 的铜矿)
    // GID 521-555 范围内的都是铜矿
    for (int gid = 521; gid <= 555; ++gid) {
        mineralDefs_[gid] = MineralDef{
            gid,
            "Copper Ore",
            3,                      // 3次敲击
            ItemType::CopperOre,    // 掉落铜矿石
            1, 3,                   // 掉落1-3个
            5                       // 5点经验
        };
    }

    // 银矿石 - GID 示例：574-608
    for (int gid = 574; gid <= 608; ++gid) {
        mineralDefs_[gid] = MineralDef{
            gid,
            "Silver Ore",
            5,                      // 5次敲击
            ItemType::SilverOre,    // 掉落银矿石
            1, 2,                   // 掉落1-2个
            10                      // 10点经验
        };
    }

    // 金矿石 - GID 示例：649-723
    for (int gid = 649; gid <= 723; ++gid) {
        mineralDefs_[gid] = MineralDef{
            gid,
            "Gold Ore",
            8,                      // 8次敲击
            ItemType::GoldOre,      // 掉落金矿石
            1, 1,                   // 掉落1个
            20                      // 20点经验
        };
    }

    // [New] 铁矿石 - 假设 GID 范围 (用户需在 Tiled 中对应)
    // 假设 750-770 为铁矿
    for (int gid = 750; gid <= 770; ++gid) {
        mineralDefs_[gid] = MineralDef{
            gid,
            "Iron Ore",
            4,                      // 4次敲击
            ItemType::IronOre,      // 掉落铁矿石
            1, 2,                   // 掉落1-2个
            8                       // 8点经验
        };
    }

    // [New] 钻石矿 - 假设 GID 范围 
    // 假设 800-810 为钻石矿
    for (int gid = 800; gid <= 810; ++gid) {
        mineralDefs_[gid] = MineralDef{
            gid,
            "Diamond Ore",
            10,                     // 10次敲击 (非常硬)
            ItemType::DiamondOre,   // 掉落钻石
            1, 1,                   // 掉落1个
            50                      // 50点经验
        };
    }

    CCLOG("Mineral definitions initialized: %d types", (int)mineralDefs_.size());
}

MiningManager::MiningResult MiningManager::mineTile(const Vec2& tileCoord)
{
    // 检查是否有矿物
    if (!mineLayer_->isMineralAt(tileCoord))
    {
        return { false, "No mineral here" };
    }

    // 获取矿物 GID
    int gid = mineLayer_->getMineralGID(tileCoord);

    // 查找矿物定义
    const MineralDef* mineralDef = getMineralDef(gid);
    if (!mineralDef)
    {
        // 如果没有定义，使用默认值（1次敲击，不掉落）
        CCLOG("Unknown mineral GID: %d, using default", gid);
        mineLayer_->clearMineralAt(tileCoord);
        mineLayer_->clearCollisionAt(tileCoord);
        return { true, "" }; // 不提示通用消息
    }

    // 生成瓦片键
    long long key = getTileKey(tileCoord);

    // 查找或创建进度
    auto it = activeMinings_.find(key);
    if (it == activeMinings_.end())
    {
        // 新矿物，创建进度
        MineralProgress progress;
        progress.tileCoord = tileCoord;
        progress.hitCount = 1;
        int hitReduction = SkillManager::getInstance()->getMiningHitReduction();
        progress.requiredHits = std::max(1, mineralDef->hitPoints - hitReduction);
        activeMinings_[key] = progress;

        if (progress.requiredHits <= 1)
        {
            // 一次就破坏
            mineLayer_->clearMineralAt(tileCoord);
            mineLayer_->clearCollisionAt(tileCoord);
            std::string dropMsg = dropItems(tileCoord, *mineralDef);
            activeMinings_.erase(key);
            SkillManager::getInstance()->recordAction(SkillManager::SkillType::Mining);
            addExp(mineralDef->expReward);
            
            if (!dropMsg.empty()) {
                return { true, dropMsg };
            } else {
                return { true, "" };
            }
        }
        else
        {
            return { true, "Mining... (" + std::to_string(progress.requiredHits - 1) + " more)" };
        }
    }
    else
    {
        // 继续敲击
        it->second.hitCount++;

        if (it->second.hitCount >= it->second.requiredHits)
        {
            // 破坏矿物
            mineLayer_->clearMineralAt(tileCoord);
            mineLayer_->clearCollisionAt(tileCoord);
            std::string dropMsg = dropItems(tileCoord, *mineralDef);
            activeMinings_.erase(it);
            SkillManager::getInstance()->recordAction(SkillManager::SkillType::Mining);
            addExp(mineralDef->expReward);
            
            if (!dropMsg.empty()) {
                return { true, dropMsg };
            } else {
                return { true, "" };
            }
        }
        else
        {
            int remaining = it->second.requiredHits - it->second.hitCount;
            return { true, "Mining... (" + std::to_string(remaining) + " more)" };
        }
    }
}

const MiningManager::MineralDef* MiningManager::getMineralDef(int gid) const
{
    auto it = mineralDefs_.find(gid);
    if (it != mineralDefs_.end())
    {
        return &it->second;
    }
    return nullptr;
}

void MiningManager::addExp(int exp)
{
    miningExp_ += exp;
    CCLOG("Mining exp: %d (+%d)", miningExp_, exp);
}

long long MiningManager::getTileKey(const Vec2& tileCoord) const
{
    int x = static_cast<int>(tileCoord.x);
    int y = static_cast<int>(tileCoord.y);
    return (static_cast<long long>(y) << 32) | (static_cast<unsigned long long>(x) & 0xffffffffULL);
}

std::string MiningManager::dropItems(const Vec2& tileCoord, const MineralDef& mineralDef)
{
    if (mineralDef.dropItem == ItemType::ITEM_NONE)
        return "";

    // 计算掉落数量
    int dropCount = mineralDef.dropMinCount;
    if (mineralDef.dropMaxCount > mineralDef.dropMinCount)
    {
        dropCount += rand() % (mineralDef.dropMaxCount - mineralDef.dropMinCount + 1);
    }

    // 添加到背包
    if (inventory_)
    {
        bool added = inventory_->addItem(mineralDef.dropItem, dropCount);
        if (added)
        {
            std::string itemName = InventoryManager::getItemName(mineralDef.dropItem);
            CCLOG("Collected %d x %s", dropCount, itemName.c_str());
            return StringUtils::format("Found %d %s!", dropCount, itemName.c_str());
        }
        else
        {
            CCLOG("Inventory full! Could not collect items");
            return "Inventory full!";
        }
    }
    return "";
}
