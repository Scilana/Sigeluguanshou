#include "MenuScene.h"
#include "GameScene.h"

USING_NS_CC;

Scene* MenuScene::createScene()
{
    return MenuScene::create();
}

bool MenuScene::init()
{
    if (!Scene::init())
        return false;

    CCLOG("Initializing Menu Scene with transparent buttons...");

    createBackground();
    createLogo();
    createMenuButtons();
    createDecorations();
    addAnimations();

    return true;
}

bool MenuScene::checkImageExists(const std::string& path)
{
    return FileUtils::getInstance()->isFileExist(path);
}

// ========== 创建背景 ==========

void MenuScene::createBackground()
{
    auto visibleSize = Director::getInstance()->getVisibleSize();
    Vec2 origin = Director::getInstance()->getVisibleOrigin();

    // 加载背景图片
    checkImageExists("images/backgrounds/menu_bg.png");
    auto bg = Sprite::create("images/backgrounds/menu_bg.png");
    if (bg) {
        bg->setPosition(Vec2(
            origin.x + visibleSize.width / 2,
            origin.y + visibleSize.height / 2
        ));

        // 缩放以适应屏幕
        float scaleX = visibleSize.width / bg->getContentSize().width;
        float scaleY = visibleSize.height / bg->getContentSize().height;
        float scale = MAX(scaleX, scaleY);
        bg->setScale(scale);

        this->addChild(bg, -10);
        CCLOG("Background image loaded successfully!");
        return;
    }
}

// ========== 创建Logo ==========

void MenuScene::createLogo()
{
    auto visibleSize = Director::getInstance()->getVisibleSize();
    Vec2 origin = Director::getInstance()->getVisibleOrigin();

    // 加载Logo图片
    checkImageExists("images/ui/logo.png");
    auto logo = Sprite::create("images/ui/logo.png");
    if (logo) {
        logo->setPosition(Vec2(
            origin.x + visibleSize.width / 2,
            origin.y + visibleSize.height * 0.75f
    ));

        float maxWidth = visibleSize.width * 1.0f;
        if (logo->getContentSize().width > maxWidth) {
            float scale = maxWidth / logo->getContentSize().width;
            logo->setScale(scale);
        }

        logo->setTag(1001);
        this->addChild(logo, 10);
        CCLOG("Logo image loaded successfully!");
        return;
    }
    
}

// ========== 创建半透明按钮 ==========

