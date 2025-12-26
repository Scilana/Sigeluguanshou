#include "Weapon.h"

int Weapon::getWeaponAttackPower(ItemType type)
{
    switch (type)
    {
    case ItemType::ITEM_WoodenSword: return 10;
    case ItemType::ITEM_IronSword: return 25;
    case ItemType::ITEM_GoldSword: return 40;
    case ItemType::ITEM_DiamondSword: return 60;
    default: return 0;
    }
}

float Weapon::getWeaponAttackRange(ItemType type)
{
    switch (type)
    {
    case ItemType::ITEM_WoodenSword: return 50.0f;
    case ItemType::ITEM_IronSword: return 55.0f;
    case ItemType::ITEM_GoldSword: return 60.0f;
    case ItemType::ITEM_DiamondSword: return 65.0f;
    default: return 0.0f;
    }
}

float Weapon::getWeaponAttackSpeed(ItemType type)
{
    switch (type)
    {
    case ItemType::ITEM_WoodenSword: return 1.0f;
    case ItemType::ITEM_IronSword: return 0.9f;
    case ItemType::ITEM_GoldSword: return 0.8f;
    case ItemType::ITEM_DiamondSword: return 0.7f;
    default: return 1.0f;
    }
}

int Weapon::getWeaponPrice(ItemType type)
{
    switch (type)
    {
    case ItemType::ITEM_WoodenSword: return 50;
    case ItemType::ITEM_IronSword: return 150;
    case ItemType::ITEM_GoldSword: return 400;
    case ItemType::ITEM_DiamondSword: return 1000;
    default: return 0;
    }
}
