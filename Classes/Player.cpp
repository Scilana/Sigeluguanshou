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

void Player::setMapLayer(MapLayer* mapLayer)
{
    mapLayer_ = mapLayer;
}

void Player::takeDamage(int damage)
{
    if (isInvulnerable_) return;

    hp_ -= damage;
    if (hp_ < 0) hp_ = 0;

    CCLOG("Player took %d damage! HP: %d/%d", damage, hp_, maxHp_);

    if (hp_ <= 0)
    {
        CCLOG("Player Died!");
        // TODO: Game Over Logic
    }
    else
    {
        isInvulnerable_ = true;
        invulnerableTimer_ = 1.0f;
        auto blink = Blink::create(1.0f, 5);
        this->runAction(blink);
    }
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
    else
        return; // No input, no movement

    // 定义碰撞检测函数 (Lambda)
    auto isPositionValid = [&](const Vec2& pos) -> bool {
        if (!mapLayer_) return true; // 无地图层则无碰撞

        // 定义相对于中心点的检测偏移量 (缩小的包围盒)
        // 稍微缩小一点以避免过于严苛的卡顿
        float halfW = 8.0f;  // 宽度的一半 (16px)
        float halfH = 6.0f;  // 高度的一半 (12px)
        
        std::vector<Vec2> checkPoints;
        checkPoints.push_back(Vec2(0, 0));
        checkPoints.push_back(Vec2(-halfW, 0));
        checkPoints.push_back(Vec2(halfW, 0));
        checkPoints.push_back(Vec2(0, -halfH));
        checkPoints.push_back(Vec2(0, halfH));
        checkPoints.push_back(Vec2(-halfW, -halfH));
        checkPoints.push_back(Vec2(halfW, -halfH));
        checkPoints.push_back(Vec2(-halfW, halfH));
        checkPoints.push_back(Vec2(halfW, halfH));
        
        Size mapSize = mapLayer_->getMapSize(); 
        Size tileSize = mapLayer_->getTileSize();
        // 计算地图像素总大小
        float mapWidth = mapSize.width * tileSize.width;
        float mapHeight = mapSize.height * tileSize.height;
        
        for (const auto& offset : checkPoints)
        {
            Vec2 testPos = pos + offset;
            
            // 1. 检查地图边界 (使用像素坐标)
            if (testPos.x < 0 || testPos.x >= mapWidth ||
                testPos.y < 0 || testPos.y >= mapHeight)
            {
                return false; // Collision
            }
            
            // 2. 检查障碍物
            if (!mapLayer_->isWalkable(testPos))
            {
                return false; // Collision
            }
        }
        return true; // No collision
    };

    // 尝试直接移动到新位置
    Vec2 offset = direction * moveSpeed_ * delta;
    Vec2 newPos = currentPos + offset;

    if (isPositionValid(newPos))
    {
        this->setPosition(newPos);
    }
    else
    {
        // 尝试只移动 X 轴 (滑动)
        Vec2 newPosX = currentPos + Vec2(offset.x, 0);
        if (std::abs(offset.x) > 0.001f && isPositionValid(newPosX))
        {
            this->setPosition(newPosX);
        }
        // 尝试只移动 Y 轴 (滑动)
        // 注意：这里使用 else if 可能会导致在尖角处完全卡住，
        // 但通常最好一次只处理一个轴的滑动，或者分别处理。
        // 如果 X 移动了，通常 Y 就不动了（或者下一帧再处理）。
        // 这里为了简单，如果 X 能走就走 X，否则试 Y。
        else 
        {
            Vec2 newPosY = currentPos + Vec2(0, offset.y);
            if (std::abs(offset.y) > 0.001f && isPositionValid(newPosY))
            {
                this->setPosition(newPosY);
            }
        }
    }
}

void Player::heal(int amount)
{
    if (amount <= 0) return;
    
    hp_ += amount;
    if (hp_ > maxHp_)
    {
        hp_ = maxHp_;
    }
    
    CCLOG("Player healed for %d HP. Current HP: %d/%d", amount, hp_, maxHp_);
}