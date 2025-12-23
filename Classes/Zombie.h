#ifndef __ZOMBIE_H__
#define __ZOMBIE_H__

#include "Monster.h"

/**
 * @brief 僵尸怪物
 *
 * 特点：
 * - 高血量，高攻击力
 * - 移动速度较慢
 * - 常见于深层矿洞
 */
class Zombie : public Monster
{
public:
    /**
     * @brief 创建僵尸
     * @param floorLevel 矿洞层数
     */
    static Zombie* create(int floorLevel);

    /**
     * @brief 初始化
     */
    bool init(int floorLevel);

    /**
     * @brief 获取贴图路径（预留）
     */
    std::string getSpritePath() const override { return "monsters/zombie.png"; }

protected:
    /**
     * @brief 初始化僵尸属性
     */
    void initStats() override;

    /**
     * @brief 初始化显示（灰色方块）
     */
    void initDisplay() override;
};

#endif // __ZOMBIE_H__
