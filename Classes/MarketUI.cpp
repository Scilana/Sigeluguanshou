#include "MarketUI.h"
#include "FarmManager.h"
#include "InventoryManager.h"
#include "MarketState.h"
#include <algorithm>

USING_NS_CC;

MarketUI* MarketUI::create(InventoryManager* inventory, MarketState* marketState, FarmManager* farmManager)
{
    MarketUI* ret = new (std::nothrow) MarketUI();
    if (ret && ret->init(inventory, marketState, farmManager))
    {
        ret->autorelease();
        return ret;
    }
    delete ret;
    return nullptr;
}

bool MarketUI::init(InventoryManager* inventory, MarketState* marketState, FarmManager* farmManager)
{
    if (!Layer::init())
        return false;

    inventory_ = inventory;
    marketState_ = marketState;
    farmManager_ = farmManager;

    if (!inventory_ || !marketState_) {
        CCLOG("ERROR: MarketUI requires InventoryManager and MarketState.");
        return false;
    }

    initBackground();
    initPanel();
    initLists();
    initControls();

    auto keyListener = EventListenerKeyboard::create();
    keyListener->onKeyPressed = CC_CALLBACK_2(MarketUI::onKeyPressed, this);
    _eventDispatcher->addEventListenerWithSceneGraphPriority(keyListener, this);

    scheduleUpdate();
    refresh();
    return true;
}

void MarketUI::initBackground()
{
    auto visibleSize = Director::getInstance()->getVisibleSize();
    background_ = LayerColor::create(Color4B(0, 0, 0, 180), visibleSize.width, visibleSize.height);
    addChild(background_, 0);

    auto listener = EventListenerTouchOneByOne::create();
    listener->setSwallowTouches(true);
    listener->onTouchBegan = [](Touch* touch, Event* event) {
        return true;
    };
    _eventDispatcher->addEventListenerWithSceneGraphPriority(listener, background_);
}

void MarketUI::initPanel()
{
    auto visibleSize = Director::getInstance()->getVisibleSize();
    float panelWidth = std::min(visibleSize.width * 0.9f, 820.0f);
    float panelHeight = std::min(visibleSize.height * 0.82f, 520.0f);

    panel_ = LayerColor::create(Color4B(40, 35, 30, 255), panelWidth, panelHeight);
    panel_->setPosition(Vec2(
        (visibleSize.width - panelWidth) / 2,
        (visibleSize.height - panelHeight) / 2
    ));
    addChild(panel_, 1);

    auto border = DrawNode::create();
    border->drawRect(Vec2(0, 0), Vec2(panelWidth, panelHeight), Color4F(0.8f, 0.7f, 0.5f, 1.0f));
    border->setLineWidth(3);
    panel_->addChild(border, 0);

    auto title = Label::createWithSystemFont("Market", "Arial", 32);
    title->setPosition(Vec2(panelWidth / 2, panelHeight - 35));
    title->setColor(Color3B(255, 235, 200));
    panel_->addChild(title, 1);

    dayLabel_ = Label::createWithSystemFont("Day: 1", "Arial", 18);
    dayLabel_->setAnchorPoint(Vec2(0, 0.5f));
    dayLabel_->setPosition(Vec2(20, panelHeight - 35));
    dayLabel_->setColor(Color3B(200, 200, 200));
    panel_->addChild(dayLabel_, 1);

    seasonLabel_ = Label::createWithSystemFont("Season: Spring", "Arial", 16);
    seasonLabel_->setAnchorPoint(Vec2(0, 0.5f));
    seasonLabel_->setPosition(Vec2(20, panelHeight - 60));
    seasonLabel_->setColor(Color3B(200, 200, 200));
    panel_->addChild(seasonLabel_, 1);

    weatherLabel_ = Label::createWithSystemFont("Weather: Sunny", "Arial", 16);
    weatherLabel_->setAnchorPoint(Vec2(0, 0.5f));
    weatherLabel_->setPosition(Vec2(20, panelHeight - 80));
    weatherLabel_->setColor(Color3B(200, 200, 200));
    panel_->addChild(weatherLabel_, 1);

    moneyLabel_ = Label::createWithSystemFont("Gold: 0", "Arial", 24);
    moneyLabel_->setAnchorPoint(Vec2(1, 0.5f));
    moneyLabel_->setPosition(Vec2(panelWidth - 20, panelHeight - 35));
    moneyLabel_->setColor(Color3B(255, 215, 0));
    panel_->addChild(moneyLabel_, 1);

    infoLabel_ = Label::createWithSystemFont("", "Arial", 18);
    infoLabel_->setPosition(Vec2(panelWidth / 2, 55));
    infoLabel_->setColor(Color3B(200, 200, 200));
    panel_->addChild(infoLabel_, 1);

    statusLabel_ = Label::createWithSystemFont("", "Arial", 18);
    statusLabel_->setPosition(Vec2(panelWidth / 2, 30));
    statusLabel_->setColor(Color3B(255, 235, 200));
    panel_->addChild(statusLabel_, 1);
}

