#include "Weapon.h"

int Weapon::getWeaponAttackPower(ItemType type)
{
    switch (type)
    {
    case ItemType::WoodenSword: return 10;
    case ItemType::IronSword: return 25;
    case ItemType::GoldSword: return 40;
    case ItemType::DiamondSword: return 60;
    case ItemType::Bow: return 15;
    default: return 0;
    }
}

float Weapon::getWeaponAttackRange(ItemType type)
{
    switch (type)
    {
    case ItemType::WoodenSword: return 50.0f;
    case ItemType::IronSword: return 55.0f;
    case ItemType::GoldSword: return 60.0f;
    case ItemType::DiamondSword: return 65.0f;
    case ItemType::Bow: return 300.0f;
    default: return 0.0f;
    }
}

float Weapon::getWeaponAttackSpeed(ItemType type)
{
    switch (type)
    {
    case ItemType::WoodenSword: return 1.0f;
    case ItemType::IronSword: return 0.9f;
    case ItemType::GoldSword: return 0.8f;
    case ItemType::DiamondSword: return 0.7f;
    case ItemType::Bow: return 1.2f;
    default: return 1.0f;
    }
}

int Weapon::getWeaponPrice(ItemType type)
{
    switch (type)
    {
    case ItemType::WoodenSword: return 50;
    case ItemType::IronSword: return 150;
    case ItemType::GoldSword: return 400;
    case ItemType::DiamondSword: return 1000;
    case ItemType::Bow: return 300;
    default: return 0;
    }
}
