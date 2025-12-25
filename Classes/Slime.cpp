#include "Slime.h"

USING_NS_CC;

Slime* Slime::create(int floorLevel)
{
    Slime* ret = new (std::nothrow) Slime();
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

bool Slime::init(int floorLevel)
{
    // 调用父类初始化
    if (!Monster::init(floorLevel))
        return false;

    return true;
}

void Slime::initStats()
{
    // 史莱姆基础属性
    name_ = "Slime";
    maxHp_ = 5;           // 低血量
    attackPower_ = 5;      // 低攻击
    moveSpeed_ = 60.0f;    // 较快移动
    attackRange_ = 35.0f;
    attackCooldown_ = 2.0f;

    // 根据楼层等级增强属性
    float multiplier = 1.0f + (floorLevel_ - 1) * 0.4f;
    attackPower_ = static_cast<int>(attackPower_ * multiplier);
    moveSpeed_ *= (1.0f + (floorLevel_ - 1) * 0.15f);

    hp_ = maxHp_;

    CCLOG("Slime created: HP=%d, ATK=%d, SPD=%.1f, Floor=%d",
          maxHp_, attackPower_, moveSpeed_, floorLevel_);
}

void Slime::initDisplay()
{
    // 尝试加载贴图
    std::string spritePath = getSpritePath();
    bool spriteLoaded = false;

    if (!spritePath.empty())
    {
        if (this->initWithFile(spritePath))
        {
            CCLOG("Slime sprite loaded: %s", spritePath.c_str());
            spriteLoaded = true;
        }
    }

    // 使用绿色方块作为临时显示（史莱姆特征色）
    if (!spriteLoaded)
    {
        displayNode_ = DrawNode::create();

        // 绘制绿色方块（比普通怪物小一点，表示史莱姆）
        Vec2 vertices[] = {
            Vec2(-10, -8),
            Vec2(10, -8),
            Vec2(10, 8),
            Vec2(-10, 8)
        };
        Color4F slimeColor(0.3f, 0.9f, 0.3f, 1.0f);  // 亮绿色
        displayNode_->drawPolygon(vertices, 4, slimeColor, 1, Color4F::WHITE);

        this->addChild(displayNode_, 0);
    }

    // 血条显示
    hpLabel_ = Label::createWithSystemFont("", "Arial", 10);
    hpLabel_->setPosition(Vec2(0, 15));
    hpLabel_->setColor(Color3B::RED);
    this->addChild(hpLabel_, 1);

    updateHpDisplay();
}
