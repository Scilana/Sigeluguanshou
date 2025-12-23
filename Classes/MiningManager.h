#ifndef __MINING_MANAGER_H__
#define __MINING_MANAGER_H__

#include "cocos2d.h"
#include "MineLayer.h"
#include "InventoryManager.h"
#include <string>
#include <unordered_map>

/**
 * @brief 挖矿管理器
 *
 * 职责：
 * - 管理矿物数据
 * - 处理挖矿逻辑
 * - 掉落物品到背包
 */
class MiningManager : public cocos2d::Node
{
public:
    /**
     * @brief 矿物定义
     */
    struct MineralDef
    {
        int id;                    // 矿物ID
        std::string name;          // 名称
        int hitPoints;             // 需要敲击次数
        ItemType dropItem;         // 掉落物品类型
        int dropMinCount;          // 最小掉落数量
        int dropMaxCount;          // 最大掉落数量
        int expReward;             // 经验奖励
    };

    /**
     * @brief 操作结果
     */
    struct MiningResult
    {
        bool success;
        std::string message;
    };

    /**
     * @brief 创建挖矿管理器
     * @param mineLayer 矿洞地图层引用
     * @param inventory 背包管理器引用
     */
    static MiningManager* create(MineLayer* mineLayer, InventoryManager* inventory);

    /**
     * @brief 初始化
     */
    bool init(MineLayer* mineLayer, InventoryManager* inventory);

    /**
     * @brief 挖掘矿物
     * @param tileCoord 瓦片坐标
     */
    MiningResult mineTile(const cocos2d::Vec2& tileCoord);

    /**
     * @brief 获取矿物信息
     * @param gid 矿物 GID
     */
    const MineralDef* getMineralDef(int gid) const;

    /**
     * @brief 获取玩家挖矿经验
     */
    int getMiningExp() const { return miningExp_; }

    /**
     * @brief 添加经验
     */
    void addExp(int exp);

private:
    MineLayer* mineLayer_;                           // 矿洞地图层引用
    InventoryManager* inventory_;                    // 背包管理器引用
    int miningExp_;                                  // 挖矿经验

    // 矿物进度追踪
    struct MineralProgress
    {
        cocos2d::Vec2 tileCoord;
        int hitCount;              // 已敲击次数
        int requiredHits;          // 需要的总次数
    };

    std::unordered_map<long long, MineralProgress> activeMinings_;  // 正在挖掘的矿物

    // 矿物定义数据库
    std::unordered_map<int, MineralDef> mineralDefs_;

    /**
     * @brief 初始化矿物定义
     */
    void initMineralDefs();

    /**
     * @brief 生成哈希键（用于追踪矿物位置）
     */
    long long getTileKey(const cocos2d::Vec2& tileCoord) const;

    /**
     * @brief 掉落物品
     */
    void dropItems(const cocos2d::Vec2& tileCoord, const MineralDef& mineralDef);
};

#endif // __MINING_MANAGER_H__
