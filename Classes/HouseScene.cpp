#include "HouseScene.h"
#include "FarmManager.h"
#include "Player.h"
#include "GameScene.h"
#include "InventoryUI.h"
#include "InventoryManager.h"

USING_NS_CC;

HouseScene* HouseScene::createScene(bool isPassedOut)
{

    HouseScene* ret = new (std::nothrow) HouseScene();
    if (ret && ret->init(isPassedOut))
    {
        ret->autorelease();
        return ret;
    }
    CC_SAFE_DELETE(ret);
    return nullptr;
}

bool HouseScene::init()
{
    return init(false);
}

bool HouseScene::init(bool isPassedOut)
{
    if (!Scene::init())
        return false;

    isPassedOut_ = isPassedOut;
    // ... rest of init

    initBackground();
    initPlayer();
    initControls();
    initUI();
    this->scheduleUpdate();

    // 初始化背包引用
    inventory_ = InventoryManager::getInstance();

    if (isPassedOut_)
    {
        // 如果是晕倒模式，直接开始睡觉流程
        isSleeping_ = true;
        if (player_) {
            player_->setVisible(false);
            player_->disableKeyboardControl();
        }
        if (sleepSprite_) sleepSprite_->setVisible(true);
        CCLOG("Player passed out - starting in sleep mode");
    }
    else
    {
        // 正常进入，生成在门口
        if (player_) player_->setPosition(Vec2(220, 100)); // 门口附近
        if (sleepSprite_) sleepSprite_->setVisible(false);
    }

    return true;
}

void HouseScene::initUI()
{
    auto visibleSize = Director::getInstance()->getVisibleSize();
    Vec2 origin = Director::getInstance()->getVisibleOrigin();

    timeLabel_ = Label::createWithSystemFont("Time: --:--", "Arial", 24);
    timeLabel_->setPosition(Vec2(origin.x + visibleSize.width * 0.5f, origin.y + visibleSize.height - 30));
    timeLabel_->setColor(Color3B::WHITE);
    this->addChild(timeLabel_, 10);
}

void HouseScene::updateUI()
{
    if (farmManager_ && timeLabel_)
    {
        std::string timeStr = StringUtils::format("Day %d, %02d:%02d", 
            farmManager_->getDayCount(), 
            farmManager_->getHour(), 
            farmManager_->getMinute());
        timeLabel_->setString(timeStr);
        
        if (isSleeping_) {
            timeLabel_->setString(timeStr + " (Sleeping...)");
            timeLabel_->setColor(Color3B::YELLOW);
        } else {
            timeLabel_->setColor(Color3B::WHITE);
        }
    }
    else if (isPassedOut_ && timeLabel_)
    {
        // Mock time display for passed out scene without FarmManager
        // 假设睡3秒，进度从 0% -> 100% (Midnight -> 6AM)
        // 6AM = 6:00
        // We can just show "Sleep..." or static "Next Day"
        timeLabel_->setString("Recovering energy... (Next Day 6:00)");
        timeLabel_->setColor(Color3B::YELLOW);
    }
}

// 【新增】起床核心逻辑
void HouseScene::wakeUp()
{
    if (!isSleeping_) return;

    // 1. 标记状态为清醒
    isSleeping_ = false;

    // 2. 玩家角色：显示、恢复控制、回满体力
    if (player_) {
        player_->setVisible(true);        // ★ 显示站立的玩家
        player_->enableKeyboardControl(); // 恢复键盘
        player_->recoverEnergy(270.0f);   // 回满体力
    }

    // 3. 睡觉贴图：隐藏
    if (sleepSprite_) {
        sleepSprite_->setVisible(false);  // ★ 去除睡觉图
    }

    CCLOG("Player woke up! (Sprites toggled)");
}

