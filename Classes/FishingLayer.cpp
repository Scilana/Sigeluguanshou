#include "FishingLayer.h"
#include "SkillManager.h"

FishingLayer* FishingLayer::create()
{
    FishingLayer* ret = new (std::nothrow) FishingLayer();
    if (ret && ret->init())
    {
        ret->autorelease();
        return ret;
    }
    CC_SAFE_DELETE(ret);
    return nullptr;
}

bool FishingLayer::init()
{
    if (!Layer::init())
        return false;

    CCLOG("Initializing Fishing Layer...");

    // 灰色半透明背景，阻挡下层点击
    auto visibleSize = Director::getInstance()->getVisibleSize();
    auto bg = LayerColor::create(Color4B(0, 0, 0, 150));
    this->addChild(bg, 0);

    // 吞噬触摸事件，防止点到地图
    auto touchListener = EventListenerTouchOneByOne::create();
    touchListener->setSwallowTouches(true);
    touchListener->onTouchBegan = [](Touch* touch, Event* event) { return true; };
    _eventDispatcher->addEventListenerWithSceneGraphPriority(touchListener, bg);

    // 初始化参数
    barHeight_ = 400.0f; // 背景条像素高度
    greenBarHeight_ = 80.0f; // 绿条像素高度 (约为背景的1/5)
    
    fishPosition_ = 0.2f;
    barPosition_ = 0.1f;
    
    // 物理参数
    barSpeed_ = 0.0f;
    // 之前调整过的参数: gravity -5.0f, thrust 10.0f
    gravity_ = -5.0f;     // 重力下坠
    thrust_ = 10.0f;       // 按住上升加速度
    bounce_ = -0.5f;       // 触底轻微反弹
    isHolding_ = false;

    // 鱼 AI
    fishSpeed_ = 0.0f;
    fishTargetPos_ = 0.5f;
    moveTimer_ = 0.0f;

    // 进度
    catchProgress_ = 0.2f; // 初始给一点进度
    isGameOver_ = false;

    initUI();
    initInput();

    this->scheduleUpdate();

    return true;
}

void FishingLayer::initUI()
{
    auto visibleSize = Director::getInstance()->getVisibleSize();
    Vec2 center = Vec2(visibleSize.width / 2, visibleSize.height / 2);

    // 1. 钓鱼界面背景 (左侧长条)
    auto barBg = LayerColor::create(Color4B(50, 50, 50, 255));
    barBg->setContentSize(Size(40, barHeight_));
    barBg->ignoreAnchorPointForPosition(false);
    barBg->setAnchorPoint(Vec2(0.5, 0.5));
    barBg->setPosition(center + Vec2(-30, 0)); // 偏左一点
    this->addChild(barBg, 1);
    barBase_ = (Sprite*)barBg; // 强转保存引用

    // 2. 绿条 (Player control)
    auto green = LayerColor::create(Color4B(100, 255, 100, 200));
    green->setContentSize(Size(36, greenBarHeight_));
    green->ignoreAnchorPointForPosition(false);
    green->setAnchorPoint(Vec2(0.5, 0)); // 底部对齐
    barBg->addChild(green, 2);
    greenBar_ = (Sprite*)green;

    // 3. 鱼 (Target)
    auto fish = LayerColor::create(Color4B(255, 100, 100, 255));
    fish->setContentSize(Size(20, 20));
    fish->ignoreAnchorPointForPosition(false);
    fish->setAnchorPoint(Vec2(0.5, 0.5));
    barBg->addChild(fish, 3);
    fishSprite_ = (Sprite*)fish;

    // 4. 右侧进度条 (Catch progress)
    auto progressBg = LayerColor::create(Color4B(0, 0, 0, 255));
    progressBg->setContentSize(Size(15, barHeight_));
    progressBg->ignoreAnchorPointForPosition(false);
    progressBg->setAnchorPoint(Vec2(0.5, 0.5));
    progressBg->setPosition(center + Vec2(30, 0));
    this->addChild(progressBg, 1); // 修正为 add 到 this, 否则位置不对? 不, 这里是相对 center 偏移

    // 前景
    auto progressFg = LayerColor::create(Color4B(255, 215, 0, 255)); // 金色
    progressFg->setContentSize(Size(15, 0)); // 初始高度 0
    progressFg->setAnchorPoint(Vec2(0, 0));
    progressFg->setPosition(Vec2(0, 0));
    progressBg->addChild(progressFg);
    
    progressBg->setName("ProgressBg");
    progressFg->setName("ProgressFg");
}

