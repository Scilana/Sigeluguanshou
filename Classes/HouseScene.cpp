#include "HouseScene.h"

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
    this->scheduleUpdate();
    return true;
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
    switch (keyCode)
    {
    case EventKeyboard::KeyCode::KEY_ESCAPE:
    case EventKeyboard::KeyCode::KEY_ENTER:
    case EventKeyboard::KeyCode::KEY_KP_ENTER:
        Director::getInstance()->popScene();
        break;
    default:
        break;
    }
}
