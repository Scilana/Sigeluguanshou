#include "Player.h"
#include "MapLayer.h"

USING_NS_CC;

Player* Player::create()
{
    Player* player = new (std::nothrow) Player();
    if (player && player->init())
    {
        player->autorelease();
        return player;
    }
    CC_SAFE_DELETE(player);
    return nullptr;
}

bool Player::init()
{
    // 使用纹理初始化（你可以替换为实际的玩家图片）
    if (!Sprite::init())
        return false;

    // 创建简单的玩家外观（32x32的红色方块）
    // 后期可以替换为真实的玩家精灵图
    this->setTextureRect(Rect(0, 0, 32, 32));
    this->setColor(Color3B(255, 100, 100));  // 红色

    // 设置锚点为中心
    this->setAnchorPoint(Vec2(0.5f, 0.5f));

    // 初始化成员变量
    mapLayer_ = nullptr;  // 地图层引用（稍后设置）
    moveSpeed_ = 200.0f;  // 200像素/秒
    moveDirection_ = Vec2::ZERO;

    keyW_ = false;
    keyA_ = false;
    keyS_ = false;
    keyD_ = false;

    keyboardListener_ = nullptr;

    // 启动更新
    this->scheduleUpdate();

    CCLOG("Player created");
    return true;
}

void Player::update(float delta)
{
    // 移动玩家
    movePlayer(delta);
}

void Player::setMoveSpeed(float speed)
{
    moveSpeed_ = speed;
}

void Player::setMapLayer(MapLayer* mapLayer)
{
    mapLayer_ = mapLayer;
    CCLOG("Player: MapLayer reference set for collision detection");
}

void Player::enableKeyboardControl()
{
    if (keyboardListener_)
        return;  // 已经启用

    // 创建键盘监听器
    keyboardListener_ = EventListenerKeyboard::create();
    keyboardListener_->onKeyPressed = CC_CALLBACK_2(Player::onKeyPressed, this);
    keyboardListener_->onKeyReleased = CC_CALLBACK_2(Player::onKeyReleased, this);

    // 添加监听器
    _eventDispatcher->addEventListenerWithSceneGraphPriority(keyboardListener_, this);

    CCLOG("Player keyboard control enabled");
}

void Player::disableKeyboardControl()
{
    if (!keyboardListener_)
        return;

    _eventDispatcher->removeEventListener(keyboardListener_);
    keyboardListener_ = nullptr;

    // 重置按键状态
    keyW_ = keyA_ = keyS_ = keyD_ = false;
    updateMoveDirection();

    CCLOG("Player keyboard control disabled");
}

void Player::onKeyPressed(EventKeyboard::KeyCode keyCode, Event* event)
{
    switch (keyCode)
    {
    case EventKeyboard::KeyCode::KEY_W:
    case EventKeyboard::KeyCode::KEY_UP_ARROW:
        keyW_ = true;
        break;
    case EventKeyboard::KeyCode::KEY_A:
    case EventKeyboard::KeyCode::KEY_LEFT_ARROW:
        keyA_ = true;
        break;
    case EventKeyboard::KeyCode::KEY_S:
    case EventKeyboard::KeyCode::KEY_DOWN_ARROW:
        keyS_ = true;
        break;
    case EventKeyboard::KeyCode::KEY_D:
    case EventKeyboard::KeyCode::KEY_RIGHT_ARROW:
        keyD_ = true;
        break;
    default:
        break;
    }

    updateMoveDirection();
}

void Player::onKeyReleased(EventKeyboard::KeyCode keyCode, Event* event)
{
    switch (keyCode)
    {
    case EventKeyboard::KeyCode::KEY_W:
    case EventKeyboard::KeyCode::KEY_UP_ARROW:
        keyW_ = false;
        break;
    case EventKeyboard::KeyCode::KEY_A:
    case EventKeyboard::KeyCode::KEY_LEFT_ARROW:
        keyA_ = false;
        break;
    case EventKeyboard::KeyCode::KEY_S:
    case EventKeyboard::KeyCode::KEY_DOWN_ARROW:
        keyS_ = false;
        break;
    case EventKeyboard::KeyCode::KEY_D:
    case EventKeyboard::KeyCode::KEY_RIGHT_ARROW:
        keyD_ = false;
        break;
    default:
        break;
    }

    updateMoveDirection();
}

void Player::updateMoveDirection()
{
    // 计算移动方向
    moveDirection_ = Vec2::ZERO;

    if (keyW_) moveDirection_.y += 1.0f;
    if (keyS_) moveDirection_.y -= 1.0f;
    if (keyA_) moveDirection_.x -= 1.0f;
    if (keyD_) moveDirection_.x += 1.0f;

    // 归一化方向向量（使对角线移动速度一致）
    if (moveDirection_.length() > 0)
    {
        moveDirection_.normalize();
    }
}

void Player::movePlayer(float delta)
{
    if (moveDirection_.length() == 0)
        return;  // 没有移动

    // 计算移动距离
    Vec2 movement = moveDirection_ * moveSpeed_ * delta;

    // 计算新位置
    Vec2 currentPos = this->getPosition();
    Vec2 newPos = currentPos + movement;

    // 如果有地图层引用，进行碰撞检测和边界检查
    if (mapLayer_)
    {
        // 获取地图大小
        Size mapSize = mapLayer_->getMapSize();

        // 玩家大小（假设是32x32）
        float playerHalfSize = 16.0f;

        // 检查并限制在地图边界内
        newPos.x = std::max(playerHalfSize, std::min(newPos.x, mapSize.width - playerHalfSize));
        newPos.y = std::max(playerHalfSize, std::min(newPos.y, mapSize.height - playerHalfSize));

        // 检查新位置是否可行走
        if (mapLayer_->isWalkable(newPos))
        {
            // 可以移动
            this->setPosition(newPos);
        }
        else
        {
            // 调试：记录碰撞
            static int collisionCount = 0;
            collisionCount++;
            if (collisionCount <= 5)  // 只打印前5次碰撞
            {
                CCLOG("Collision %d: tried to move from (%.1f, %.1f) to (%.1f, %.1f)",
                    collisionCount, currentPos.x, currentPos.y, newPos.x, newPos.y);
            }

            // 不可行走，尝试分别检测X和Y方向
            // 这样可以实现"滑墙"效果

            // 尝试只在X方向移动
            Vec2 newPosX = Vec2(newPos.x, currentPos.y);
            // 限制在边界内
            newPosX.x = std::max(playerHalfSize, std::min(newPosX.x, mapSize.width - playerHalfSize));

            if (mapLayer_->isWalkable(newPosX))
            {
                this->setPosition(newPosX);
            }
            // 尝试只在Y方向移动
            else
            {
                Vec2 newPosY = Vec2(currentPos.x, newPos.y);
                // 限制在边界内
                newPosY.y = std::max(playerHalfSize, std::min(newPosY.y, mapSize.height - playerHalfSize));

                if (mapLayer_->isWalkable(newPosY))
                {
                    this->setPosition(newPosY);
                }
                // 如果两个方向都不能移动，则停留在原位
            }
        }
    }
    else
    {
        // 没有地图层引用，直接移动（无碰撞检测）
        this->setPosition(newPos);

        // 第一次警告
        static bool warningShown = false;
        if (!warningShown)
        {
            CCLOG("Warning: Player has no MapLayer reference, collision detection disabled!");
            warningShown = true;
        }
    }
}