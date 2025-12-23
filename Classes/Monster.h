#ifndef __MONSTER_H__
#define __MONSTER_H__

#include "cocos2d.h"
#include <string>

// 前向声明
class Player;
class MineLayer;

/**
 * @brief 怪物基类
 *
 * 职责：
 * - 怪物的基础属性和行为
 * - AI：追踪玩家、攻击
 * - 受伤和死亡逻辑
 *
 * 注意：
 * - 怪物暂时用白色方块表示
 * - 预留贴图接口，方便后续替换
 */
class Monster : public cocos2d::Sprite
{
public:
    /**
     * @brief 创建怪物
     * @param floorLevel 矿洞层数（影响怪物强度）
     */
    static Monster* create(int floorLevel);

    /**
     * @brief 初始化
     */
    bool init(int floorLevel);

    /**
     * @brief 每帧更新
     */
    virtual void update(float delta) override;

    // ========== 战斗相关 ==========

    /**
     * @brief 受到伤害
     * @param damage 伤害值
     */
    virtual void takeDamage(int damage);

    /**
     * @brief 是否已死亡
     */
    bool isDead() const { return hp_ <= 0; }

    /**
     * @brief 攻击玩家
     */
    virtual void attackPlayer(Player* player);

    // ========== 属性获取 ==========

    int getHp() const { return hp_; }
    int getMaxHp() const { return maxHp_; }
    int getAttackPower() const { return attackPower_; }
    float getAttackRange() const { return attackRange_; }
    std::string getMonsterName() const { return name_; }

    // ========== AI相关 ==========

    /**
     * @brief 设置目标玩家
     */
    void setTargetPlayer(Player* player) { targetPlayer_ = player; }

    /**
     * @brief 设置地图层引用
     */
    void setMapLayer(MineLayer* mapLayer) { mapLayer_ = mapLayer; }

    // ========== 贴图预留接口 ==========

    /**
     * @brief 获取精灵贴图路径（子类重写）
     * 返回空字符串表示使用默认白色方块
     */
    virtual std::string getSpritePath() const { return ""; }

protected:
    // 基础属性
    std::string name_;
    int hp_;
    int maxHp_;
    int attackPower_;
    float moveSpeed_;
    float attackRange_;
    float attackCooldown_;
    float currentAttackCooldown_;

    // 楼层等级（影响属性）
    int floorLevel_;

    // AI相关
    Player* targetPlayer_;
    MineLayer* mapLayer_;

    // 显示相关
    cocos2d::DrawNode* displayNode_;       // 临时显示用
    cocos2d::Label* hpLabel_;              // 血条显示

    /**
     * @brief 初始化属性（子类重写以设置不同属性）
     */
    virtual void initStats();

    /**
     * @brief 初始化显示（临时白色方块）
     */
    virtual void initDisplay();

    /**
     * @brief AI逻辑：追踪玩家
     */
    virtual void updateAI(float delta);

    /**
     * @brief 移动向目标位置
     */
    void moveTowards(const cocos2d::Vec2& targetPos, float delta);

    /**
     * @brief 更新血条显示
     */
    void updateHpDisplay();

    /**
     * @brief 死亡处理
     */
    virtual void onDeath();
};

#endif // __MONSTER_H__
