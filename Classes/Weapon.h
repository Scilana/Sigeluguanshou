#ifndef __WEAPON_H__
#define __WEAPON_H__

#include "cocos2d.h"
#include "InventoryManager.h"
#include <string>

/**
 * @brief 武器基类
 *
 * 职责：
 * - 定义武器的基本属性
 * - 提供攻击功能接口
 * - 预留贴图接口
 */
class Weapon
{
public:
    virtual ~Weapon() = default;

    // ========== 属性获取 ==========

    virtual std::string getName() const = 0;
    virtual int getAttackPower() const = 0;
    virtual float getAttackRange() const = 0;
    virtual float getAttackSpeed() const = 0;
    virtual int getPrice() const = 0;

    /**
     * @brief 获取对应的物品类型
     */
    virtual ItemType getItemType() const = 0;

    /**
     * @brief 获取贴图路径（预留）
     */
    virtual std::string getSpritePath() const { return ""; }

    /**
     * @brief 根据物品类型获取武器数据
     */
    static int getWeaponAttackPower(ItemType type);
    static float getWeaponAttackRange(ItemType type);
    static float getWeaponAttackSpeed(ItemType type);
    static int getWeaponPrice(ItemType type);
};

/**
 * @brief 木剑
 */
class WoodenSword : public Weapon
{
public:
    std::string getName() const override { return "Wooden Sword"; }
    int getAttackPower() const override { return 10; }
    float getAttackRange() const override { return 50.0f; }
    float getAttackSpeed() const override { return 1.0f; }
    int getPrice() const override { return 50; }
    ItemType getItemType() const override { return ItemType::WoodenSword; }
    std::string getSpritePath() const override { return "weapons/wooden_sword.png"; }
};

/**
 * @brief 铁剑
 */
class IronSword : public Weapon
{
public:
    std::string getName() const override { return "Iron Sword"; }
    int getAttackPower() const override { return 25; }
    float getAttackRange() const override { return 55.0f; }
    float getAttackSpeed() const override { return 0.9f; }
    int getPrice() const override { return 150; }
    ItemType getItemType() const override { return ItemType::IronSword; }
    std::string getSpritePath() const override { return "weapons/iron_sword.png"; }
};

/**
 * @brief 金剑
 */
class GoldSword : public Weapon
{
public:
    std::string getName() const override { return "Gold Sword"; }
    int getAttackPower() const override { return 40; }
    float getAttackRange() const override { return 60.0f; }
    float getAttackSpeed() const override { return 0.8f; }
    int getPrice() const override { return 400; }
    ItemType getItemType() const override { return ItemType::GoldSword; }
    std::string getSpritePath() const override { return "weapons/gold_sword.png"; }
};

/**
 * @brief 钻石剑
 */
class DiamondSword : public Weapon
{
public:
    std::string getName() const override { return "Diamond Sword"; }
    int getAttackPower() const override { return 60; }
    float getAttackRange() const override { return 65.0f; }
    float getAttackSpeed() const override { return 0.7f; }
    int getPrice() const override { return 1000; }
    ItemType getItemType() const override { return ItemType::DiamondSword; }
    std::string getSpritePath() const override { return "weapons/diamond_sword.png"; }
};

#endif // __WEAPON_H__
