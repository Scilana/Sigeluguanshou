#include "Player.h"
#include "MapLayer.h" 

USING_NS_CC;

// 【配置区域】
// 图片尺寸：70x120
const float FRAME_WIDTH = 70.0f;
const float FRAME_HEIGHT = 120.0f;

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
    CC_SAFE_RELEASE(useAxeUpAnimation_);
    CC_SAFE_RELEASE(useAxeDownAnimation_);
    CC_SAFE_RELEASE(useAxeLeftAnimation_);
    CC_SAFE_RELEASE(useAxeRightAnimation_);
    CC_SAFE_RELEASE(useHoeUpAnimation_);
    CC_SAFE_RELEASE(useHoeDownAnimation_);
    CC_SAFE_RELEASE(useHoeLeftAnimation_);
    CC_SAFE_RELEASE(useHoeRightAnimation_);

}

bool Player::init()
{
    if (!Sprite::init())
        return false;

    // 1. 加载动画资源 (核心步骤)
    loadAnimations();

    // 2. 设置初始状态
    currentState_ = PlayerState::IDLE;
    stateBeforeAttack_ = PlayerState::IDLE;
    currentAnimAction_ = nullptr;
    isAttacking_ = false;

    // 3. 设置初始外观 (默认朝下的站立图)
    if (FileUtils::getInstance()->isFileExist("characters/standDown.png")) {
        this->initWithFile("characters/standDown.png");
    }
    else {
        // 如果没有图片，先用色块代替（调试用）
        this->setTextureRect(Rect(0, 0, FRAME_WIDTH, FRAME_HEIGHT));
        this->setColor(Color3B::BLUE);
    }

    // 缩放设置
    this->setScale(0.3f);

    // 保持像素清晰，不模糊 (可选)
    // this->getTexture()->setAliasTexParameters();

    // 4. 初始化移动相关数值
    baseMoveSpeed_ = 150.0f;
    moveSpeed_ = baseMoveSpeed_;
    moveDirection_ = Vec2::ZERO;
    facingDirection_ = Vec2(0, -1); // 默认朝下
    isMoving_ = false;
    mapLayer_ = nullptr;

    // 5. 初始化按键状态
    isUpPressed_ = false;
    isDownPressed_ = false;
    isLeftPressed_ = false;
    isRightPressed_ = false;
    isAttackPressed_ = false;

    // 6. 初始化战斗属性
    maxHp_ = 100;
    hp_ = maxHp_;
    isInvulnerable_ = false;
    invulnerableTimer_ = 0.0f;

    // 7. 初始化能量系统
    maxEnergy_ = 270.0f;
    currentEnergy_ = maxEnergy_;
    isExhausted_ = false;

    this->scheduleUpdate();

    return true;
}