void MarketUI::initLists()
{
    float panelWidth = panel_->getContentSize().width;
    float panelHeight = panel_->getContentSize().height;

    float leftX = 40.0f;
    float rightX = panelWidth / 2 + 20.0f;
    float listTop = panelHeight - 125.0f;
    float rowHeight = 28.0f;

    buyTitle_ = Label::createWithSystemFont("Buy Seeds", "Arial", 20);
    buyTitle_->setAnchorPoint(Vec2(0, 0.5f));
    buyTitle_->setPosition(Vec2(leftX, panelHeight - 100));
    buyTitle_->setColor(Color3B(220, 220, 220));
    panel_->addChild(buyTitle_, 1);

    sellTitle_ = Label::createWithSystemFont("Sell Crops", "Arial", 20);
    sellTitle_->setAnchorPoint(Vec2(0, 0.5f));
    sellTitle_->setPosition(Vec2(rightX, panelHeight - 100));
    sellTitle_->setColor(Color3B(220, 220, 220));
    panel_->addChild(sellTitle_, 1);

    const auto& buyGoods = marketState_->getBuyGoods();
    const auto& sellGoods = marketState_->getSellGoods();

    buyLabels_.clear();
    sellLabels_.clear();

    for (size_t i = 0; i < buyGoods.size(); ++i)
    {
        auto label = Label::createWithSystemFont("", "Arial", 18);
        label->setAnchorPoint(Vec2(0, 0.5f));
        label->setPosition(Vec2(leftX, listTop - rowHeight * i));
        panel_->addChild(label, 1);
        buyLabels_.push_back(label);
    }

    for (size_t i = 0; i < sellGoods.size(); ++i)
    {
        auto label = Label::createWithSystemFont("", "Arial", 18);
        label->setAnchorPoint(Vec2(0, 0.5f));
        label->setPosition(Vec2(rightX, listTop - rowHeight * i));
        panel_->addChild(label, 1);
        sellLabels_.push_back(label);
    }
}

void MarketUI::initControls()
{
    auto hint = Label::createWithSystemFont(
        "Up/Down: Select  Left/Right: Switch  Enter: Trade  P/Esc: Close",
        "Arial",
        16
    );
    hint->setPosition(Vec2(panel_->getContentSize().width / 2, 10));
    hint->setColor(Color3B(150, 150, 150));
    panel_->addChild(hint, 1);
}

