#ifndef __SHIPPING_BIN_H__
#define __SHIPPING_BIN_H__

#include "StorageChest.h"

/**
 * @brief 交易箱类
 * 继承自储物箱类，但外观与标识不同。
 * 每天结束时由农场管理器处理其中物品并结算。
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

#endif 
