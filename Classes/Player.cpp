#include "Player.h"
#include "MapLayer.h" 

USING_NS_CC;

const float FRAME_WIDTH = 48.0f;   // 如果图片是48，就写48
const float FRAME_HEIGHT = 48.0f;  //

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

// 析构函数：非常重要！
// 释放我们手动 retain 的动画对象，防止内存泄漏
Player::~Player()
{
    CC_SAFE_RELEASE(walkUpAnimation_);
    CC_SAFE_RELEASE(walkDownAnimation_);
    CC_SAFE_RELEASE(walkLeftAnimation_);
    CC_SAFE_RELEASE(walkRightAnimation_);
}

bool Player::init()
{
    if (!Sprite::init())
        return false;

    // 1. 加载动画资源 (核心步骤)
    loadAnimations();

    // 2. 设置初始状态
    currentState_ = PlayerState::IDLE;
    currentAnimAction_ = nullptr;

    // 3. 设置初始外观 (站立图)
    // 注意：这里需要确保 characters/player_idle.png 存在
    if (FileUtils::getInstance()->isFileExist("characters/player_idle.png")) {
        this->initWithFile("characters/player_idle.png");
    }
    else {
        // 如果没有图片，先用色块代替（调试用）
        this->setTextureRect(Rect(0, 0, FRAME_WIDTH, FRAME_HEIGHT));
        this->setColor(Color3B::BLUE);
    }

    // 你的地图格子是 32x32，如果你希望人物正好占一个格子大小
    float targetHeight = 36.0f;

    // 计算缩放比例： 32 / 48 = 0.66
    float scaleRatio = targetHeight / FRAME_HEIGHT;

    this->setScale(scaleRatio); // <--- 添加这一行，Cocos会自动把它缩放到32大小

    // 4. 初始化数值
    moveSpeed_ = 150.0f;
    moveDirection_ = Vec2::ZERO;
    facingDirection_ = Vec2(0, -1); // 默认朝下
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

// 加载动画的具体实现
void Player::loadAnimations()
{
    // 初始化指针为空
    walkUpAnimation_ = nullptr;
    walkDownAnimation_ = nullptr;
    walkLeftAnimation_ = nullptr;
    walkRightAnimation_ = nullptr;

    // Lambda 辅助函数：简化重复代码
    // 参数：方向后缀(如"down")，帧数
    auto createAnim = [](std::string direction, int frameCount) -> Animation* {
        Vector<SpriteFrame*> frames;
        for (int i = 1; i <= frameCount; i++) {
            std::string filename = StringUtils::format("characters/player_walk_%s_%d.png", direction.c_str(), i);

            // 检查文件是否存在，防止崩溃
            if (!FileUtils::getInstance()->isFileExist(filename)) {
                CCLOG("Warning: Animation file missing: %s", filename.c_str());
                continue;
            }

            auto frame = SpriteFrame::create(filename, Rect(0, 0, FRAME_WIDTH, FRAME_HEIGHT));
            if (frame) frames.pushBack(frame);
        }
        // 0.15f 是每帧间隔时间，数字越小动作越快
        return Animation::createWithSpriteFrames(frames, 0.15f);
        };

    // 创建并保留引用 (Retain)
    walkDownAnimation_ = createAnim("down", 4);
    if (walkDownAnimation_) walkDownAnimation_->retain();

    walkUpAnimation_ = createAnim("up", 4);
    if (walkUpAnimation_) walkUpAnimation_->retain();

    walkLeftAnimation_ = createAnim("left", 4);
    if (walkLeftAnimation_) walkLeftAnimation_->retain();

    walkRightAnimation_ = createAnim("right", 4);
    if (walkRightAnimation_) walkRightAnimation_->retain();
}

// 播放动画的核心逻辑
void Player::playAnimation(PlayerState state)
{
    if (currentState_ == state) return; // 状态未改变，直接返回

    // 1. 停止当前正在播放的动作
    if (currentAnimAction_) {
        this->stopAction(currentAnimAction_);
        currentAnimAction_ = nullptr;
    }

    currentState_ = state;
    Animation* targetAnim = nullptr;

    // 2. 根据状态选择动画数据
    switch (state) {
    case PlayerState::WALK_UP:    targetAnim = walkUpAnimation_; break;
    case PlayerState::WALK_DOWN:  targetAnim = walkDownAnimation_; break;
    case PlayerState::WALK_LEFT:  targetAnim = walkLeftAnimation_; break;
    case PlayerState::WALK_RIGHT: targetAnim = walkRightAnimation_; break;
    case PlayerState::IDLE:
    default:
        // 待机状态：恢复为站立图
        if (FileUtils::getInstance()->isFileExist("characters/player_idle.png")) {
            this->setSpriteFrame(SpriteFrame::create("characters/player_idle.png", Rect(0, 0, FRAME_WIDTH, FRAME_HEIGHT)));
        }
        return;
    }

    // 执行新动画
    if (targetAnim) {
        auto animate = Animate::create(targetAnim);
        // RepeatForever 让角色一直走动
        currentAnimAction_ = RepeatForever::create(animate);
        this->runAction(currentAnimAction_);
    }
}

void Player::update(float delta)
{
    // 更新无敌时间
    if (isInvulnerable_)
    {
        invulnerableTimer_ -= delta;
        if (invulnerableTimer_ <= 0)
        {
            isInvulnerable_ = false;
            this->setOpacity(255);
            this->setVisible(true); // 确保闪烁结束后可见
        }
    }

    // 计算移动方向
    Vec2 dir = Vec2::ZERO;
    if (isUpPressed_) dir.y += 1;
    if (isDownPressed_) dir.y -= 1;
    if (isLeftPressed_) dir.x -= 1;
    if (isRightPressed_) dir.x += 1;

    moveDirection_ = dir;
    isMoving_ = (dir.lengthSquared() > 0);

    // ========== 动画状态机控制 ==========
    if (isMoving_)
    {
        // 判定优先级：如果同时按两个键，决定播放哪个方向的动画
        // 这里的逻辑是：垂直移动幅度大就播垂直动画，否则播水平动画
        if (std::abs(dir.y) >= std::abs(dir.x)) {
            if (dir.y > 0) playAnimation(PlayerState::WALK_UP);
            else playAnimation(PlayerState::WALK_DOWN);
        }
        else {
            if (dir.x > 0) playAnimation(PlayerState::WALK_RIGHT);
            else playAnimation(PlayerState::WALK_LEFT);
        }

        // 更新物理朝向
        facingDirection_ = dir;
        facingDirection_.normalize();

        // 执行物理移动
        updateMovement(delta);
    }
    else
    {
        // 没有按键时，播放待机
        playAnimation(PlayerState::IDLE);
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
        // 1. 尝试直接移动到目标位置
        if (mapLayer_->isWalkable(nextPos))
        {
            this->setPosition(nextPos);
        }
        else
        {
            // 2. 滑墙逻辑 (Slide along wall)
            // 如果斜着走被挡住了，尝试只保留 X 轴移动
            Vec2 nextPosX = currentPos + Vec2(direction.x * moveSpeed_ * delta, 0);
            if (mapLayer_->isWalkable(nextPosX) && std::abs(direction.x) > 0.1f)
            {
                this->setPosition(nextPosX);
            }
            else
            {
                // 3. 尝试只保留 Y 轴移动
                Vec2 nextPosY = currentPos + Vec2(0, direction.y * moveSpeed_ * delta);
                if (mapLayer_->isWalkable(nextPosY) && std::abs(direction.y) > 0.1f)
                {
                    this->setPosition(nextPosY);
                }
            }
        }
    }
    else
    {
        // 如果没有地图层，直接移动不检测碰撞
        this->setPosition(nextPos);
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
        // 这里可以添加死亡逻辑，比如停止所有动画，播放死亡帧等
        this->stopAllActions();
    }
    else
    {
        isInvulnerable_ = true;
        invulnerableTimer_ = 1.0f;
        // 受伤闪烁效果
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