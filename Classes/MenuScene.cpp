#include "MenuScene.h"
#include "GameScene.h"
#include "SaveManager.h"
#include "SimpleAudioEngine.h"

USING_NS_CC;

Scene* MenuScene::createScene()
{
    return MenuScene::create();
}

bool MenuScene::init()
{
    if (!Scene::init())
        return false;

    CCLOG("Initializing Menu Scene with start screen assets...");

    auto audio = CocosDenshion::SimpleAudioEngine::getInstance();
    if (!audio->isBackgroundMusicPlaying()) {
        audio->playBackgroundMusic("MUSIC/01 - Stardew Valley Overture.mp3", true);
    }

    createBackground();
    createDecorations();
    createLogo();
    createMenuButtons();
    addAnimations();

    return true;
}

bool MenuScene::checkImageExists(const std::string& path)
{
    return FileUtils::getInstance()->isFileExist(path);
}

cocos2d::MenuItemImage* MenuScene::createImageButton(
    const std::string& normalImage,
    const std::string& selectedImage,
    const cocos2d::ccMenuCallback& callback
)
{
    if (!checkImageExists(normalImage))
        return nullptr;

    std::string selectedPath = selectedImage;
    if (!checkImageExists(selectedImage))
        selectedPath = normalImage;

    auto button = MenuItemImage::create(normalImage, selectedPath, callback);
    return button;
}

// ========== Background ==========

void MenuScene::createBackground()
{
    auto visibleSize = Director::getInstance()->getVisibleSize();
    Vec2 origin = Director::getInstance()->getVisibleOrigin();

    auto bg = Sprite::create("ui/start/background1.png");
    if (!bg) {
        bg = Sprite::create("images/backgrounds/menu_bg.png");
    }
    if (!bg)
        return;

    float scaleX = visibleSize.width / bg->getContentSize().width;
    float scaleY = visibleSize.height / bg->getContentSize().height;
    float scale = MAX(scaleX, scaleY);

    bg->setPosition(Vec2(
        origin.x + visibleSize.width / 2,
        origin.y + visibleSize.height / 2
    ));
    bg->setScale(scale);
    this->addChild(bg, -20);

    auto overlay = Sprite::create("ui/start/background2.png");
    if (overlay) {
        overlay->setPosition(Vec2(
            origin.x + visibleSize.width / 2,
            origin.y + visibleSize.height / 2
        ));
        overlay->setScale(scale);
        overlay->setOpacity(200);
        this->addChild(overlay, -19);
    }

    auto overlayHalf = Sprite::create("ui/start/background2-half.png");
    if (overlayHalf) {
        float overlayScale = visibleSize.width / overlayHalf->getContentSize().width;
        overlayHalf->setAnchorPoint(Vec2(0.5f, 1.0f));
        overlayHalf->setPosition(Vec2(
            origin.x + visibleSize.width / 2,
            origin.y + visibleSize.height
        ));
        overlayHalf->setScale(overlayScale);
        overlayHalf->setOpacity(220);
        this->addChild(overlayHalf, -18);
    }
}

// ========== Logo ==========

void MenuScene::createLogo()
{
    auto visibleSize = Director::getInstance()->getVisibleSize();
    Vec2 origin = Director::getInstance()->getVisibleOrigin();

    auto logo = Sprite::create("ui/start/title.png");
    if (!logo) {
        logo = Sprite::create("images/ui/logo.png");
    }
    if (!logo)
        return;

    float targetWidth = visibleSize.width * 0.6f;
    float scale = targetWidth / logo->getContentSize().width;

    logo->setPosition(Vec2(
        origin.x + visibleSize.width / 2,
        origin.y + visibleSize.height * 0.72f
    ));
    logo->setScale(scale);
    logo->setTag(1001);
    this->addChild(logo, 10);
}

// ========== Menu Buttons ==========

void MenuScene::createMenuButtons()
{
    auto visibleSize = Director::getInstance()->getVisibleSize();
    Vec2 origin = Director::getInstance()->getVisibleOrigin();

    const std::string startNormal = "ui/start/start1.png";
    const std::string startSelected = "ui/start/start2.png";
    const std::string loadNormal = "ui/start/load1.png";
    const std::string loadSelected = "ui/start/load2.png";
    const std::string coopNormal = "ui/start/coop1.png";
    const std::string coopSelected = "ui/start/coop2.png";
    const std::string quitNormal = "ui/start/quit1.png";
    const std::string quitSelected = "ui/start/quit2.png";

    auto sample = Sprite::create(startNormal);
    float buttonScale = 1.0f;
    float buttonWidth = visibleSize.width * 0.18f;
    if (sample) {
        float targetButtonWidth = visibleSize.width * 0.18f;
        buttonScale = targetButtonWidth / sample->getContentSize().width;
        buttonWidth = sample->getContentSize().width * buttonScale;
    }

    float gap = visibleSize.width * 0.035f;
    float totalWidth = buttonWidth * 4 + gap * 3;
    float startX = origin.x + (visibleSize.width - totalWidth) / 2 + buttonWidth / 2;
    float y = origin.y + visibleSize.height * 0.18f;

    auto startButton = createImageButton(startNormal, startSelected,
        CC_CALLBACK_1(MenuScene::startGameCallback, this));
    auto loadButton = createImageButton(loadNormal, loadSelected,
        CC_CALLBACK_1(MenuScene::continueGameCallback, this));
    auto coopButton = createImageButton(coopNormal, coopSelected,
        CC_CALLBACK_1(MenuScene::coopCallback, this));
    auto quitButton = createImageButton(quitNormal, quitSelected,
        CC_CALLBACK_1(MenuScene::exitGameCallback, this));

    auto menu = Menu::create();
    menu->setPosition(Vec2::ZERO);
    this->addChild(menu, 20);

    if (startButton) {
        startButton->setScale(buttonScale);
        startButton->setPosition(Vec2(startX, y));
        menu->addChild(startButton);
    }

    if (loadButton) {
        loadButton->setScale(buttonScale);
        loadButton->setPosition(Vec2(startX + (buttonWidth + gap) * 1, y));
        menu->addChild(loadButton);
    }

    if (coopButton) {
        coopButton->setScale(buttonScale);
        coopButton->setPosition(Vec2(startX + (buttonWidth + gap) * 2, y));
        menu->addChild(coopButton);
    }

    if (quitButton) {
        quitButton->setScale(buttonScale);
        quitButton->setPosition(Vec2(startX + (buttonWidth + gap) * 3, y));
        menu->addChild(quitButton);
    }
}

