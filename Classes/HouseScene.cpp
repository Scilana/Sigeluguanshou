#include "HouseScene.h"
#include "FarmManager.h"
#include "Player.h"

USING_NS_CC;

HouseScene* HouseScene::createScene()
{
    return HouseScene::create();
}

bool HouseScene::init()
{
    if (!Scene::init())
        return false;

    initBackground();
    initPlayer();
    initControls();
    initUI();
    this->scheduleUpdate();
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
}

void HouseScene::initPlayer()
{
    auto visibleSize = Director::getInstance()->getVisibleSize();
    Vec2 origin = Director::getInstance()->getVisibleOrigin();

    player_ = Player::create();
    if (!player_)
        return;

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
        float speedMultiplier = isSleeping_ ? 40.0f : 1.0f; // 40x speed when sleeping
        farmManager_->update(delta * speedMultiplier);
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
    if (isSleeping_) {
        if (keyCode == EventKeyboard::KeyCode::KEY_K) {
            isSleeping_ = false;
            if (player_) {
                player_->enableKeyboardControl();
                player_->recoverEnergy(270.0f); // 起床回满能量
            }
            CCLOG("Player woke up and recovered energy");
        }
        return; // Lock other keys
    }

    switch (keyCode)
    {
    case EventKeyboard::KeyCode::KEY_ESCAPE:
        Director::getInstance()->popScene();
        break;
    case EventKeyboard::KeyCode::KEY_J:
        {
            // Check if near bed (Bottom-Right area based on user feedback)
            auto visibleSize = Director::getInstance()->getVisibleSize();
            auto origin = Director::getInstance()->getVisibleOrigin();
            Vec2 bedPos(origin.x + visibleSize.width * 0.75f, origin.y + visibleSize.height * 0.25f);
            
            float dist = player_->getPosition().distance(bedPos);
            if (dist < 80.0f) {
                isSleeping_ = true;
                if (player_) player_->disableKeyboardControl();
                CCLOG("Player started sleeping");
            } else {
                CCLOG("Too far from bed: %.1f", dist);
            }
        }
        break;
    default:
        break;
    }
}
