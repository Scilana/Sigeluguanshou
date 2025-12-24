#include "Player.h"
#include "MapLayer.h" 

USING_NS_CC;

Player* Player::create()
{
    Player* ret = new (std::nothrow) Player();
    if (ret && ret->init())
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

bool Player::init()
{
    if (!Sprite::init())
        return false;

    if (FileUtils::getInstance()->isFileExist("characters/player.png"))
    {
        this->initWithFile("characters/player.png");
    }
    else
    {
        auto drawNode = DrawNode::create();
        Vec2 vertices[] = {
            Vec2(-16, -16),
            Vec2(16, -16),
            Vec2(16, 16),
            Vec2(-16, 16)
        };
        drawNode->drawPolygon(vertices, 4, Color4F(0.2f, 0.4f, 0.8f, 1.0f), 1, Color4F::WHITE);
        this->addChild(drawNode);
    }

    moveSpeed_ = 150.0f;
    moveDirection_ = Vec2::ZERO;
    facingDirection_ = Vec2(0, -1);
    isMoving_ = false;
    mapLayer_ = nullptr;
    
    isUpPressed_ = false;
    isDownPressed_ = false;
    isLeftPressed_ = false;
    isRightPressed_ = false;

    maxHp_ = 100;
    hp_ = maxHp_;
    isInvulnerable_ = false;
    invulnerableTimer_ = 0.0f;

    this->scheduleUpdate();

    return true;
}

void Player::update(float delta)
{
    // 更新战斗状态
    if (isInvulnerable_)
    {
        invulnerableTimer_ -= delta;
        if (invulnerableTimer_ <= 0)
        {
            isInvulnerable_ = false;
            this->setOpacity(255);
        }
    }

    // 根据按键状态计算移动方向
    Vec2 dir = Vec2::ZERO;
    if (isUpPressed_) dir.y += 1;
    if (isDownPressed_) dir.y -= 1;
    if (isLeftPressed_) dir.x -= 1;
    if (isRightPressed_) dir.x += 1;
    
    moveDirection_ = dir;
    isMoving_ = (dir.lengthSquared() > 0);

    // 更新朝向
    if (isMoving_)
    {
        if (dir.x != 0 || dir.y != 0) {
            facingDirection_ = dir;
            facingDirection_.normalize();
        }
    }

    // 更新移动
    if (isMoving_)
    {
        updateMovement(delta);
    }
}

void Player::enableKeyboardControl()
{
    auto listener = EventListenerKeyboard::create();
    listener->onKeyPressed = CC_CALLBACK_2(Player::onKeyPressed, this);
    listener->onKeyReleased = CC_CALLBACK_2(Player::onKeyReleased, this);
    _eventDispatcher->addEventListenerWithSceneGraphPriority(listener, this);
}

void Player::disableKeyboardControl()
{
    _eventDispatcher->removeEventListenersForTarget(this);
    resetKeyStates();
}

void Player::resetKeyStates()
{
    isUpPressed_ = false;
    isDownPressed_ = false;
    isLeftPressed_ = false;
    isRightPressed_ = false;
    isMoving_ = false;
    moveDirection_ = Vec2::ZERO;
}

void Player::setMapLayer(MapLayer* mapLayer)
{
    mapLayer_ = mapLayer;
}

void Player::takeDamage(int damage)
{
    if (isInvulnerable_) return;
    
    hp_ -= damage;
    if (hp_ < 0) hp_ = 0;
    
    // 受伤无敌时间
    isInvulnerable_ = true;
    invulnerableTimer_ = 1.0f;
    
    // 闪烁效果
    auto blink = Blink::create(1.0f, 5);
    auto finish = CallFunc::create([this]() {
        isInvulnerable_ = false;
        this->setOpacity(255);
    });
    this->runAction(Sequence::create(blink, finish, nullptr));
    
    if (hp_ <= 0)
    {
        // 死亡
        CCLOG("Player died!");
    }
}

void Player::heal(int amount)
{
    hp_ += amount;
    if (hp_ > maxHp_) hp_ = maxHp_;
    CCLOG("Player healed: +%d, Current HP: %d", amount, hp_);
}

void Player::onKeyPressed(EventKeyboard::KeyCode keyCode, Event* event)
{
    switch (keyCode)
    {
    case EventKeyboard::KeyCode::KEY_W:
    case EventKeyboard::KeyCode::KEY_UP_ARROW:
        isUpPressed_ = true;
        break;
    case EventKeyboard::KeyCode::KEY_S:
    case EventKeyboard::KeyCode::KEY_DOWN_ARROW:
        isDownPressed_ = true;
        break;
    case EventKeyboard::KeyCode::KEY_A:
    case EventKeyboard::KeyCode::KEY_LEFT_ARROW:
        isLeftPressed_ = true;
        break;
    case EventKeyboard::KeyCode::KEY_D:
    case EventKeyboard::KeyCode::KEY_RIGHT_ARROW:
        isRightPressed_ = true;
        break;
    default:
        break;
    }
}

void Player::onKeyReleased(EventKeyboard::KeyCode keyCode, Event* event)
{
    switch (keyCode)
    {
    case EventKeyboard::KeyCode::KEY_W:
    case EventKeyboard::KeyCode::KEY_UP_ARROW:
        isUpPressed_ = false;
        break;
    case EventKeyboard::KeyCode::KEY_S:
    case EventKeyboard::KeyCode::KEY_DOWN_ARROW:
        isDownPressed_ = false;
        break;
    case EventKeyboard::KeyCode::KEY_A:
    case EventKeyboard::KeyCode::KEY_LEFT_ARROW:
        isLeftPressed_ = false;
        break;
    case EventKeyboard::KeyCode::KEY_D:
    case EventKeyboard::KeyCode::KEY_RIGHT_ARROW:
        isRightPressed_ = false;
        break;
    default:
        break;
    }
}

void Player::updateMovement(float delta)
{
    Vec2 currentPos = this->getPosition();
    Vec2 direction = moveDirection_;
    if (direction.lengthSquared() > 0)
        direction.normalize(); 

    Vec2 nextPos = currentPos + direction * moveSpeed_ * delta;

    if (mapLayer_)
    {
        if (mapLayer_->isWalkable(nextPos))
        {
            this->setPosition(nextPos);
        }
        else
        {
            Vec2 nextPosX = currentPos + Vec2(direction.x * moveSpeed_ * delta, 0);
            if (mapLayer_->isWalkable(nextPosX))
            {
                this->setPosition(nextPosX);
            }
            else
            {
                Vec2 nextPosY = currentPos + Vec2(0, direction.y * moveSpeed_ * delta);
                if (mapLayer_->isWalkable(nextPosY))
                {
                    this->setPosition(nextPosY);
                }
            }
        }
    }
    else
    {
        this->setPosition(nextPos);
    }
}