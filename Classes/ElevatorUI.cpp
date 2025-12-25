#include "ElevatorUI.h"

USING_NS_CC;

ElevatorUI* ElevatorUI::create()
{
    ElevatorUI* ret = new (std::nothrow) ElevatorUI();
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

bool ElevatorUI::init()
{
    if (!Layer::init())
        return false;

    // 设置全局 Z-order
    this->setGlobalZOrder(2000);

    inputText_ = "";

    // 创建UI组件
    createBackground();
    createPanel();
    
    // 隐藏原来的点击按钮，只显示列表信息
    // 我们可以直接禁用它们或者改变显示方式。
    // 为了满足需求 "锁定一切其他操作"，我们将 swallow touches。
    createFloorButtons();

    // 键盘监听（高优先级）
    auto keyListener = EventListenerKeyboard::create();
    keyListener->onKeyPressed = CC_CALLBACK_2(ElevatorUI::onKeyPressed, this);
    _eventDispatcher->addEventListenerWithSceneGraphPriority(keyListener, this);

    // 吞噬触摸事件，防止点击穿透，同时也为了锁定其他操作
    auto touchListener = EventListenerTouchOneByOne::create();
    touchListener->setSwallowTouches(true);
    touchListener->onTouchBegan = [](Touch* touch, Event* event) { return true; };
    _eventDispatcher->addEventListenerWithSceneGraphPriority(touchListener, this);

    CCLOG("ElevatorUI created");

    return true;
}

void ElevatorUI::createBackground()
{
    auto visibleSize = Director::getInstance()->getVisibleSize();

    // 半透明黑色背景
    backgroundLayer_ = LayerColor::create(Color4B(0, 0, 0, 200));
    this->addChild(backgroundLayer_, 0);
}

void ElevatorUI::createPanel()
{
    auto visibleSize = Director::getInstance()->getVisibleSize();
    Vec2 center = Vec2(visibleSize.width / 2, visibleSize.height / 2);

    // 绘制面板背景
    auto panelNode = DrawNode::create();
    Vec2 panelSize(400, 400); // 稍微加大
    Vec2 halfSize = panelSize / 2;

    Vec2 vertices[] = {
        Vec2(-halfSize.x, -halfSize.y),
        Vec2(halfSize.x, -halfSize.y),
        Vec2(halfSize.x, halfSize.y),
        Vec2(-halfSize.x, halfSize.y)
    };
    Color4F panelColor(0.1f, 0.1f, 0.2f, 0.95f);
    Color4F borderColor(0.8f, 0.8f, 0.8f, 1.0f);
    panelNode->drawPolygon(vertices, 4, panelColor, 3, borderColor);
    panelNode->setPosition(center);
    this->addChild(panelNode, 1);

    // 标题
    titleLabel_ = Label::createWithSystemFont("Enter Floor Number", "Arial", 28);
    titleLabel_->setPosition(Vec2(center.x, center.y + 160));
    titleLabel_->setColor(Color3B::YELLOW);
    this->addChild(titleLabel_, 2);

    // 输入显示框
    auto inputBg = DrawNode::create();
    inputBg->drawSolidRect(Vec2(-100, -25), Vec2(100, 25), Color4F(0,0,0,0.5f));
    inputBg->setPosition(Vec2(center.x, center.y + 110));
    this->addChild(inputBg, 2);

    inputLabel_ = Label::createWithSystemFont("", "Arial", 32);
    inputLabel_->setPosition(Vec2(center.x, center.y + 110));
    inputLabel_->setColor(Color3B::WHITE);
    this->addChild(inputLabel_, 3);

    // 提示
    auto hintLabel = Label::createWithSystemFont("Type number (0-5) then ENTER", "Arial", 16);
    hintLabel->setPosition(Vec2(center.x, center.y - 140));
    hintLabel->setColor(Color3B::GRAY);
    this->addChild(hintLabel, 2);
    
    auto escLabel = Label::createWithSystemFont("Press ESC to Exit", "Arial", 14);
    escLabel->setPosition(Vec2(center.x, center.y - 170));
    escLabel->setColor(Color3B::RED);
    this->addChild(escLabel, 2);
}

void ElevatorUI::createFloorButtons()
{
    auto visibleSize = Director::getInstance()->getVisibleSize();
    Vec2 center = Vec2(visibleSize.width / 2, visibleSize.height / 2);

    std::vector<std::string> floorNames = {
        u8"[0] farm",
        u8"[1]",
        u8"[2]",
        u8"[3]",
        u8"[4]",
        u8"[5]"
    };


    // 仅显示列表信息，不再是按钮（或者作为不可点击的列表展示）
    for (int i = 0; i < 6; ++i)
    {
        auto label = Label::createWithSystemFont(floorNames[i], "Arial", 20);
        label->setColor(Color3B::WHITE);
        label->setAnchorPoint(Vec2(0, 0.5));
        
        float yPos = center.y + 70 - i * 35; // 稍微调高起始点
        label->setPosition(Vec2(center.x - 80, yPos));

        this->addChild(label, 3);
    }
}

void ElevatorUI::setFloorSelectCallback(const std::function<void(int)>& callback)
{
    floorSelectCallback_ = callback;
}

void ElevatorUI::setCloseCallback(const std::function<void()>& callback)
{
    closeCallback_ = callback;
}

void ElevatorUI::show()
{
    // 淡入动画
    this->setOpacity(0);
    auto fadeIn = FadeIn::create(0.2f);
    this->runAction(fadeIn);
}

void ElevatorUI::close()
{
    // 淡出动画后移除
    auto fadeOut = FadeOut::create(0.2f);
    auto remove = CallFunc::create([this]() {
        if (closeCallback_)
        {
            closeCallback_();
        }
        this->removeFromParent();
    });
    auto sequence = Sequence::create(fadeOut, remove, nullptr);
    this->runAction(sequence);
}

void ElevatorUI::onFloorButtonClicked(Ref* sender, int floor)
{
    // 不再响应点击
}

void ElevatorUI::onCloseButtonClicked(Ref* sender)
{
    // 不再响应点击
}

void ElevatorUI::onKeyPressed(EventKeyboard::KeyCode keyCode, Event* event)
{
    // ESC 退出
    if (keyCode == EventKeyboard::KeyCode::KEY_ESCAPE)
    {
        close();
        return;
    }

    // Enter 确认
    if (keyCode == EventKeyboard::KeyCode::KEY_ENTER)
    {
        if (inputText_.empty()) return;
        
        try {
            int floor = std::stoi(inputText_);
            // 简单校验
            if (floor >= 0 && floor <= 5)
            {
                if (floorSelectCallback_)
                {
                    floorSelectCallback_(floor);
                }
                close();
            }
            else
            {
                // 无效楼层
                inputText_ = "";
                inputLabel_->setString("Invalid!");
                inputLabel_->setColor(Color3B::RED);
            }
        }
        catch (...)
        {
            inputText_ = "";
            inputLabel_->setString("Error");
        }
        return;
    }

    // Backspace 删除
    if (keyCode == EventKeyboard::KeyCode::KEY_BACKSPACE || 
        keyCode == EventKeyboard::KeyCode::KEY_DELETE)
    {
        if (!inputText_.empty())
        {
            inputText_.pop_back();
            inputLabel_->setString(inputText_);
            inputLabel_->setColor(Color3B::WHITE);
        }
        return;
    }

    // 数字键输入
    int number = -1;
    if (keyCode >= EventKeyboard::KeyCode::KEY_0 && keyCode <= EventKeyboard::KeyCode::KEY_9)
    {
        number = (int)keyCode - (int)EventKeyboard::KeyCode::KEY_0;
    }
    // 注意：部分 Cocos2d-x 版本可能使用不同的枚举名（如 KEY_NUMPAD0），
    // 或者根本没有定义小键盘枚举。为避免编译错误，暂时移除小键盘支持，
    // 或者如果确实需要，可以尝试 KEY_KP_ENTER (已确认有报错吗？图片只显示 KP_0/9)
    // 根据报错，KEY_KP_ENTER 可能也被报错，检查图片...
    // 图片显示 KEY_KP_0, KEY_KP_9 未定义。KEY_KP_ENTER 我们在上面用到了，
    // 需要检查是否也报错。虽然图片里没红线，但最好一起处理。
    // 为了安全，我们暂时只用主键盘数字键。
    
    if (number != -1)
    {
        // 限制长度，防止溢出，虽然只有几层
        if (inputText_.length() < 2) 
        {
            // 如果显示的还是错误提示，先清空
            if (inputLabel_->getString() == "Invalid!" || inputLabel_->getString() == "Error")
            {
                inputText_ = "";
                inputLabel_->setColor(Color3B::WHITE);
            }
            
            inputText_ += std::to_string(number);
            inputLabel_->setString(inputText_);
        }
    }
}
