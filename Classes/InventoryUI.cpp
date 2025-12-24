#include "InventoryUI.h"

USING_NS_CC;

const float InventoryUI::SLOT_SIZE = 60.0f;
const float InventoryUI::SLOT_SPACING = 10.0f;

InventoryUI* InventoryUI::create(InventoryManager* inventory)
{
    InventoryUI* ret = new (std::nothrow) InventoryUI();
    if (ret && ret->init(inventory))
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

bool InventoryUI::init(InventoryManager* inventory)
{
    if (!Layer::init())
        return false;

    inventory_ = inventory;
    if (!inventory_)
    {
        CCLOG("ERROR: InventoryUI requires a valid InventoryManager!");
        return false;
    }

    // 初始化UI组件
    initBackground();
    initPanel();
    initSlots();
    initControls();

    // 键盘监听（ESC 和 B 关闭）
    auto keyListener = EventListenerKeyboard::create();
    keyListener->onKeyPressed = CC_CALLBACK_2(InventoryUI::onKeyPressed, this);
    _eventDispatcher->addEventListenerWithSceneGraphPriority(keyListener, this);

    CCLOG("InventoryUI initialized");

    return true;
}

void InventoryUI::initBackground()
{
    auto visibleSize = Director::getInstance()->getVisibleSize();

    // 半透明黑色背景
    background_ = LayerColor::create(Color4B(0, 0, 0, 180), visibleSize.width, visibleSize.height);
    this->addChild(background_, 0);

    // 背景可点击（防止点击穿透）
    auto listener = EventListenerTouchOneByOne::create();
    listener->setSwallowTouches(true);
    listener->onTouchBegan = [](Touch* touch, Event* event) {
        return true;  // 吞噬触摸事件
    };
    _eventDispatcher->addEventListenerWithSceneGraphPriority(listener, background_);
}

void InventoryUI::initPanel()
{
    auto visibleSize = Director::getInstance()->getVisibleSize();

    // 计算面板大小
    float panelWidth = COLS * (SLOT_SIZE + SLOT_SPACING) + SLOT_SPACING * 2 + 40;
    float panelHeight = ROWS * (SLOT_SIZE + SLOT_SPACING) + SLOT_SPACING * 2 + 120;

    // 面板背景（深色）
    panel_ = LayerColor::create(Color4B(40, 35, 30, 255), panelWidth, panelHeight);
    panel_->setPosition(Vec2(
        (visibleSize.width - panelWidth) / 2,
        (visibleSize.height - panelHeight) / 2
    ));
    this->addChild(panel_, 1);

    // 面板边框
    auto border = DrawNode::create();
    border->drawRect(
        Vec2(0, 0),
        Vec2(panelWidth, panelHeight),
        Color4F(0.8f, 0.7f, 0.5f, 1.0f)
    );
    border->setLineWidth(3);
    panel_->addChild(border, 0);

    // 标题
    titleLabel_ = Label::createWithSystemFont("Inventory", "Arial", 32);
    titleLabel_->setPosition(Vec2(panelWidth / 2, panelHeight - 40));
    titleLabel_->setColor(Color3B(255, 235, 200));
    panel_->addChild(titleLabel_, 1);

    // 金币显示
    moneyLabel_ = Label::createWithSystemFont("Gold: 500", "Arial", 24);
    moneyLabel_->setAnchorPoint(Vec2(1, 0.5f));
    moneyLabel_->setPosition(Vec2(panelWidth - 20, panelHeight - 40));
    moneyLabel_->setColor(Color3B(255, 215, 0));
    panel_->addChild(moneyLabel_, 1);

    // 物品信息显示（底部）
    infoLabel_ = Label::createWithSystemFont("", "Arial", 18);
    infoLabel_->setPosition(Vec2(panelWidth / 2, 30));
    infoLabel_->setColor(Color3B(200, 200, 200));
    panel_->addChild(infoLabel_, 1);
}

void InventoryUI::initSlots()
{
    slotSprites_.clear();

    float startX = 20 + SLOT_SPACING;
    float startY = panel_->getContentSize().height - 80 - SLOT_SPACING;

    for (int row = 0; row < ROWS; ++row)
    {
        for (int col = 0; col < COLS; ++col)
        {
            int slotIndex = row * COLS + col;
            if (slotIndex >= InventoryManager::MAX_SLOTS)
                break;

            SlotSprite slot;
            slot.slotIndex = slotIndex;

            // 计算位置
            float x = startX + col * (SLOT_SIZE + SLOT_SPACING);
            float y = startY - row * (SLOT_SIZE + SLOT_SPACING);

            // 创建格子背景
            slot.background = createSlotBackground();
            slot.background->setPosition(Vec2(x, y));
            panel_->addChild(slot.background, 2);

            // 创建物品图标（初始为空）
            slot.icon = nullptr;

            // 创建数量标签
            slot.countLabel = Label::createWithSystemFont("", "Arial", 16);
            slot.countLabel->setAnchorPoint(Vec2(1, 0));
            slot.countLabel->setPosition(Vec2(SLOT_SIZE - 5, 5));
            slot.countLabel->setColor(Color3B::WHITE);
            slot.background->addChild(slot.countLabel, 1);

            // 添加点击事件
            auto listener = EventListenerTouchOneByOne::create();
            listener->setSwallowTouches(true);
            listener->onTouchBegan = [this, slotIndex](Touch* touch, Event* event) {
                auto target = static_cast<Sprite*>(event->getCurrentTarget());
                Vec2 locationInNode = target->convertToNodeSpace(touch->getLocation());
                Size size = target->getContentSize();
                Rect rect = Rect(0, 0, size.width, size.height);

                if (rect.containsPoint(locationInNode))
                {
                    onSlotClicked(slotIndex);
                    return true;
                }
                return false;
            };
            _eventDispatcher->addEventListenerWithSceneGraphPriority(listener, slot.background);

            slotSprites_.push_back(slot);
        }
    }

    // 初始刷新
    refresh();
}

void InventoryUI::initControls()
{
    auto visibleSize = Director::getInstance()->getVisibleSize();

    // 关闭按钮提示
    auto hint = Label::createWithSystemFont("Press ESC or B to close", "Arial", 18);
    hint->setPosition(Vec2(visibleSize.width / 2, 30));
    hint->setColor(Color3B(150, 150, 150));
    this->addChild(hint, 2);
}

void InventoryUI::refresh()
{
    if (!inventory_)
        return;

    // 更新金币显示
    moneyLabel_->setString(StringUtils::format("Gold: %d", inventory_->getMoney()));

    // 更新所有格子
    for (auto& slot : slotSprites_)
    {
        updateSlot(slot.slotIndex);
    }
}

void InventoryUI::updateSlot(int slotIndex)
{
    if (slotIndex < 0 || slotIndex >= (int)slotSprites_.size())
        return;

    auto& slot = slotSprites_[slotIndex];
    const auto& itemSlot = inventory_->getSlot(slotIndex);

    // 移除旧图标
    if (slot.icon)
    {
        slot.icon->removeFromParent();
        slot.icon = nullptr;
    }

    // 如果槽位为空
    if (itemSlot.isEmpty())
    {
        slot.countLabel->setString("");
        return;
    }

    // 创建物品图标
    slot.icon = createItemIcon(itemSlot.type);
    if (slot.icon)
    {
        slot.icon->setPosition(Vec2(SLOT_SIZE / 2, SLOT_SIZE / 2));
        slot.background->addChild(slot.icon, 0);
    }

    // 更新数量
    if (itemSlot.count > 1)
    {
        slot.countLabel->setString(StringUtils::format("%d", itemSlot.count));
    }
    else
    {
        slot.countLabel->setString("");
    }
}

cocos2d::Sprite* InventoryUI::createSlotBackground()
{
    auto sprite = Sprite::create();
    sprite->setAnchorPoint(Vec2(0, 0));
    sprite->setTextureRect(Rect(0, 0, SLOT_SIZE, SLOT_SIZE));
    sprite->setColor(Color3B(60, 55, 50));

    // 边框
    auto border = DrawNode::create();
    border->drawRect(
        Vec2(0, 0),
        Vec2(SLOT_SIZE, SLOT_SIZE),
        Color4F(0.5f, 0.45f, 0.4f, 1.0f)
    );
    border->setLineWidth(2);
    sprite->addChild(border, -1);

    return sprite;
}

cocos2d::Sprite* InventoryUI::createItemIcon(ItemType itemType)
{
    // 创建占位符图标（使用颜色区分不同物品）
    auto icon = Sprite::create();
    icon->setTextureRect(Rect(0, 0, SLOT_SIZE - 10, SLOT_SIZE - 10));

    Color3B color = getItemColor(itemType);
    icon->setColor(color);

    // 添加物品名称缩写
    std::string name = InventoryManager::getItemName(itemType);
    std::string abbreviation;

    // 创建缩写（取首字母）
    if (!name.empty())
    {
        if (name.find(" ") != std::string::npos)
        {
            // 多个单词，取每个单词首字母
            abbreviation += name[0];
            for (size_t i = 1; i < name.length(); ++i)
            {
                if (name[i - 1] == ' ')
                    abbreviation += name[i];
            }
        }
        else
        {
            // 单个单词，取前2-3个字母
            abbreviation = name.substr(0, std::min((size_t)3, name.length()));
        }
    }

    auto label = Label::createWithSystemFont(abbreviation, "Arial", 14);
    label->setPosition(Vec2((SLOT_SIZE - 10) / 2, (SLOT_SIZE - 10) / 2));
    label->setColor(Color3B::WHITE);
    icon->addChild(label, 1);

    return icon;
}

void InventoryUI::onSlotClicked(int slotIndex)
{
    const auto& slot = inventory_->getSlot(slotIndex);

    if (slot.isEmpty())
    {
        infoLabel_->setString("");
    }
    else
    {
        std::string info = StringUtils::format(
            "%s x%d - %s",
            InventoryManager::getItemName(slot.type).c_str(),
            slot.count,
            InventoryManager::getItemDescription(slot.type).c_str()
        );
        infoLabel_->setString(info);

        CCLOG("Clicked slot %d: %s x%d", slotIndex,
            InventoryManager::getItemName(slot.type).c_str(), slot.count);
    }

    // 可以在这里添加更多交互，比如拖拽、使用物品等
}

void InventoryUI::onKeyPressed(EventKeyboard::KeyCode keyCode, Event* event)
{
    if (keyCode == EventKeyboard::KeyCode::KEY_ESCAPE ||
        keyCode == EventKeyboard::KeyCode::KEY_B)
    {
        close();
    }
}

void InventoryUI::setCloseCallback(const std::function<void()>& callback)
{
    closeCallback_ = callback;
}

void InventoryUI::show()
{
    // 显示动画：从透明渐入
    this->setOpacity(0);
    this->setScale(0.8f);

    auto fadeIn = FadeIn::create(0.2f);
    auto scaleUp = ScaleTo::create(0.2f, 1.0f);
    auto spawn = Spawn::create(fadeIn, scaleUp, nullptr);

    this->runAction(spawn);
}

void InventoryUI::close()
{
    // 关闭动画：渐出并缩小
    auto fadeOut = FadeOut::create(0.2f);
    auto scaleDown = ScaleTo::create(0.2f, 0.8f);
    auto spawn = Spawn::create(fadeOut, scaleDown, nullptr);

    auto callback = CallFunc::create([this]() {
        if (closeCallback_)
        {
            closeCallback_();
        }
        this->removeFromParent();
    });

    auto sequence = Sequence::create(spawn, callback, nullptr);
    this->runAction(sequence);

    CCLOG("InventoryUI closing");
}

cocos2d::Color3B InventoryUI::getItemColor(ItemType itemType) const
{
    switch (itemType)
    {
    // 工具 - 棕色系
    case ItemType::Hoe:
    case ItemType::WateringCan:
    case ItemType::Scythe:
    case ItemType::Axe:
    case ItemType::Pickaxe:
        return Color3B(139, 90, 43);

    // 钓鱼竿 - 蓝色
    case ItemType::FishingRod:
        return Color3B(70, 130, 180);

    // 种子 - 绿色系
    case ItemType::SeedTurnip:
    case ItemType::SeedPotato:
    case ItemType::SeedCorn:
    case ItemType::SeedTomato:
    case ItemType::SeedPumpkin:
    case ItemType::SeedBlueberry:
        return Color3B(34, 139, 34);

    // 农作物 - 橙色/红色系
    case ItemType::Turnip:
    case ItemType::Potato:
        return Color3B(210, 180, 140);
    case ItemType::Corn:
        return Color3B(255, 215, 0);
    case ItemType::Tomato:
        return Color3B(220, 20, 60);
    case ItemType::Pumpkin:
        return Color3B(255, 117, 24);
    case ItemType::Blueberry:
        return Color3B(65, 105, 225);

    // 木材 - 深棕色
    case ItemType::Wood:
        return Color3B(101, 67, 33);

    // 鱼 - 银色
    case ItemType::Fish:
        return Color3B(192, 192, 192);

    default:
        return Color3B(128, 128, 128);
    }
}
