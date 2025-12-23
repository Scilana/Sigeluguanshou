#ifndef __SLIME_H__
#define __SLIME_H__

#include "Monster.h"

/**
 * @brief 史莱姆怪物
 *
 * 特点：
 * - 低血量，低攻击力
 * - 移动速度较快
 * - 常见于浅层矿洞
 */
class Slime : public Monster
{
public:
    /**
     * @brief 创建史莱姆
     * @param floorLevel 矿洞层数
     */
    static Slime* create(int floorLevel);

    /**
     * @brief 初始化
     */
    bool init(int floorLevel);

    /**
     * @brief 获取贴图路径（预留）
     */
    std::string getSpritePath() const override { return "monsters/slime.png"; }

protected:
    /**
     * @brief 初始化史莱姆属性
     */
    void initStats() override;

    /**
     * @brief 初始化显示（绿色方块）
     */
    void initDisplay() override;
};

#endif // __SLIME_H__