void MenuScene::createMenuButtons()
{
    auto visibleSize = Director::getInstance()->getVisibleSize();
    Vec2 origin = Director::getInstance()->getVisibleOrigin();

    // 创建半透明样式的按钮
    auto createTransparentButton = [](const std::string& text, const ccMenuCallback& callback) {
        // 按钮背景 - 半透明黑色
        // Alpha值: 180 = 约70%透明度, 可以调整 (0-255)
        auto normalBg = LayerColor::create(Color4B(0, 0, 0, 180), 320, 70);

        // 添加边框效果
        auto border = DrawNode::create();
        Vec2 rectangle[4] = {
            Vec2(0, 0),
            Vec2(320, 0),
            Vec2(320, 70),
            Vec2(0, 70)
        };
        Color4F borderColor(1.0f, 1.0f, 1.0f, 0.8f); // 白色边框
        border->drawPolygon(rectangle, 4, Color4F(0, 0, 0, 0), 2, borderColor);
        normalBg->addChild(border);

        // 按钮文字 - 白色，带阴影
        auto label = Label::createWithSystemFont(text, "Arial", 32);
        label->setPosition(Vec2(160, 35));
        label->setColor(Color3B::WHITE);
        label->enableShadow(Color4B(0, 0, 0, 200), Size(2, -2), 0);
        normalBg->addChild(label);

        // 悬停状态 - 稍微亮一些
        auto hoverBg = LayerColor::create(Color4B(255, 255, 255, 100), 320, 70);
        auto hoverBorder = DrawNode::create();
        hoverBorder->drawPolygon(rectangle, 4, Color4F(0, 0, 0, 0), 3, Color4F(1, 1, 1, 1));
        hoverBg->addChild(hoverBorder);

        auto hoverLabel = Label::createWithSystemFont(text, "Arial", 32);
        hoverLabel->setPosition(Vec2(160, 35));
        hoverLabel->setColor(Color3B::WHITE);
        hoverLabel->enableShadow(Color4B(0, 0, 0, 200), Size(2, -2), 0);
        hoverBg->addChild(hoverLabel);

        // 按下状态 - 更暗
        auto pressedBg = LayerColor::create(Color4B(0, 0, 0, 220), 320, 70);
        auto pressedBorder = DrawNode::create();
        pressedBorder->drawPolygon(rectangle, 4, Color4F(0, 0, 0, 0), 2, Color4F(0.8f, 0.8f, 0.8f, 0.8f));
        pressedBg->addChild(pressedBorder);

        auto pressedLabel = Label::createWithSystemFont(text, "Arial", 32);
        pressedLabel->setPosition(Vec2(160, 35));
        pressedLabel->setColor(Color3B(200, 200, 200));
        pressedLabel->enableShadow(Color4B(0, 0, 0, 200), Size(2, -2), 0);
        pressedBg->addChild(pressedLabel);

        // 创建菜单项
        auto button = MenuItemSprite::create(normalBg, hoverBg, pressedBg, callback);

        return button;
        };

    // 创建按钮
    auto startButton = createTransparentButton("Start Game",
        CC_CALLBACK_1(MenuScene::startGameCallback, this));
    startButton->setPosition(Vec2(
        origin.x + visibleSize.width / 2,
        origin.y + visibleSize.height * 0.45f
    ));

    auto continueButton = createTransparentButton("Continue",
        CC_CALLBACK_1(MenuScene::continueGameCallback, this));
    continueButton->setPosition(Vec2(
        origin.x + visibleSize.width / 2,
        origin.y + visibleSize.height * 0.36f
    ));

    auto settingsButton = createTransparentButton("Settings",
        CC_CALLBACK_1(MenuScene::settingsCallback, this));
    settingsButton->setPosition(Vec2(
        origin.x + visibleSize.width / 2,
        origin.y + visibleSize.height * 0.27f
    ));

    auto exitButton = createTransparentButton("Exit",
        CC_CALLBACK_1(MenuScene::exitGameCallback, this));
    exitButton->setPosition(Vec2(
        origin.x + visibleSize.width / 2,
        origin.y + visibleSize.height * 0.18f
    ));

    // 创建菜单
    auto menu = Menu::create(startButton, continueButton, settingsButton, exitButton, nullptr);
    menu->setPosition(Vec2::ZERO);
    this->addChild(menu, 20);

    CCLOG("Transparent buttons created!");
}

// ========== 创建装饰元素 ==========

void MenuScene::createDecorations()
{
    auto visibleSize = Director::getInstance()->getVisibleSize();
    Vec2 origin = Director::getInstance()->getVisibleOrigin();

    // 版本信息 - 半透明背景
    auto versionBg = LayerColor::create(Color4B(0, 0, 0, 150), 120, 30);
    versionBg->setPosition(Vec2(
        origin.x + visibleSize.width - 125,
        origin.y + 10
    ));
    this->addChild(versionBg, 29);

    auto version = Label::createWithSystemFont("v0.1.0 - Alpha", "Arial", 16);
    version->setPosition(Vec2(
        origin.x + visibleSize.width - 65,
        origin.y + 25
    ));
    version->setColor(Color3B::WHITE);
    version->enableShadow(Color4B(0, 0, 0, 200), Size(1, -1), 0);
    this->addChild(version, 30);

    // 制作信息 - 半透明背景
    auto creditsBg = LayerColor::create(Color4B(0, 0, 0, 150), 140, 30);
    creditsBg->setPosition(Vec2(origin.x + 10, origin.y + 10));
    this->addChild(creditsBg, 29);

    auto credits = Label::createWithSystemFont("Made with Love", "Arial", 16);
    credits->setPosition(Vec2(origin.x + 80, origin.y + 25));
    credits->setColor(Color3B::WHITE);
    credits->enableShadow(Color4B(0, 0, 0, 200), Size(1, -1), 0);
    this->addChild(credits, 30);
}

