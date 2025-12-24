#include "TreasureChest.h"
#include "Weapon.h"
#include <cstdlib>

USING_NS_CC;

TreasureChest* TreasureChest::create(int floorLevel)
{
    TreasureChest* ret = new (std::nothrow) TreasureChest();
    if (ret && ret->init(floorLevel))
    {
        ret->autorelease();
        return ret;
    }
    else
    {
        delete ret;
        ret = nullptr;
        return nullptr;
    }
}

bool TreasureChest::init(int floorLevel)
{
    if (!Node::init())
        return false;

    floorLevel_ = floorLevel;
    isOpened_ = false;

    initDisplay();

    CCLOG("TreasureChest created at floor %d", floorLevel_);

    return true;
}

void TreasureChest::initDisplay()
{
    displayNode_ = DrawNode::create();

    // 绘制箱体 (底座) - 深棕色
    Vec2 bodyVertices[] = {
        Vec2(-14, -10),
        Vec2(14, -10),
        Vec2(14, 6),
        Vec2(-14, 6)
    };
    Color4F bodyColor(0.55f, 0.27f, 0.07f, 1.0f); // Sattelbrown
    displayNode_->drawPolygon(bodyVertices, 4, bodyColor, 1, Color4F(0.3f, 0.15f, 0.05f, 1.0f));

    // 绘制盖子 -稍亮的棕色
    Vec2 lidVertices[] = {
        Vec2(-15, 6),
        Vec2(15, 6),
        Vec2(15, 14),
        Vec2(-15, 14)
    };
    Color4F lidColor(0.65f, 0.35f, 0.1f, 1.0f);
    displayNode_->drawPolygon(lidVertices, 4, lidColor, 1, Color4F(0.4f, 0.2f, 0.05f, 1.0f));
    
    // 金色镶边 (竖条)
    displayNode_->drawSolidRect(Vec2(-10, -10), Vec2(-7, 14), Color4F(1.0f, 0.84f, 0.0f, 1.0f)); // 左
    displayNode_->drawSolidRect(Vec2(7, -10), Vec2(10, 14), Color4F(1.0f, 0.84f, 0.0f, 1.0f));   // 右

    // 锁扣 (中间)
    displayNode_->drawSolidRect(Vec2(-3, 2), Vec2(3, 8), Color4F(0.8f, 0.8f, 0.8f, 1.0f)); // 银色锁底
    displayNode_->drawSolidCircle(Vec2(0, 5), 2, 0, 10, Color4F::BLACK); // 锁孔

    this->addChild(displayNode_, 0);

    // 标签 (?)
    label_ = Label::createWithSystemFont("?", "Arial", 12);
    label_->setPosition(Vec2(0, 24));
    label_->setColor(Color3B::YELLOW);
    // 浮动动画
    auto moveUp = MoveBy::create(1.0f, Vec2(0, 5));
    auto moveDown = moveUp->reverse();
    label_->runAction(RepeatForever::create(Sequence::create(moveUp, moveDown, nullptr)));
    
    this->addChild(label_, 1);
}

TreasureChest::LootResult TreasureChest::open()
{
    if (isOpened_)
    {
        return { ItemType::None, 0, "Already opened!" };
    }

    isOpened_ = true;
    playOpenAnimation();

    return generateLoot();
}

TreasureChest::LootResult TreasureChest::generateLoot()
{
    LootResult result = { ItemType::None, 0, "" };

    // 根据楼层生成掉落物品
    // 楼层越深，掉落越好
    int roll = rand() % 100;

    // 武器掉落概率：楼层1: 5%, 楼层2: 10%, 楼层3: 15%, 楼层4: 25%
    int weaponChance = 5 + (floorLevel_ - 1) * 5 + (floorLevel_ >= 3 ? 5 : 0);

    if (roll < weaponChance)
    {
        // 掉落武器
        int weaponRoll = rand() % 100;

        if (floorLevel_ >= 4 && weaponRoll < 20)
        {
            result.item = ItemType::DiamondSword;
            result.message = "Amazing! Diamond Sword!";
        }
        else if (floorLevel_ >= 3 && weaponRoll < 40)
        {
            result.item = ItemType::GoldSword;
            result.message = "Wow! Gold Sword!";
        }
        else if (floorLevel_ >= 2 && weaponRoll < 60)
        {
            result.item = ItemType::IronSword;
            result.message = "Nice! Iron Sword!";
        }
        else
        {
            result.item = ItemType::WoodenSword;
            result.message = "Found a Wooden Sword!";
        }
        result.count = 1;
    }
    else
    {
        // 掉落矿石或金币
        int resourceRoll = rand() % 100;

        if (floorLevel_ >= 3 && resourceRoll < 30)
        {
            // 金矿
            result.item = ItemType::GoldOre;
            result.count = 1 + rand() % 2;  // 1-2个
            result.message = StringUtils::format("Found %d Gold Ore!", result.count);
        }
        else if (floorLevel_ >= 2 && resourceRoll < 60)
        {
            // 银矿
            result.item = ItemType::SilverOre;
            result.count = 1 + rand() % 3;  // 1-3个
            result.message = StringUtils::format("Found %d Silver Ore!", result.count);
        }
        else
        {
            // 铜矿
            result.item = ItemType::CopperOre;
            result.count = 2 + rand() % 4;  // 2-5个
            result.message = StringUtils::format("Found %d Copper Ore!", result.count);
        }
    }

    CCLOG("Chest opened: %s x%d", InventoryManager::getItemName(result.item).c_str(), result.count);

    return result;
}

void TreasureChest::playOpenAnimation()
{
    // 改变颜色表示已开启
    if (displayNode_)
    {
        displayNode_->clear();

        // 绘制箱体 (底座) - 保持不变
        Vec2 bodyVertices[] = {
            Vec2(-14, -10),
            Vec2(14, -10),
            Vec2(14, 6),
            Vec2(-14, 6)
        };
        Color4F bodyColor(0.55f, 0.27f, 0.07f, 1.0f);
        displayNode_->drawPolygon(bodyVertices, 4, bodyColor, 1, Color4F(0.3f, 0.15f, 0.05f, 1.0f));

        // 绘制内部 (黑色/深色) - 表示空了
        displayNode_->drawSolidRect(Vec2(-12, -4), Vec2(12, 6), Color4F(0.1f, 0.05f, 0.0f, 1.0f));

        // 绘制盖子 (向后翻/移开)
        // 这里简单地画一个变暗的盖子在上方，表示打开
        Vec2 lidVertices[] = {
            Vec2(-15, 14),
            Vec2(15, 14),
            Vec2(15, 20),
            Vec2(-15, 20)
        };
        Color4F lidColor(0.45f, 0.25f, 0.05f, 1.0f); // 暗一点
        displayNode_->drawPolygon(lidVertices, 4, lidColor, 1, Color4F(0.3f, 0.15f, 0.05f, 1.0f));
    }

    if (label_)
    {
        label_->setString("O");
        label_->setColor(Color3B::GRAY);
    }

    // 缩放动画
    auto scaleUp = ScaleTo::create(0.1f, 1.2f);
    auto scaleDown = ScaleTo::create(0.1f, 1.0f);
    auto sequence = Sequence::create(scaleUp, scaleDown, nullptr);
    this->runAction(sequence);
}