// 加载动画的具体实现
void Player::loadAnimations()
{
    // 1. 初始化指针
    walkUpAnimation_ = nullptr;
    walkDownAnimation_ = nullptr;
    walkLeftAnimation_ = nullptr;
    walkRightAnimation_ = nullptr;

    useAxeUpAnimation_ = nullptr;
    useAxeDownAnimation_ = nullptr;
    useAxeLeftAnimation_ = nullptr;
    useAxeRightAnimation_ = nullptr;

    useHoeUpAnimation_ = nullptr;
    useHoeDownAnimation_ = nullptr;
    useHoeLeftAnimation_ = nullptr;
    useHoeRightAnimation_ = nullptr;

    // 2. 修正后的 Lambda 函数：区分走路和工具的命名格式
    auto createAnim = [](std::string actionName, std::string direction, int frameCount) -> Animation* {
        Vector<SpriteFrame*> frames;
        for (int i = 1; i <= frameCount; i++) {
            std::string filename;

            // 【关键修复】如果是走路动画，使用原版命名格式
            if (actionName == "walk") {
                // 格式：characters/player_walk_down_1.png
                filename = StringUtils::format("characters/player_walk_%s_%d.png", direction.c_str(), i);
            }
            // 如果是工具动画 (UseAxe 或 UseHoe)，使用新版命名格式
            else {
                // 格式：characters/downUseHoe1.png
                filename = StringUtils::format("characters/%s%s%d.png", direction.c_str(), actionName.c_str(), i);
            }

            // 检查文件是否存在
            if (!FileUtils::getInstance()->isFileExist(filename)) {
                // 只有文件真的缺失时才报错，避免刷屏
                CCLOG("Warning: Animation file missing: %s", filename.c_str());
                continue;
            }

            auto frame = SpriteFrame::create(filename, Rect(0, 0, 70, 120));
            if (frame) frames.pushBack(frame);
        }

        // 走路速度 0.15f，挥动工具速度 0.1f (稍微快点)
        float delay = (actionName == "walk") ? 0.15f : 0.1f;
        return Animation::createWithSpriteFrames(frames, delay);
        };

    // 3. 加载走路动画 (这里 actionName 传 "walk"，会触发上面的 if 分支)
    walkDownAnimation_ = createAnim("walk", "down", 4);
    if (walkDownAnimation_) walkDownAnimation_->retain();

    walkUpAnimation_ = createAnim("walk", "up", 4);
    if (walkUpAnimation_) walkUpAnimation_->retain();

    walkLeftAnimation_ = createAnim("walk", "left", 3);
    if (walkLeftAnimation_) walkLeftAnimation_->retain();

    walkRightAnimation_ = createAnim("walk", "right", 3);
    if (walkRightAnimation_) walkRightAnimation_->retain();

    // 4. 加载斧头动画 (actionName 传 "UseAxe"，触发 else 分支)
    useAxeDownAnimation_ = createAnim("UseAxe", "down", 3);
    if (useAxeDownAnimation_) useAxeDownAnimation_->retain();

    useAxeUpAnimation_ = createAnim("UseAxe", "up", 3);
    if (useAxeUpAnimation_) useAxeUpAnimation_->retain();

    useAxeLeftAnimation_ = createAnim("UseAxe", "left", 3);
    if (useAxeLeftAnimation_) useAxeLeftAnimation_->retain();

    useAxeRightAnimation_ = createAnim("UseAxe", "right", 3);
    if (useAxeRightAnimation_) useAxeRightAnimation_->retain();

    // 5. 加载锄头动画 (actionName 传 "UseHoe"，触发 else 分支)
    // 注意：根据你的截图，down 只有 2 帧，其他是 3 帧
    useHoeDownAnimation_ = createAnim("UseHoe", "down", 2);
    if (useHoeDownAnimation_) useHoeDownAnimation_->retain();

    useHoeUpAnimation_ = createAnim("UseHoe", "up", 3);
    if (useHoeUpAnimation_) useHoeUpAnimation_->retain();

    useHoeLeftAnimation_ = createAnim("UseHoe", "left", 3);
    if (useHoeLeftAnimation_) useHoeLeftAnimation_->retain();

    useHoeRightAnimation_ = createAnim("UseHoe", "right", 3);
    if (useHoeRightAnimation_) useHoeRightAnimation_->retain();
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
    case PlayerState::WALK_UP:
        targetAnim = walkUpAnimation_;
        break;
    case PlayerState::WALK_DOWN:
        targetAnim = walkDownAnimation_;
        break;
    case PlayerState::WALK_LEFT:
        targetAnim = walkLeftAnimation_;
        break;
    case PlayerState::WALK_RIGHT:
        targetAnim = walkRightAnimation_;
        break;

    case PlayerState::ATTACK:
    {
        Vec2 dir = facingDirection_;
        // 判断主要方向
        bool isVertical = (std::abs(dir.y) >= std::abs(dir.x));
        bool isUp = (dir.y > 0);
        bool isRight = (dir.x > 0);

        // 【核心修改】根据当前拿的工具类型决定播放什么动画
        if (currentToolType_ == ItemType::Hoe)
        {
            // === 锄头动画 ===
            if (isVertical) {
                targetAnim = isUp ? useHoeUpAnimation_ : useHoeDownAnimation_;
            }
            else {
                targetAnim = isRight ? useHoeRightAnimation_ : useHoeLeftAnimation_;
            }
        }
        else
        {
            // === 默认/斧头动画 ===
            // (如果以后有镐子 Pickaxe，也可以在这里加 else if)
            if (isVertical) {
                targetAnim = isUp ? useAxeUpAnimation_ : useAxeDownAnimation_;
            }
            else {
                targetAnim = isRight ? useAxeRightAnimation_ : useAxeLeftAnimation_;
            }
        }
    }
    break;

    case PlayerState::IDLE:
    default:
        // 待机状态：根据朝向显示对应的站立图
    {
        std::string standFile;
        Vec2 dir = facingDirection_;

        // 判断朝向（优先判断垂直方向）
        if (std::abs(dir.y) >= std::abs(dir.x)) {
            standFile = (dir.y > 0) ? "characters/standUp.png" : "characters/standDown.png";
        }
        else {
            standFile = (dir.x > 0) ? "characters/standRight.png" : "characters/standLeft.png";
        }

        // 加载对应的站立图
        if (FileUtils::getInstance()->isFileExist(standFile)) {
            this->setSpriteFrame(SpriteFrame::create(standFile, Rect(0, 0, FRAME_WIDTH, FRAME_HEIGHT)));
        }
    }
    return;
    }

    // 执行新动画
    if (targetAnim) {
        auto animate = Animate::create(targetAnim);

        if (state == PlayerState::ATTACK) {
            // 攻击动画只播放一次，完成后执行回调
            auto callback = CallFunc::create(CC_CALLBACK_0(Player::onAttackAnimationFinished, this));
            currentAnimAction_ = Sequence::create(animate, callback, nullptr);
        }
        else {
            // 走路动画循环播放
            currentAnimAction_ = RepeatForever::create(animate);
        }

        this->runAction(currentAnimAction_);
    }
}