// ========== 添加动画 ==========

void MenuScene::addAnimations()
{
    // Logo漂浮动画
    auto logo = this->getChildByTag(1001);
    if (logo) {
        auto moveUp = MoveBy::create(2.0f, Vec2(0, 15));
        auto moveDown = moveUp->reverse();
        auto seq = Sequence::create(moveUp, moveDown, nullptr);
        logo->runAction(RepeatForever::create(seq));

        // 添加轻微的缩放效果
        auto scaleUp = ScaleTo::create(2.0f, 1.02f);
        auto scaleDown = ScaleTo::create(2.0f, 1.0f);
        auto scaleSeq = Sequence::create(scaleUp, scaleDown, nullptr);
        logo->runAction(RepeatForever::create(scaleSeq));
    }
}

// ========== 回调函数 ==========

void MenuScene::startGameCallback(Ref* sender)
{
    CCLOG("Start Game clicked!");

    auto visibleSize = Director::getInstance()->getVisibleSize();

    // Uncomment when GameScene is ready
    
    auto scene = GameScene::createScene();
    Director::getInstance()->replaceScene(
        TransitionFade::create(1.0f, scene)
    );
    
}

void MenuScene::continueGameCallback(Ref* sender)
{
    CCLOG("Continue clicked!");

    auto visibleSize = Director::getInstance()->getVisibleSize();

    auto messageBg = LayerColor::create(Color4B(0, 0, 0, 200), 450, 100);
    messageBg->setPosition(Vec2(
        (visibleSize.width - 450) / 2,
        (visibleSize.height - 100) / 2
    ));
    this->addChild(messageBg, 100);

    auto label = Label::createWithSystemFont("No saved game found!", "Arial", 36);
    label->setPosition(visibleSize.width / 2, visibleSize.height / 2);
    label->setColor(Color3B(255, 255, 100));
    label->enableShadow(Color4B(0, 0, 0, 255), Size(2, -2), 0);
    this->addChild(label, 101);

    auto removeAction = Sequence::create(
        DelayTime::create(1.0f),
        RemoveSelf::create(),
        nullptr
    );
    messageBg->runAction(removeAction);
    label->runAction(removeAction->clone());
}

void MenuScene::settingsCallback(Ref* sender)
{
    CCLOG("Settings clicked!");

    auto visibleSize = Director::getInstance()->getVisibleSize();

    auto messageBg = LayerColor::create(Color4B(0, 0, 0, 200), 450, 100);
    messageBg->setPosition(Vec2(
        (visibleSize.width - 450) / 2,
        (visibleSize.height - 100) / 2
    ));
    this->addChild(messageBg, 100);

    auto label = Label::createWithSystemFont("Settings Coming Soon!", "Arial", 36);
    label->setPosition(visibleSize.width / 2, visibleSize.height / 2);
    label->setColor(Color3B(100, 200, 255));
    label->enableShadow(Color4B(0, 0, 0, 255), Size(2, -2), 0);
    this->addChild(label, 101);

    auto removeAction = Sequence::create(
        DelayTime::create(1.0f),
        RemoveSelf::create(),
        nullptr
    );
    messageBg->runAction(removeAction);
    label->runAction(removeAction->clone());
}

void MenuScene::exitGameCallback(Ref* sender)
{
    CCLOG("Exit clicked!");
    Director::getInstance()->end();

#if (CC_TARGET_PLATFORM == CC_PLATFORM_IOS)
    exit(0);
#endif
}