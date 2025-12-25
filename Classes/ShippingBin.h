#ifndef __SHIPPING_BIN_H__
#define __SHIPPING_BIN_H__

#include "StorageChest.h"

/**
 * @brief 交易箱类
 * 继承自 StorageChest，但具有不同的外观和标识。
 * 每天结束时，FarmManager 会专门处理其中的物品进行结算。
 */
class ShippingBin : public StorageChest
{
public:
    static ShippingBin* create(const cocos2d::Vec2& tileCoord);
    
    virtual bool init(const cocos2d::Vec2& tileCoord) override;
    
    // 标识自己是交易箱
    virtual bool isShippingBin() const override { return true; }
    
    // 重写外观初始化
    virtual void initDisplay() override;
};

#endif // __SHIPPING_BIN_H__
