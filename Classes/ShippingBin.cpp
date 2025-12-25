#include "ShippingBin.h"

USING_NS_CC;

ShippingBin* ShippingBin::create(const Vec2& tileCoord)
{
    ShippingBin* ret = new (std::nothrow) ShippingBin();
    if (ret && ret->init(tileCoord))
    {
        ret->autorelease();
        return ret;
    }
    CC_SAFE_DELETE(ret);
    return nullptr;
}

bool ShippingBin::init(const Vec2& tileCoord)
{
    // 调用父类 init (创建背包等)
    if (!StorageChest::init(tileCoord))
        return false;
    
    return true;
}

void ShippingBin::initDisplay()
{
    displayNode_ = DrawNode::create();

    // 绘制箱体 (底座) - 红色系，区分于普通储物箱
    Vec2 bodyVertices[] = {
        Vec2(-14, -10),
        Vec2(14, -10),
        Vec2(14, 6),
        Vec2(-14, 6)
    };
    // 红色箱体
    Color4F bodyColor(0.8f, 0.2f, 0.2f, 1.0f); 
    displayNode_->drawPolygon(bodyVertices, 4, bodyColor, 1, Color4F(0.4f, 0.1f, 0.1f, 1.0f));

    // 绘制盖子 - 深红色
    Vec2 lidVertices[] = {
        Vec2(-15, 6),
        Vec2(15, 6),
        Vec2(15, 14),
        Vec2(-15, 14)
    };
    Color4F lidColor(0.9f, 0.3f, 0.3f, 1.0f);
    displayNode_->drawPolygon(lidVertices, 4, lidColor, 1, Color4F(0.5f, 0.15f, 0.15f, 1.0f));
    
    // 金色边框 (表示能换钱)
    displayNode_->drawSolidRect(Vec2(-10, -10), Vec2(-8, 14), Color4F(1.0f, 0.84f, 0.0f, 1.0f)); // 左
    displayNode_->drawSolidRect(Vec2(8, -10), Vec2(10, 14), Color4F(1.0f, 0.84f, 0.0f, 1.0f));   // 右

    // 投递口标志 (比如一个金币或者简单的符号)
    displayNode_->drawSolidCircle(Vec2(0, 5), 3, 0, 10, Color4F(1.0f, 0.84f, 0.0f, 1.0f));

    this->addChild(displayNode_, 0);
}
