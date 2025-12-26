#ifndef __FISH_H__
#define __FISH_H__

#include "InventoryManager.h"
#include "MarketState.h"
#include <vector>
#include <string>

class Fish {
public:
    virtual ~Fish() {}
    virtual ItemType getType() const = 0;
    virtual std::string getName() const = 0;
    virtual int getBasePrice() const = 0;
    virtual float getDifficulty() const = 0; // 0.1 (easy) to 1.0 (hard)
    virtual float getMovementFrequency() const = 0; // How fast it changes target
    virtual bool isSaltwater() const = 0;
    
    // 环境检查
    virtual bool canSpawn(int hour, int season, int weather) const {
        return true; // 默认全天候
    }

    static Fish* createByType(ItemType type);
};

// 鳀鱼 - Anchovy (海水, 简单)
class AnchovyFish : public Fish {
public:
    ItemType getType() const override { return ItemType::ITEM_Anchovy; }
    std::string getName() const override { return "Anchovy"; }
    int getBasePrice() const override { return 30; }
    float getDifficulty() const override { return 0.2f; }
    float getMovementFrequency() const override { return 1.5f; }
    bool isSaltwater() const override { return true; }
};

// 鲤鱼 - Carp (淡水, 极易)
class CarpFish : public Fish {
public:
    ItemType getType() const override { return ItemType::ITEM_Carp; }
    std::string getName() const override { return "Carp"; }
    int getBasePrice() const override { return 30; }
    float getDifficulty() const override { return 0.1f; }
    float getMovementFrequency() const override { return 2.0f; }
    bool isSaltwater() const override { return false; }
};

// 鳗鱼 - Eel (海水/淡水雨夜, 困难)
class EelFish : public Fish {
public:
    ItemType getType() const override { return ItemType::ITEM_Eel; }
    std::string getName() const override { return "Eel"; }
    int getBasePrice() const override { return 85; }
    float getDifficulty() const override { return 0.7f; }
    float getMovementFrequency() const override { return 0.8f; }
    bool isSaltwater() const override { return true; }
    bool canSpawn(int hour, int season, int weather) const override {
        // 典型雨天或深夜
        return (weather == (int)MarketState::Weather::HeavyRain || hour >= 18 || hour <= 6);
    }
};

// 比目鱼 - Flounder (海水, 中等)
class FlounderFish : public Fish {
public:
    ItemType getType() const override { return ItemType::ITEM_Flounder; }
    std::string getName() const override { return "Flounder"; }
    int getBasePrice() const override { return 50; }
    float getDifficulty() const override { return 0.4f; }
    float getMovementFrequency() const override { return 1.2f; }
    bool isSaltwater() const override { return true; }
};

// 大口黑鲈 - Largemouth Bass (淡水, 中等)
class LargemouthBassFish : public Fish {
public:
    ItemType getType() const override { return ItemType::ITEM_Largemouth_Bass; }
    std::string getName() const override { return "Largemouth Bass"; }
    int getBasePrice() const override { return 100; }
    float getDifficulty() const override { return 0.5f; }
    float getMovementFrequency() const override { return 1.0f; }
    bool isSaltwater() const override { return false; }
};

// 河豚 - Pufferfish (海水晴午, 极难)
class PufferfishFish : public Fish {
public:
    ItemType getType() const override { return ItemType::ITEM_Pufferfish; }
    std::string getName() const override { return "Pufferfish"; }
    int getBasePrice() const override { return 200; }
    float getDifficulty() const override { return 0.9f; }
    float getMovementFrequency() const override { return 0.5f; }
    bool isSaltwater() const override { return true; }
    bool canSpawn(int hour, int season, int weather) const override {
        return (weather == (int)MarketState::Weather::Sunny && hour >= 12 && hour <= 16);
    }
};

// 虹鳟鱼 - Rainbow Trout (淡水晴, 困难)
class RainbowTroutFish : public Fish {
public:
    ItemType getType() const override { return ItemType::ITEM_Rainbow_Trout; }
    std::string getName() const override { return "Rainbow Trout"; }
    int getBasePrice() const override { return 65; }
    float getDifficulty() const override { return 0.6f; }
    float getMovementFrequency() const override { return 0.9f; }
    bool isSaltwater() const override { return false; }
    bool canSpawn(int hour, int season, int weather) const override {
        return (weather == (int)MarketState::Weather::Sunny);
    }
};

// 鲟鱼 - Sturgeon (湖泊/海水冬, 史诗)
class SturgeonFish : public Fish {
public:
    ItemType getType() const override { return ItemType::ITEM_Sturgeon; }
    std::string getName() const override { return "Sturgeon"; }
    int getBasePrice() const override { return 200; }
    float getDifficulty() const override { return 1.0f; }
    float getMovementFrequency() const override { return 0.4f; }
    bool isSaltwater() const override { return true; } 
};

// 罗非鱼 - Tilapia (海水, 中等)
class TilapiaFish : public Fish {
public:
    ItemType getType() const override { return ItemType::ITEM_Tilapia; }
    std::string getName() const override { return "Tilapia"; }
    int getBasePrice() const override { return 75; }
    float getDifficulty() const override { return 0.4f; }
    float getMovementFrequency() const override { return 1.3f; }
    bool isSaltwater() const override { return true; }
};

#endif // __FISH_H__