void MarketUI::refresh()
{
    if (!inventory_ || !marketState_)
        return;

    int dayCount = 1;
    if (farmManager_) {
        dayCount = farmManager_->getDayCount();
    }
    lastDay_ = dayCount;
    marketState_->updatePrices(dayCount);

    moneyLabel_->setString(StringUtils::format("Gold: %d", inventory_->getMoney()));
    dayLabel_->setString(StringUtils::format("Day: %d", dayCount));
    seasonLabel_->setString(StringUtils::format("Season: %s", marketState_->getSeasonName().c_str()));
    weatherLabel_->setString(StringUtils::format("Weather: %s", marketState_->getWeatherName().c_str()));

    const auto& buyGoods = marketState_->getBuyGoods();
    for (size_t i = 0; i < buyLabels_.size() && i < buyGoods.size(); ++i)
    {
        std::string name = InventoryManager::getItemName(buyGoods[i].itemType);
        buyLabels_[i]->setString(StringUtils::format("%s - %dG", name.c_str(), buyGoods[i].currentPrice));
    }

    const auto& sellGoods = marketState_->getSellGoods();
    for (size_t i = 0; i < sellLabels_.size() && i < sellGoods.size(); ++i)
    {
        std::string name = InventoryManager::getItemName(sellGoods[i].itemType);
        sellLabels_[i]->setString(StringUtils::format("%s - %dG", name.c_str(), sellGoods[i].currentPrice));
    }

    updateHighlight();
    updateInfo();
}

void MarketUI::updateHighlight()
{
    for (size_t i = 0; i < buyLabels_.size(); ++i)
    {
        bool selected = (focus_ == FocusList::Buy && static_cast<int>(i) == selectedBuy_);
        buyLabels_[i]->setColor(selected ? Color3B(255, 215, 0) : Color3B(210, 210, 210));
    }

    for (size_t i = 0; i < sellLabels_.size(); ++i)
    {
        bool selected = (focus_ == FocusList::Sell && static_cast<int>(i) == selectedSell_);
        sellLabels_[i]->setColor(selected ? Color3B(255, 215, 0) : Color3B(210, 210, 210));
    }

    buyTitle_->setColor(focus_ == FocusList::Buy ? Color3B(255, 235, 200) : Color3B(180, 180, 180));
    sellTitle_->setColor(focus_ == FocusList::Sell ? Color3B(255, 235, 200) : Color3B(180, 180, 180));
}

void MarketUI::updateInfo()
{
    const auto& buyGoods = marketState_->getBuyGoods();
    const auto& sellGoods = marketState_->getSellGoods();

    if (focus_ == FocusList::Buy && !buyGoods.empty())
    {
        int index = std::min(selectedBuy_, static_cast<int>(buyGoods.size() - 1));
        const auto& good = buyGoods[index];
        std::string name = InventoryManager::getItemName(good.itemType);
        int owned = inventory_->getItemCount(good.itemType);
        infoLabel_->setString(StringUtils::format("Buy %s for %dG (You have %d)", name.c_str(), good.currentPrice, owned));
    }
    else if (focus_ == FocusList::Sell && !sellGoods.empty())
    {
        int index = std::min(selectedSell_, static_cast<int>(sellGoods.size() - 1));
        const auto& good = sellGoods[index];
        std::string name = InventoryManager::getItemName(good.itemType);
        int owned = inventory_->getItemCount(good.itemType);
        infoLabel_->setString(StringUtils::format("Sell %s for %dG (You have %d)", name.c_str(), good.currentPrice, owned));
    }
    else
    {
        infoLabel_->setString("");
    }
}

void MarketUI::moveSelection(int delta)
{
    if (focus_ == FocusList::Buy)
    {
        int size = static_cast<int>(marketState_->getBuyGoods().size());
        if (size == 0) return;
        selectedBuy_ = (selectedBuy_ + delta + size) % size;
    }
    else
    {
        int size = static_cast<int>(marketState_->getSellGoods().size());
        if (size == 0) return;
        selectedSell_ = (selectedSell_ + delta + size) % size;
    }

    updateHighlight();
    updateInfo();
}

void MarketUI::switchFocus()
{
    focus_ = (focus_ == FocusList::Buy) ? FocusList::Sell : FocusList::Buy;
    updateHighlight();
    updateInfo();
}

