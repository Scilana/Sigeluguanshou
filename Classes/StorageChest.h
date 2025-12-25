#ifndef __STORAGE_CHEST_H__
#define __STORAGE_CHEST_H__

#include "cocos2d.h"
#include "InventoryManager.h"

/**
 * @brief 储物箱类
 * 
 * 职责：
 * - 维护自己的 InventoryManager (32格)
 * - 记录地理坐标 (TileCoord)
 * - 处理交互
 */
class StorageChest : public cocos2d::Node
{
public:
    static StorageChest* create(const cocos2d::Vec2& tileCoord);
    
    virtual bool init(const cocos2d::Vec2& tileCoord);
    
    InventoryManager* getInventory() const { return inventory_; }
    cocos2d::Vec2 getTileCoord() const { return tileCoord_; }
    
    virtual bool isShippingBin() const { return false; }

    /**
     * @brief 视觉表现相关
     */
    virtual void initDisplay();

protected:
    cocos2d::Vec2 tileCoord_;
    InventoryManager* inventory_;
    cocos2d::DrawNode* displayNode_;
};

#endif // __STORAGE_CHEST_H__
