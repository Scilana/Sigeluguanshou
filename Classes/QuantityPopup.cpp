#include "QuantityPopup.h"

USING_NS_CC;

QuantityPopup* QuantityPopup::create(int maxVal, const std::function<void(int)>& callback)
{
    QuantityPopup* ret = new (std::nothrow) QuantityPopup();
    if (ret && ret->init(maxVal, callback))
    {
        ret->autorelease();
        return ret;
    }
    CC_SAFE_DELETE(ret);
    return nullptr;
}

bool QuantityPopup::init(int maxVal, const std::function<void(int)>& callback)
{
    if (!Layer::init())
        return false;

    maxVal_ = maxVal;
    callback_ = callback;
    inputText_ = std::to_string(maxVal); // 默认全选

    createUI();

    auto keyListener = EventListenerKeyboard::create();
    keyListener->onKeyPressed = CC_CALLBACK_2(QuantityPopup::onKeyPressed, this);
    _eventDispatcher->addEventListenerWithSceneGraphPriority(keyListener, this);

    auto touchListener = EventListenerTouchOneByOne::create();
    touchListener->setSwallowTouches(true);
    touchListener->onTouchBegan = [](Touch*, Event*) { return true; };
    _eventDispatcher->addEventListenerWithSceneGraphPriority(touchListener, this);

    return true;
}

void QuantityPopup::createUI()
{
    auto visibleSize = Director::getInstance()->getVisibleSize();
    Vec2 center(visibleSize.width / 2, visibleSize.height / 2);

    // 半透明背景
    auto bg = LayerColor::create(Color4B(0, 0, 0, 180));
    this->addChild(bg);

    // 面板
    auto panel = DrawNode::create();
    panel->drawSolidRect(Vec2(-150, -100), Vec2(150, 100), Color4F(0.2f, 0.2f, 0.2f, 1.0f));
    panel->drawRect(Vec2(-150, -100), Vec2(150, 100), Color4F::WHITE);
    panel->setPosition(center);
    this->addChild(panel);

    titleLabel_ = Label::createWithSystemFont("Enter Quantity", "Arial", 20);
    titleLabel_->setPosition(Vec2(center.x, center.y + 60));
    this->addChild(titleLabel_);

    auto subTitle = Label::createWithSystemFont(StringUtils::format("(Max: %d)", maxVal_), "Arial", 14);
    subTitle->setPosition(Vec2(center.x, center.y + 35));
    subTitle->setColor(Color3B::GRAY);
    this->addChild(subTitle);

    inputLabel_ = Label::createWithSystemFont(inputText_, "Arial", 32);
    inputLabel_->setPosition(center);
    inputLabel_->setColor(Color3B::YELLOW);
    this->addChild(inputLabel_);

    auto hint = Label::createWithSystemFont("Press ENTER to Confirm\nESC to Cancel", "Arial", 12);
    hint->setPosition(Vec2(center.x, center.y - 60));
    hint->setAlignment(TextHAlignment::CENTER);
    this->addChild(hint);
}

void QuantityPopup::onKeyPressed(EventKeyboard::KeyCode keyCode, Event* event)
{
    if (keyCode == EventKeyboard::KeyCode::KEY_ESCAPE)
    {
        close();
        return;
    }

    if (keyCode == EventKeyboard::KeyCode::KEY_ENTER || keyCode == EventKeyboard::KeyCode::KEY_KP_ENTER)
    {
        submit();
        return;
    }

    if (keyCode >= EventKeyboard::KeyCode::KEY_0 && keyCode <= EventKeyboard::KeyCode::KEY_9)
    {
        int num = (int)keyCode - (int)EventKeyboard::KeyCode::KEY_0;
        if (inputText_ == "0") inputText_ = "";
        inputText_ += std::to_string(num);
        
        // 限制最大值
        try {
            int val = std::stoi(inputText_);
            if (val > maxVal_) inputText_ = std::to_string(maxVal_);
        } catch(...) {}
        
        inputLabel_->setString(inputText_);
        return;
    }

    if (keyCode == EventKeyboard::KeyCode::KEY_BACKSPACE)
    {
        if (!inputText_.empty()) {
            inputText_.pop_back();
            if (inputText_.empty()) inputText_ = "0";
            inputLabel_->setString(inputText_);
        }
    }
}

void QuantityPopup::submit()
{
    int val = 0;
    try {
        val = std::stoi(inputText_);
    } catch(...) {}
    
    if (val > 0) {
        if (callback_) callback_(std::min(val, maxVal_));
        close();
    }
}

void QuantityPopup::close()
{
    this->removeFromParent();
}