void MarketUI::onKeyPressed(EventKeyboard::KeyCode keyCode, Event* event)
{
    if (keyCode == EventKeyboard::KeyCode::KEY_ESCAPE ||
        keyCode == EventKeyboard::KeyCode::KEY_P)
    {
        close();
        event->stopPropagation();
        return;
    }

    if (keyCode == EventKeyboard::KeyCode::KEY_LEFT_ARROW ||
        keyCode == EventKeyboard::KeyCode::KEY_A ||
        keyCode == EventKeyboard::KeyCode::KEY_RIGHT_ARROW ||
        keyCode == EventKeyboard::KeyCode::KEY_D)
    {
        switchFocus();
        event->stopPropagation();
        return;
    }

    if (keyCode == EventKeyboard::KeyCode::KEY_UP_ARROW ||
        keyCode == EventKeyboard::KeyCode::KEY_W)
    {
        moveSelection(-1);
        event->stopPropagation();
        return;
    }

    if (keyCode == EventKeyboard::KeyCode::KEY_DOWN_ARROW ||
        keyCode == EventKeyboard::KeyCode::KEY_S)
    {
        moveSelection(1);
        event->stopPropagation();
        return;
    }

    if (keyCode == EventKeyboard::KeyCode::KEY_ENTER ||
        keyCode == EventKeyboard::KeyCode::KEY_KP_ENTER ||
        keyCode == EventKeyboard::KeyCode::KEY_SPACE)
    {
        executeTrade();
        event->stopPropagation();
        return;
    }
}

void MarketUI::executeTrade()
{
    const auto& buyGoods = marketState_->getBuyGoods();
    const auto& sellGoods = marketState_->getSellGoods();
    statusLabel_->setString("");

    if (focus_ == FocusList::Buy)
    {
        if (buyGoods.empty())
            return;

        const auto& good = buyGoods[static_cast<size_t>(selectedBuy_)];
        int price = marketState_->getBuyPrice(good.itemType);
        std::string name = InventoryManager::getItemName(good.itemType);

        if (price <= 0)
        {
            statusLabel_->setString("This item is not for sale.");
            return;
        }

        if (!inventory_->removeMoney(price))
        {
            statusLabel_->setString("Not enough gold.");
            return;
        }

        if (!inventory_->addItem(good.itemType, 1))
        {
            inventory_->addMoney(price);
            statusLabel_->setString("Inventory full.");
            return;
        }

        statusLabel_->setString(StringUtils::format("Bought %s (-%dG)", name.c_str(), price));
    }
    else
    {
        if (sellGoods.empty())
            return;

        const auto& good = sellGoods[static_cast<size_t>(selectedSell_)];
        int price = marketState_->getSellPrice(good.itemType);
        std::string name = InventoryManager::getItemName(good.itemType);

        if (price <= 0)
        {
            statusLabel_->setString("This item cannot be sold.");
            return;
        }

        if (!inventory_->hasItem(good.itemType, 1))
        {
            statusLabel_->setString(StringUtils::format("No %s to sell.", name.c_str()));
            return;
        }

        inventory_->removeItem(good.itemType, 1);
        inventory_->addMoney(price);
        statusLabel_->setString(StringUtils::format("Sold %s (+%dG)", name.c_str(), price));
    }

    refresh();
}

void MarketUI::setCloseCallback(const std::function<void()>& callback)
{
    closeCallback_ = callback;
}

void MarketUI::show()
{
    setOpacity(0);
    setScale(0.85f);

    auto fadeIn = FadeIn::create(0.2f);
    auto scaleUp = ScaleTo::create(0.2f, 1.0f);
    runAction(Spawn::create(fadeIn, scaleUp, nullptr));
}

void MarketUI::close()
{
    auto fadeOut = FadeOut::create(0.2f);
    auto scaleDown = ScaleTo::create(0.2f, 0.85f);

    auto callback = CallFunc::create([this]() {
        if (closeCallback_) {
            closeCallback_();
        }
        removeFromParent();
    });

    runAction(Sequence::create(Spawn::create(fadeOut, scaleDown, nullptr), callback, nullptr));
}

void MarketUI::update(float delta)
{
    CC_UNUSED_PARAM(delta);
    int dayCount = farmManager_ ? farmManager_->getDayCount() : 1;
    if (dayCount != lastDay_) {
        lastDay_ = dayCount;
        refresh();
    }
}
