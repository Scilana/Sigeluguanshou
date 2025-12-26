#include "Fish.h"

Fish* Fish::createByType(ItemType type) {
    switch (type) {
        case ItemType::ITEM_Anchovy: return new AnchovyFish();
        case ItemType::ITEM_Carp: return new CarpFish();
        case ItemType::ITEM_Eel: return new EelFish();
        case ItemType::ITEM_Flounder: return new FlounderFish();
        case ItemType::ITEM_Largemouth_Bass: return new LargemouthBassFish();
        case ItemType::ITEM_Pufferfish: return new PufferfishFish();
        case ItemType::ITEM_Rainbow_Trout: return new RainbowTroutFish();
        case ItemType::ITEM_Sturgeon: return new SturgeonFish();
        case ItemType::ITEM_Tilapia: return new TilapiaFish();
        default: return nullptr;
    }
}