// 攻击动画结束后的回调
void Player::onAttackAnimationFinished()
{
    isAttacking_ = false;

    // 恢复到攻击前的状态
    playAnimation(stateBeforeAttack_);
}

// 播放挥斧动画（按下J键时调用）
void Player::playSwingAnimation()
{
    if (isAttacking_) return; // 如果正在攻击，不重复触发

    // 保存当前状态（用于恢复）
    stateBeforeAttack_ = currentState_;

    // 标记为攻击状态
    isAttacking_ = true;

    // 播放攻击动画
    playAnimation(PlayerState::ATTACK);

    CCLOG("Swing attack! Direction: (%.2f, %.2f)", facingDirection_.x, facingDirection_.y);
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

    // ========== 攻击状态优先级最高 ==========
    // 如果正在攻击，不处理移动
    if (isAttacking_) {
        return;
    }

    // 检测是否按下攻击键
    if (isAttackPressed_) {
        playSwingAnimation();
        return; // 开始攻击后不处理移动
    }

    // ========== 移动逻辑 ==========
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
        // 更新朝向（移动时更新，停止后保持）
        facingDirection_ = dir;
        facingDirection_.normalize();

        // 判定优先级：如果同时按两个键，决定播放哪个方向的动画
        // 这里的逻辑是：垂直移动幅度大就播垂直动画，否则播水平动画
        if (std::abs(dir.y) >= std::abs(dir.x)) {
            playAnimation((dir.y > 0) ? PlayerState::WALK_UP : PlayerState::WALK_DOWN);
        }
        else {
            playAnimation((dir.x > 0) ? PlayerState::WALK_RIGHT : PlayerState::WALK_LEFT);
        }

        // 执行物理移动
        updateMovement(delta);
    }
    else
    {
        // 没有按键时，播放待机（会根据 facingDirection_ 显示对应的站立图）
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

// ========== 能量系统 ==========

void Player::consumeEnergy(float amount)
{
    if (amount <= 0) return;

    currentEnergy_ -= amount;
    if (currentEnergy_ <= 0)
    {
        currentEnergy_ = 0;
        if (!isExhausted_)
        {
            setExhausted(true);
            CCLOG("Player is EXHAUSTED! Energy: 0");
        }
    }
}

void Player::recoverEnergy(float amount)
{
    if (amount <= 0) return;

    currentEnergy_ += amount;
    if (currentEnergy_ > maxEnergy_)
    {
        currentEnergy_ = maxEnergy_;
    }

    if (isExhausted_ && currentEnergy_ > 0)
    {
        // 简单处理：只要有能量就不再力竭
        setExhausted(false);
    }
}

void Player::setExhausted(bool exhausted)
{
    isExhausted_ = exhausted;
    if (isExhausted_)
    {
        moveSpeed_ = baseMoveSpeed_ * 0.5f; // 力竭减半
    }
    else
    {
        moveSpeed_ = baseMoveSpeed_;
    }
}

// ========== 战斗系统 ==========

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

void Player::heal(int amount)
{
    hp_ += amount;
    if (hp_ > maxHp_) hp_ = maxHp_;
    CCLOG("Player healed: +%d, Current HP: %d", amount, hp_);
}

// ========== 键盘控制 ==========

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
    isAttackPressed_ = false;
    isMoving_ = false;
    moveDirection_ = Vec2::ZERO;
}

void Player::setMapLayer(MapLayer* mapLayer)
{
    mapLayer_ = mapLayer;
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
    case EventKeyboard::KeyCode::KEY_J:
        isAttackPressed_ = true;
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
    case EventKeyboard::KeyCode::KEY_J:
        isAttackPressed_ = false;
        break;
    default:
        break;
    }
}