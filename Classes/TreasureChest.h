#ifndef __TREASURE_CHEST_H__
#define __TREASURE_CHEST_H__

#include "cocos2d.h"
#include "InventoryManager.h"
#include <vector>

/**
 * @brief 宝箱类
 *
 * 职责：
 * - 在矿洞中生成宝箱
 * - 根据楼层等级决定掉落物品
 * - 处理开启逻辑
 */
class TreasureChest : public cocos2d::Node
{
public:
    /**
     * @brief 掉落结果
     */
    struct LootResult
    {
        ItemType item;
        int count;
        std::string message;
    };

    /**
     * @brief 创建宝箱
     * @param floorLevel 矿洞层数（影响掉落质量）
     */
    static TreasureChest* create(int floorLevel);

    /**
     * @brief 初始化
     */
    bool init(int floorLevel);

    /**
     * @brief 开启宝箱
     * @return 掉落结果
     */
    LootResult open();

    /**
     * @brief 是否已开启
     */
    bool isOpened() const { return isOpened_; }

    /**
     * @brief 获取楼层等级
     */
    int getFloorLevel() const { return floorLevel_; }

private:
    int floorLevel_;
    bool isOpened_;
    cocos2d::DrawNode* displayNode_;
    cocos2d::Label* label_;

    /**
     * @brief 初始化显示
     */
    void initDisplay();

    /**
     * @brief 生成掉落物品
     */
    LootResult generateLoot();

    /**
     * @brief 播放开启动画
     */
    void playOpenAnimation();
};

#endif // __TREASURE_CHEST_H__