// ========== Decorations ==========

void MenuScene::createDecorations()
{
    auto visibleSize = Director::getInstance()->getVisibleSize();
    Vec2 origin = Director::getInstance()->getVisibleOrigin();

    float decorScale = visibleSize.width / 1920.0f;

    auto clouds = Sprite::create("ui/start/Clouds.png");
    if (clouds) {
        clouds->setPosition(Vec2(
            origin.x + visibleSize.width * 0.72f,
            origin.y + visibleSize.height * 0.78f
        ));
        clouds->setScale(decorScale * 1.4f);
        clouds->setTag(2001);
        this->addChild(clouds, -5);
    }

    auto cloud = Sprite::create("ui/start/Cloud.png");
    if (cloud) {
        cloud->setPosition(Vec2(
            origin.x + visibleSize.width * 0.85f,
            origin.y + visibleSize.height * 0.62f
        ));
        cloud->setScale(decorScale * 1.1f);
        cloud->setTag(2002);
        this->addChild(cloud, -6);
    }
}

// ========== Animations ==========

void MenuScene::addAnimations()
{
    auto logo = this->getChildByTag(1001);
    if (logo) {
        auto moveUp = MoveBy::create(2.5f, Vec2(0, 12));
        auto moveDown = moveUp->reverse();
        auto seq = Sequence::create(moveUp, moveDown, nullptr);
        logo->runAction(RepeatForever::create(seq));

        auto scaleUp = ScaleTo::create(2.5f, logo->getScale() * 1.02f);
        auto scaleDown = ScaleTo::create(2.5f, logo->getScale());
        auto scaleSeq = Sequence::create(scaleUp, scaleDown, nullptr);
        logo->runAction(RepeatForever::create(scaleSeq));
    }

    auto clouds = this->getChildByTag(2001);
    if (clouds) {
        auto drift = MoveBy::create(8.0f, Vec2(35, 0));
        auto driftBack = drift->reverse();
        auto seq = Sequence::create(drift, driftBack, nullptr);
        clouds->runAction(RepeatForever::create(seq));
    }

    auto cloud = this->getChildByTag(2002);
    if (cloud) {
        auto drift = MoveBy::create(10.0f, Vec2(25, 0));
        auto driftBack = drift->reverse();
        auto seq = Sequence::create(drift, driftBack, nullptr);
        cloud->runAction(RepeatForever::create(seq));
    }
}

// ========== Callbacks ==========

void MenuScene::startGameCallback(Ref* sender)
{
    CCLOG("Start Game clicked!");

    // 检查是否已有存档
    if (SaveManager::getInstance()->hasSaveFile())
    {
        CCLOG("Warning: Existing save file will be overwritten on first save");
        // 可以选择删除旧存档或提示用户
        // SaveManager::getInstance()->deleteSaveFile();
    }

    // 创建新游戏（不加载存档）
    auto scene = GameScene::createScene();
    Director::getInstance()->replaceScene(
        TransitionFade::create(1.0f, scene)
    );
}

void MenuScene::continueGameCallback(Ref* sender)
{
    CCLOG("Continue Game clicked!");

    auto visibleSize = Director::getInstance()->getVisibleSize();

    // 检查是否有存档文件
    if (!SaveManager::getInstance()->hasSaveFile())
    {
        // 没有存档，显示提示信息
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
            DelayTime::create(1.5f),
            RemoveSelf::create(),
            nullptr
        );
        messageBg->runAction(removeAction);
        label->runAction(removeAction->clone());

        return;
    }

    // 有存档，加载游戏
    CCLOG("Loading saved game...");
    auto scene = GameScene::createScene(true);  // true 表示从存档加载
    Director::getInstance()->replaceScene(
        TransitionFade::create(1.0f, scene)
    );
}

void MenuScene::coopCallback(Ref* sender)
{
    CCLOG("Co-op clicked!");

    auto visibleSize = Director::getInstance()->getVisibleSize();

    auto messageBg = LayerColor::create(Color4B(0, 0, 0, 200), 520, 100);
    messageBg->setPosition(Vec2(
        (visibleSize.width - 520) / 2,
        (visibleSize.height - 100) / 2
    ));
    this->addChild(messageBg, 100);

    auto label = Label::createWithSystemFont("Co-op Coming Soon!", "Arial", 36);
    label->setPosition(visibleSize.width / 2, visibleSize.height / 2);
    label->setColor(Color3B(120, 220, 120));
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