void HouseScene::toggleInventory()
{
    if (inventoryUI_)
    {
        inventoryUI_->close();
        return;
    }

    if (!inventory_) return;

    inventoryUI_ = InventoryUI::create(inventory_);
    if (inventoryUI_)
    {
        inventoryUI_->setCloseCallback(CC_CALLBACK_0(HouseScene::onInventoryClosed, this));
        this->addChild(inventoryUI_, 100); // High Z-order
        inventoryUI_->show();
    }
}

void HouseScene::onInventoryClosed()
{
    inventoryUI_ = nullptr;
}

void HouseScene::initBackground()
{
    auto visibleSize = Director::getInstance()->getVisibleSize();
    Vec2 origin = Director::getInstance()->getVisibleOrigin();

    background_ = Sprite::create("myhouse/Myhouse.png");
    if (!background_)
        return;

    float scaleX = visibleSize.width / background_->getContentSize().width;
    float scaleY = visibleSize.height / background_->getContentSize().height;
    float scale = MIN(scaleX, scaleY);

    background_->setPosition(Vec2(
        origin.x + visibleSize.width / 2,
        origin.y + visibleSize.height / 2
    ));
    background_->setScale(scale);
    this->addChild(background_, 0);

    sleepSprite_ = Sprite::create("myhouse/sleeping.png");
    if (sleepSprite_)
    {
        // 1. 设置中心点位置
        sleepSprite_->setPosition(Vec2(180.5f, 77.5f));

        // 2. (可选) 如果你的图片尺寸不是 53x55，可以强制缩放适应这个框
         float targetW = 53.0f;
         float targetH = 55.0f;
         sleepSprite_->setScaleX(targetW / sleepSprite_->getContentSize().width);
         sleepSprite_->setScaleY(targetH / sleepSprite_->getContentSize().height);

        // 3. 初始隐藏
        sleepSprite_->setVisible(false);

        // 4. 加到背景上
        background_->addChild(sleepSprite_, 10);
    }
}

void HouseScene::initPlayer()
{
    auto visibleSize = Director::getInstance()->getVisibleSize();
    Vec2 origin = Director::getInstance()->getVisibleOrigin();

    player_ = Player::create();
    if (!player_)
        return;

    player_->setScale(0.9f);

    player_->setPosition(Vec2(
        origin.x + visibleSize.width * 0.5f,
        origin.y + visibleSize.height * 0.2f
    ));
    player_->enableKeyboardControl();
    this->addChild(player_, 1);
}

void HouseScene::update(float delta)
{
    if (!player_ || !background_)
        return;

    // Handle time passing even if GameScene is paused
    if (farmManager_) {
        float speedMultiplier = isSleeping_ ? 20.0f : 1.0f; // 40x speed when sleeping
        farmManager_->update(delta * speedMultiplier);

        //自动起床
        if (isSleeping_ && farmManager_->getHour() == 6)
        {
            CCLOG("Alarm clock! 6:00 AM reached.");
            wakeUp(); // 时间到了，自动起床！
        }
    }
    else if (isPassedOut_)
    {
        // 如果是从矿洞晕倒回来的，没有 FarmManager，使用本地模拟时间
        // 假设晕倒时是午夜 (0:00)，我们需要睡到 6:00
        // 午夜对应 localTime = 0. 或者我们只需要一个简单的计时器
        // 简单处理：让它睡 2 秒钟然后醒来，假装到了早上
        
        static float sleepTimer = 0.0f;
        sleepTimer += delta;
        if (sleepTimer > 3.0f) // 睡3秒
        {
            sleepTimer = 0.0f;
            wakeUp();
        }
    }
    
    updateUI();

    if (isSleeping_) return; // Lock movement when sleeping

    Rect bounds = background_->getBoundingBox();
    float margin = 6.0f;
    float halfW = player_->getContentSize().width * player_->getScaleX() * 0.5f;
    float halfH = player_->getContentSize().height * player_->getScaleY() * 0.5f;

    float minX = bounds.getMinX() + halfW + margin;
    float maxX = bounds.getMaxX() - halfW - margin;
    float minY = bounds.getMinY() + halfH + margin;
    float maxY = bounds.getMaxY() - halfH - margin;

    Vec2 pos = player_->getPosition();
    pos.x = clampf(pos.x, minX, maxX);
    pos.y = clampf(pos.y, minY, maxY);
    player_->setPosition(pos);
}