void FishingLayer::initInput()
{
    // 鼠标监听
    auto mouseListener = EventListenerMouse::create();
    mouseListener->onMouseDown = [this](Event* event) {
        EventMouse* e = (EventMouse*)event;
        if (e->getMouseButton() == EventMouse::MouseButton::BUTTON_LEFT)
        {
            this->isHolding_ = true;
        }
    };
    mouseListener->onMouseUp = [this](Event* event) {
        EventMouse* e = (EventMouse*)event;
        if (e->getMouseButton() == EventMouse::MouseButton::BUTTON_LEFT)
        {
            this->isHolding_ = false;
        }
    };
    _eventDispatcher->addEventListenerWithSceneGraphPriority(mouseListener, this);

    // 键盘监听 (空格也可以控制)
    auto keyListener = EventListenerKeyboard::create();
    keyListener->onKeyPressed = [this](EventKeyboard::KeyCode code, Event* event) {
        if (code == EventKeyboard::KeyCode::KEY_SPACE || code == EventKeyboard::KeyCode::KEY_C)
        {
            this->isHolding_ = true;
        }
    };
    keyListener->onKeyReleased = [this](EventKeyboard::KeyCode code, Event* event) {
        if (code == EventKeyboard::KeyCode::KEY_SPACE || code == EventKeyboard::KeyCode::KEY_C)
        {
            this->isHolding_ = false;
        }
    };
    _eventDispatcher->addEventListenerWithSceneGraphPriority(keyListener, this);
}

void FishingLayer::update(float delta)
{
    if (isGameOver_) return;

    updateBarPhysics(delta);
    updateFishMovement(delta);
    updateProgress(delta);
}

void FishingLayer::updateBarPhysics(float delta)
{
    float acceleration = gravity_;
    if (isHolding_)
    {
        acceleration += thrust_;
    }

    barSpeed_ += acceleration * delta;
    barPosition_ += barSpeed_ * delta;

    // 边界碰撞
    float maxPos = 1.0f - (greenBarHeight_ / barHeight_);
    if (barPosition_ >= maxPos)
    {
        barPosition_ = maxPos;
        barSpeed_ = 0;
    }
    
    if (barPosition_ <= 0.0f)
    {
        barPosition_ = 0.0f;
        if (barSpeed_ < 0)
        {
            barSpeed_ = barSpeed_ * bounce_;
            if (std::abs(barSpeed_) < 0.5f) barSpeed_ = 0;
        }
    }

    if (greenBar_)
    {
        greenBar_->setPosition(Vec2(20, barPosition_ * barHeight_)); 
    }
}

void FishingLayer::updateFishMovement(float delta)
{
    moveTimer_ -= delta;
    if (moveTimer_ <= 0)
    {
        fishTargetPos_ = CCRANDOM_0_1() * 0.8f + 0.1f;
        moveTimer_ = CCRANDOM_0_1() * 2.0f + 0.5f;
    }

    float smooth = 2.0f * delta;
    if (std::abs(fishTargetPos_ - fishPosition_) < smooth)
    {
        fishPosition_ = fishTargetPos_;
    }
    else if (fishPosition_ < fishTargetPos_)
    {
        fishPosition_ += smooth;
    }
    else
    {
        fishPosition_ -= smooth;
    }

    if (fishSprite_)
    {
        fishSprite_->setPosition(Vec2(20, fishPosition_ * barHeight_));
    }
}

void FishingLayer::updateProgress(float delta)
{
    float barTop = barPosition_ + (greenBarHeight_ / barHeight_);
    float barBottom = barPosition_;
    
    // 稍微宽松一点判定
    bool isCatching = (fishPosition_ >= barBottom && fishPosition_ <= barTop);

    float gainMult = SkillManager::getInstance()->getFishingCatchGainMultiplier();
    float lossMult = SkillManager::getInstance()->getFishingCatchLossMultiplier();
    if (isCatching)
    {
        catchProgress_ += 0.2f * delta * gainMult;
        if (greenBar_) greenBar_->setOpacity(255);
    }
    else
    {
        catchProgress_ -= 0.1f * delta * lossMult;
        if (greenBar_) greenBar_->setOpacity(150);
    }

    catchProgress_ = std::max(0.0f, std::min(1.0f, catchProgress_));

    auto bg = this->getChildByName("ProgressBg");
    if (bg)
    {
        auto fg = bg->getChildByName("ProgressFg");
        if (fg)
        {
            fg->setContentSize(Size(15, catchProgress_ * barHeight_));
            if (catchProgress_ > 0.7f) fg->setColor(Color3B::GREEN);
            else if (catchProgress_ > 0.3f) fg->setColor(Color3B::YELLOW);
            else fg->setColor(Color3B::RED);
        }
    }

    if (catchProgress_ >= 1.0f)
    {
        finishFishing(true);
    }
    else if (catchProgress_ <= 0.0f)
    {
        finishFishing(false);
    }
}

void FishingLayer::finishFishing(bool success)
{
    if (isGameOver_) return;
    isGameOver_ = true;

    auto label = Label::createWithSystemFont(success ? "PERFECT!" : "MISS", "Arial", 48);
    label->setPosition(Director::getInstance()->getVisibleSize() / 2);
    label->setColor(success ? Color3B::YELLOW : Color3B::RED);
    this->addChild(label, 10);

    auto delay = DelayTime::create(1.5f);
    auto callFunc = CallFunc::create([this, success]() {
        if (finishCallback_)
        {
            finishCallback_(success);
        }
        this->removeFromParent();
    });

    this->runAction(Sequence::create(delay, callFunc, nullptr));
}

void FishingLayer::setFinishCallback(std::function<void(bool)> callback)
{
    finishCallback_ = callback;
}
