#include "Zombie.h"

USING_NS_CC;

Zombie* Zombie::create(int floorLevel)
{
    Zombie* ret = new (std::nothrow) Zombie();
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

bool Zombie::init(int floorLevel)
{
    // 调用父类初始化
    if (!Monster::init(floorLevel))
        return false;

    return true;
}

void Zombie::initStats()
{
    // 僵尸基础属性
    name_ = "Zombie";
    maxHp_ = 5;           // 高血量
    attackPower_ = 10;     // 高攻击
    moveSpeed_ = 35.0f;    // 较慢移动
    attackRange_ = 40.0f;
    attackCooldown_ = 1.5f;

    // 根据楼层等级增强属性
    float multiplier = 1.0f + (floorLevel_ - 1) * 0.6f;
    attackPower_ = static_cast<int>(attackPower_ * multiplier);
    moveSpeed_ *= (1.0f + (floorLevel_ - 1) * 0.1f);

    hp_ = maxHp_;

    CCLOG("Zombie created: HP=%d, ATK=%d, SPD=%.1f, Floor=%d",
          maxHp_, attackPower_, moveSpeed_, floorLevel_);
}

void Zombie::initDisplay()
{
    // 尝试加载贴图
    std::string spritePath = getSpritePath();
    bool spriteLoaded = false;

    if (!spritePath.empty())
    {
        if (this->initWithFile(spritePath))
        {
            CCLOG("Zombie sprite loaded: %s", spritePath.c_str());
            spriteLoaded = true;
        }
    }

    // 使用灰色方块作为临时显示（僵尸特征色）
    if (!spriteLoaded)
    {
        displayNode_ = DrawNode::create();

        // 绘制灰色方块（比史莱姆大，表示僵尸）
        Vec2 vertices[] = {
            Vec2(-14, -16),
            Vec2(14, -16),
            Vec2(14, 16),
            Vec2(-14, 16)
        };
        Color4F zombieColor(0.5f, 0.5f, 0.5f, 1.0f);  // 灰色
        displayNode_->drawPolygon(vertices, 4, zombieColor, 1, Color4F::WHITE);

        this->addChild(displayNode_, 0);
    }

    // 血条显示
    hpLabel_ = Label::createWithSystemFont("", "Arial", 10);
    hpLabel_->setPosition(Vec2(0, 22));
    hpLabel_->setColor(Color3B::RED);
    this->addChild(hpLabel_, 1);

    updateHpDisplay();
}