void HouseScene::initControls()
{
    auto keyListener = EventListenerKeyboard::create();
    keyListener->onKeyPressed = CC_CALLBACK_2(HouseScene::onKeyPressed, this);
    _eventDispatcher->addEventListenerWithSceneGraphPriority(keyListener, this);
}

void HouseScene::onKeyPressed(EventKeyboard::KeyCode keyCode, Event* event)
{
    // 如果正在睡觉
    if (isSleeping_) {
        // 按 K 键手动起床
        if (keyCode == EventKeyboard::KeyCode::KEY_K) {
            wakeUp(); // 直接调用我们刚才写的函数
        }
        return; // 锁定其他按键
    }

    switch (keyCode)
    {
    case EventKeyboard::KeyCode::KEY_J:
    {
        if (!background_ || !player_) break;

        // 1. 获取玩家在背景图内的坐标
        Vec2 playerPosLocal = background_->convertToNodeSpace(player_->getPosition());

        // 2. 定义触发区域 (Trigger Area)
        // 只在 X: 154~177 范围内触发
        // Y范围保持和床一样: 50~105
        float triggerX = 154.0f;
        float triggerY = 50.0f;
        float triggerW = 177.0f - 154.0f; // 23 像素宽
        float triggerH = 105.0f - 50.0f;  // 55 像素高

        Rect triggerArea(triggerX, triggerY, triggerW, triggerH);

        // 3. 判定
        if (triggerArea.containsPoint(playerPosLocal))
        {
            isSleeping_ = true;
            player_->disableKeyboardControl();
            player_->setVisible(false);     // 隐藏玩家

            if (sleepSprite_) {
                sleepSprite_->setVisible(true); // 显示刚才定好位置的贴图
            }

            CCLOG("Player sleeping! Pos: (%.1f, %.1f)", playerPosLocal.x, playerPosLocal.y);
        }
        else
        {
            CCLOG("Not in sleep area. Pos: (%.1f, %.1f)", playerPosLocal.x, playerPosLocal.y);
        }
    }
    break;

    case EventKeyboard::KeyCode::KEY_ENTER:
    {
        if (!background_ || !player_) break;

        // 1. 获取玩家在背景图内的坐标
        Vec2 playerPosLocal = background_->convertToNodeSpace(player_->getPosition());

        // 2. 定义区域
        // 原坐标(画图软件): x[61, 97], y[170, 190]
        // 转换后(Cocos):   x[61, 97], y[2, 22]   <-- 也就是图片的最底端

        // Rect(x, y, width, height)
        // x = 61
        // y = 2
        // width = 97 - 61 = 36
        // height = 22 - 2 = 20
        Rect doorArea(61.0f, 2.0f, 36.0f, 20.0f);

        // 3. 判定
        if (doorArea.containsPoint(playerPosLocal))
        {
            CCLOG("Exiting house...");
            if (isPassedOut_)
            {
                Director::getInstance()->replaceScene(TransitionFade::create(0.5f, GameScene::createScene()));
            }
            else
            {
                Director::getInstance()->popScene();
            }
        }
        else
        {
            // 调试日志：显示玩家当前坐标 vs 门的位置
            CCLOG("Pos: (%.1f, %.1f) - Target Y is roughly 2~22", playerPosLocal.x, playerPosLocal.y);
        }
    }
    break;
    case EventKeyboard::KeyCode::KEY_B:
    case EventKeyboard::KeyCode::KEY_I:
    case EventKeyboard::KeyCode::KEY_TAB:
        toggleInventory();
        break;
        
    default:
        break;
    }
}
