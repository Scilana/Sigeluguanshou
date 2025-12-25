#include "Monster.h"
#include "Player.h"
#include "MineLayer.h"

USING_NS_CC;

Monster* Monster::create(int floorLevel)
{
    Monster* ret = new (std::nothrow) Monster();
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

bool Monster::init(int floorLevel)
{
    if (!Sprite::init())
        return false;

    floorLevel_ = floorLevel;
    targetPlayer_ = nullptr;
    mapLayer_ = nullptr;
    currentAttackCooldown_ = 0.0f;

    // 初始化属性
    initStats();

    // 初始化显示
    initDisplay();

    // 启动更新
    this->scheduleUpdate();

    return true;
}

void Monster::initStats()
{
    // 基础属性（子类可重写）
    name_ = "Monster";
    maxHp_ = 5;
    attackPower_ = 5;
    moveSpeed_ = 50.0f;
    attackRange_ = 40.0f;
    attackCooldown_ = 1.5f;

    // 根据楼层等级增强属性 (降低增长幅度)
    float multiplier = 1.0f + (floorLevel_ - 1) * 0.2f; // 每层增加 20%，之前是 50%
    
    attackPower_ = static_cast<int>(attackPower_ * 0.6f * multiplier); // 基础伤害打6折
    moveSpeed_ *= (1.0f + (floorLevel_ - 1) * 0.05f); 

    if (attackPower_ < 1) attackPower_ = 1;

    hp_ = maxHp_;

    CCLOG("Monster created: %s, HP=%d, ATK=%d, Floor=%d",
          name_.c_str(), maxHp_, attackPower_, floorLevel_);
}

void Monster::initDisplay()
{
    // 尝试加载贴图
    std::string spritePath = getSpritePath();
    if (!spritePath.empty())
    {
        // 如果有贴图路径，尝试加载
        if (this->initWithFile(spritePath))
        {
            CCLOG("Monster sprite loaded: %s", spritePath.c_str());
        }
        else
        {
            CCLOG("Failed to load sprite: %s, using placeholder", spritePath.c_str());
            spritePath = "";  // 回退到占位符
        }
    }

    // 使用白色方块作为临时显示
    if (spritePath.empty())
    {
        displayNode_ = DrawNode::create();

        // 绘制白色方块
        Vec2 vertices[] = {
            Vec2(-12, -12),
            Vec2(12, -12),
            Vec2(12, 12),
            Vec2(-12, 12)
        };
        displayNode_->drawPolygon(vertices, 4, Color4F::WHITE, 1, Color4F::BLACK);

        this->addChild(displayNode_, 0);
    }

    // 血条显示
    hpLabel_ = Label::createWithSystemFont("", "Arial", 10);
    hpLabel_->setPosition(Vec2(0, 20));
    hpLabel_->setColor(Color3B::RED);
    this->addChild(hpLabel_, 1);

    updateHpDisplay();
}

void Monster::update(float delta)
{
    if (isDead())
        return;

    // 更新攻击冷却
    if (currentAttackCooldown_ > 0)
    {
        currentAttackCooldown_ -= delta;
    }

    // AI逻辑
    updateAI(delta);

    // 更新血条
    updateHpDisplay();
}

void Monster::updateAI(float delta)
{
    if (!targetPlayer_ || isDead())
        return;

    Vec2 playerPos = targetPlayer_->getPosition();
    Vec2 myPos = this->getPosition();
    float distance = myPos.distance(playerPos);

    // 如果在攻击范围内，尝试攻击
    if (distance <= attackRange_)
    {
        if (currentAttackCooldown_ <= 0)
        {
            attackPlayer(targetPlayer_);
        }
    }
    else
    {
        // 否则追踪玩家
        moveTowards(playerPos, delta);
    }
}

void Monster::moveTowards(const Vec2& targetPos, float delta)
{
    Vec2 myPos = this->getPosition();
    Vec2 direction = targetPos - myPos;
    if (direction.length() > 0)
    {
        direction.normalize();
    }

    Vec2 newPos = myPos + direction * moveSpeed_ * delta;

    // 碰撞检测（如果有地图层）
    if (mapLayer_)
    {
        // 1. 检查是否在地图范围内
        Size mapSize = mapLayer_->getMapSize();
        if (newPos.x < 0 || newPos.x > mapSize.width ||
            newPos.y < 0 || newPos.y > mapSize.height)
        {
            return; // 超出地图边界，不移动
        }

        // 2. 检查是否可行走
        if (mapLayer_->isWalkable(newPos))
        {
            this->setPosition(newPos);
        }
        else
        {
            // 简单的滑墙处理：尝试只移动 X 或 Y
            Vec2 newPosX = myPos + Vec2(direction.x * moveSpeed_ * delta, 0);
            if (mapLayer_->isWalkable(newPosX))
            {
                this->setPosition(newPosX);
            }
            else
            {
                Vec2 newPosY = myPos + Vec2(0, direction.y * moveSpeed_ * delta);
                if (mapLayer_->isWalkable(newPosY))
                {
                    this->setPosition(newPosY);
                }
            }
        }
    }
    else
    {
        this->setPosition(newPos);
    }

}

void Monster::takeDamage(int damage)
{
    if (isDead())
        return;

    hp_ -= damage;
    CCLOG("%s took %d damage, HP: %d/%d", name_.c_str(), damage, hp_, maxHp_);

    // 受伤闪烁效果
    auto blink = Sequence::create(
        TintTo::create(0.1f, 255, 0, 0),
        TintTo::create(0.1f, 255, 255, 255),
        nullptr
    );
    this->runAction(blink);

    if (isDead())
    {
        onDeath();
    }
    else
    {
        updateHpDisplay();
    }
}

void Monster::attackPlayer(Player* player)
{
    if (!player || currentAttackCooldown_ > 0)
        return;

    CCLOG("%s attacks player for %d damage!", name_.c_str(), attackPower_);

    // 对玩家造成伤害
    player->takeDamage(attackPower_);

    // 重置攻击冷却
    currentAttackCooldown_ = attackCooldown_;

    // 攻击动画（简单的缩放效果）
    auto attackAnim = Sequence::create(
        ScaleTo::create(0.1f, 1.2f),
        ScaleTo::create(0.1f, 1.0f),
        nullptr
    );
    this->runAction(attackAnim);
}

void Monster::updateHpDisplay()
{
    if (hpLabel_)
    {
        char hpStr[32];
        sprintf(hpStr, "%d/%d", hp_, maxHp_);
        hpLabel_->setString(hpStr);
    }
}

void Monster::onDeath()
{
    CCLOG("%s died!", name_.c_str());

    // 死亡动画
    auto fadeOut = FadeOut::create(0.5f);
    auto remove = RemoveSelf::create();
    auto sequence = Sequence::create(fadeOut, remove, nullptr);
    this->runAction(sequence);
}
