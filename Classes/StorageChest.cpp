#include "StorageChest.h"

USING_NS_CC;

StorageChest* StorageChest::create(const Vec2& tileCoord)
{
    StorageChest* ret = new (std::nothrow) StorageChest();
    if (ret && ret->init(tileCoord))
    {
        ret->autorelease();
        return ret;
    }
    CC_SAFE_DELETE(ret);
    return nullptr;
}

bool StorageChest::init(const Vec2& tileCoord)
{
    if (!Node::init())
        return false;

    tileCoord_ = tileCoord;
    
    // 创建 32 格的独立背包
    inventory_ = InventoryManager::create(32);
    if (inventory_) {
        this->addChild(inventory_); // 挂在节点下，方便自动销毁或引用
    }

    initDisplay();

    return true;
}

void StorageChest::initDisplay()
{
    displayNode_ = DrawNode::create();

    // 参考 TreasureChest 的视觉表现，但可以用稍微不同的颜色
    // 绘制箱体 (底座) - 棕色
    Vec2 bodyVertices[] = {
        Vec2(-14, -10),
        Vec2(14, -10),
        Vec2(14, 6),
        Vec2(-14, 6)
    };
    Color4F bodyColor(0.45f, 0.25f, 0.1f, 1.0f); // 略深
    displayNode_->drawPolygon(bodyVertices, 4, bodyColor, 1, Color4F(0.2f, 0.1f, 0.05f, 1.0f));

    // 绘制盖子
    Vec2 lidVertices[] = {
        Vec2(-15, 6),
        Vec2(15, 6),
        Vec2(15, 14),
        Vec2(-15, 14)
    };
    Color4F lidColor(0.55f, 0.3f, 0.1f, 1.0f);
    displayNode_->drawPolygon(lidVertices, 4, lidColor, 1, Color4F(0.3f, 0.15f, 0.05f, 1.0f));
    
    // 银色边框 (Storage Chest 区别于 Mine Chest 的金色边框)
    displayNode_->drawSolidRect(Vec2(-10, -10), Vec2(-8, 14), Color4F(0.7f, 0.7f, 0.7f, 1.0f)); // 左
    displayNode_->drawSolidRect(Vec2(8, -10), Vec2(10, 14), Color4F(0.7f, 0.7f, 0.7f, 1.0f));   // 右

    // 锁扣
    displayNode_->drawSolidRect(Vec2(-2, 4), Vec2(2, 8), Color4F(0.3f, 0.3f, 0.3f, 1.0f));

    this->addChild(displayNode_, 0);
}
