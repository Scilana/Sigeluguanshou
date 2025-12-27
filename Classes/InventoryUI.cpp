#include "InventoryUI.h"
#include "InventoryManager.h"
#include "QuantityPopup.h"
#include "MarketState.h"

USING_NS_CC;

const float InventoryUI::SLOT_SIZE = 60.0f;
const float InventoryUI::SLOT_SPACING = 10.0f;

InventoryUI* InventoryUI::create(InventoryManager* inventory, MarketState* marketState)
{
    InventoryUI* ret = new (std::nothrow) InventoryUI();
    if (ret && ret->init(inventory, marketState))
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

bool InventoryUI::init(InventoryManager* inventory, MarketState* marketState)
{
    if (!Layer::init())
        return false;

    auto visibleSize = Director::getInstance()->getVisibleSize();
    this->setContentSize(visibleSize);
    
    inventory_ = inventory;
    marketState_ = marketState;
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

    selectedSlotIndex_ = -1; // -1 表示未选中

    // 键盘监听（ESC 和 B 关闭）
    auto keyListener = EventListenerKeyboard::create();
    keyListener->onKeyPressed = CC_CALLBACK_2(InventoryUI::onKeyPressed, this);
    _eventDispatcher->addEventListenerWithSceneGraphPriority(keyListener, this);

    CCLOG("InventoryUI initialized");

    _selectionMode = false;
    return true;
}

void InventoryUI::setSelectionMode(bool enabled)
{
    _selectionMode = enabled;
}

void InventoryUI::setOnItemSelectedCallback(const ItemSelectCallback& callback)
{
    _onItemSelected = callback;
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
            if (slotIndex >= inventory_->getSlotCount())
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
                
                // Debugging Click Issue: Revert to standard and Log EVERYTHING
                Vec2 locationInNode = target->convertTouchToNodeSpace(touch);
                Size size = target->getContentSize();
                Rect rect = Rect(0, 0, size.width, size.height);
                
                bool hit = rect.containsPoint(locationInNode);
                CCLOG("Slot %d touched. Hit: %s, location: (%.1f, %.1f)", slotIndex, hit ? "YES" : "NO", locationInNode.x, locationInNode.y);
                
                if (hit)
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

    updateSelection();
}

void InventoryUI::updateSelection()
{
    // 更新选中框显示
    for (auto& slot : slotSprites_)
    {
        if (slot.slotIndex == selectedSlotIndex_)
        {
             // 选中颜色改为更明显的高亮色 (黄褐色)
             slot.background->setColor(Color3B(180, 160, 100)); 
        }
        else
        {
             // 默认颜色 (深灰褐色)
             slot.background->setColor(Color3B(60, 55, 50)); 
        }
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
    slot.icon = createItemIcon(itemSlot);
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

cocos2d::Sprite* InventoryUI::createItemIcon(const InventoryManager::ItemSlot& slot)
{
    ItemType itemType = slot.type;
    std::string iconPath = InventoryManager::getItemIconPath(itemType);
    Sprite* icon = nullptr;
    float iconSize = SLOT_SIZE - 10.0f;

    if (!iconPath.empty() && FileUtils::getInstance()->isFileExist(iconPath))
    {
        icon = Sprite::create(iconPath);
        if (icon)
        {
            auto size = icon->getContentSize();
            float scale = std::min(iconSize / size.width, iconSize / size.height);
            icon->setScale(scale);
        }
    }

    if (!icon)
    {
        // Fallback colored placeholder.
        icon = Sprite::create();
        icon->setTextureRect(Rect(0, 0, iconSize, iconSize));
        Color3B color = getItemColor(itemType);
        icon->setColor(color);

        std::string name = InventoryManager::getItemName(itemType);
        std::string abbreviation;
        if (!name.empty())
        {
            if (name.find(" ") != std::string::npos)
            {
                abbreviation += name[0];
                for (size_t i = 1; i < name.length(); ++i) {
                    if (name[i - 1] == ' ') abbreviation += name[i];
                }
            }
            else {
                abbreviation = name.substr(0, std::min((size_t)3, name.length()));
            }
        }

        auto label = Label::createWithSystemFont(abbreviation, "Arial", 14);
        label->setPosition(Vec2(iconSize / 2, iconSize / 2));
        label->setColor(Color3B::WHITE);
        icon->addChild(label, 1);
    }

    // === Durability Bar ===
    if (slot.isTool() && slot.maxDurability > 0)
    {
        float percent = (float)slot.durability / slot.maxDurability;
        float barWidth = iconSize; // Full width of icon
        float barHeight = 4.0f;
        
        // Background (Black)
        auto barBg = LayerColor::create(Color4B::BLACK, barWidth, barHeight);
        barBg->setPosition(Vec2(0, 0)); // Bottom of icon
        // LayerColor anchor is usually (0,0), but strictly it's a layer. 
        // Best to use DrawNode for small bars or just LayerColor.
        // Since icon is a Sprite, (0,0) is bottom-left relative to its texture RECT if anchor is (0.5, 0.5) UNLESS it's a child.
        // Sprites have anchor (0.5, 0.5) by default. Children are placed relative to (0,0) of the parent's content size space.
        // Sprite content space (0,0) is bottom-left.
        float contentW = icon->getContentSize().width;
        float contentH = icon->getContentSize().height;
        
        // We need to scale the bar to match the visual size, but the icon might be scaled.
        // Providing a consistent overlay is easier if we attach it to a Node that isn't scaled, OR we handle scale.
        // Actually, let's attach to the icon and inverse scale? No, that's complex.
        // Let's attach to the icon but position at bottom.
        
        // Using DrawNode is safer for dynamic sizing
        auto draw = DrawNode::create();
        
        // Local coordinates on the icon
        Vec2 start(0, 0);
        Vec2 end(contentW, contentH * 0.1f); // 10% height bar? Or fixed pixels?
        // Let's use fixed logic assuming the icon roughly fills the slot visually
        
        float w = contentW;
        float h = 5.0f; 
        
        // Bg
        draw->drawSolidRect(Vec2(0, 0), Vec2(w, h), Color4F(0, 0, 0, 1));
        
        // Fg color
        Color4F color = Color4F::GREEN;
        if (percent < 0.2f) color = Color4F::RED;
        else if (percent < 0.5f) color = Color4F(1.0f, 0.8f, 0.0f, 1.0f); // Yellow/Orange
        
        // Fg
        draw->drawSolidRect(Vec2(0, 0), Vec2(w * percent, h), color);
        
        icon->addChild(draw, 10);
    }

    return icon;
}

void InventoryUI::onSlotClicked(int slotIndex)
{
    CCLOG("onSlotClicked: %d, current selection: %d", slotIndex, selectedSlotIndex_);
    
    // Selection Mode Logic (For Merchant Sell)
    if (_selectionMode) {
        if (_onItemSelected) {
            const auto& slot = inventory_->getSlot(slotIndex);
            if (!slot.isEmpty()) { 
                _onItemSelected(slotIndex, slot.type, slot.count);
                // Note: We don't change internal selection or swap in this mode
            }
        }
        return;
    }

    // 如果已经选中了一个槽位，且点击的是另一个槽位，则进行交换
    if (selectedSlotIndex_ != -1 && selectedSlotIndex_ != slotIndex)
    {
        CCLOG("Swapping slot %d with %d", selectedSlotIndex_, slotIndex);
        inventory_->swapSlots(selectedSlotIndex_, slotIndex);
        
        // 更新信息显示
        const auto& newSlot = inventory_->getSlot(slotIndex);
        if (!newSlot.isEmpty())
        {
            std::string info = StringUtils::format(
                "%s x%d - %s",
                InventoryManager::getItemName(newSlot.type).c_str(),
                newSlot.count,
                InventoryManager::getItemDescription(newSlot.type).c_str()
            );
            infoLabel_->setString(info);
        }
        
        // 交换后维持选中在目标位置，方便连续移动
        selectedSlotIndex_ = slotIndex;
        
        // 延迟刷新到下一帧，避免在触摸事件中修改场景图
        this->scheduleOnce([this](float) {
            refresh();
        }, 0, "deferred_refresh");
    }
    else
    {
        // 第一次点击，或者是点击同一个（取消选中）
        if (selectedSlotIndex_ == slotIndex)
        {
            CCLOG("Deselecting slot %d", slotIndex);
            selectedSlotIndex_ = -1;
            infoLabel_->setString("");
        }
        else
        {
            CCLOG("Selecting slot %d", slotIndex);
            selectedSlotIndex_ = slotIndex;
            const auto& slot = inventory_->getSlot(slotIndex);
            if (!slot.isEmpty())
            {
                std::string info = StringUtils::format(
                    "%s x%d - %s",
                    InventoryManager::getItemName(slot.type).c_str(),
                    slot.count,
                    InventoryManager::getItemDescription(slot.type).c_str()
                );
                infoLabel_->setString(info);
            }
            else
            {
                infoLabel_->setString("Empty slot");
            }
        }
    }

    // 延迟更新选中框到下一帧
    this->scheduleOnce([this](float) {
        updateSelection();
    }, 0, "deferred_update_selection");
}

void InventoryUI::onKeyPressed(EventKeyboard::KeyCode keyCode, Event* event)
{
    if (keyCode == EventKeyboard::KeyCode::KEY_ESCAPE ||
        keyCode == EventKeyboard::KeyCode::KEY_B)
    {
        close();
        return;
    }

    // 数字键重新绑定快捷键
    if (keyCode >= EventKeyboard::KeyCode::KEY_0 && keyCode <= EventKeyboard::KeyCode::KEY_9)
    {
        int num = (int)keyCode - (int)EventKeyboard::KeyCode::KEY_0;
        int targetSlotIndex = (num == 0) ? 9 : num - 1; // 1->0, 2->1 ... 0->9

        // 如果当前有选中的物品，交换
        if (selectedSlotIndex_ != -1)
        {
            if (selectedSlotIndex_ != targetSlotIndex)
            {
                inventory_->swapSlots(selectedSlotIndex_, targetSlotIndex);
                
                // 刷新界面
                refresh();
                
                // 显示提示
                if (infoLabel_)
                {
                    std::string itemName = InventoryManager::getItemName(inventory_->getSlot(targetSlotIndex).type);
                    infoLabel_->setString(StringUtils::format("Moved %s to slot %d", itemName.c_str(), num));
                }
                
                // 交换后，更新选中框位置或者取消选中
                // 这里我们选择更新选中到新的位置，方便继续操作
                selectedSlotIndex_ = targetSlotIndex;
                updateSelection();
            }
        }
    }

    // J 键转移物品
    if (keyCode == EventKeyboard::KeyCode::KEY_J)
    {
        handleTransfer();
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
    case ItemType::ITEM_Anchovy:
    case ItemType::ITEM_Carp:
    case ItemType::ITEM_Eel:
    case ItemType::ITEM_Flounder:
    case ItemType::ITEM_Largemouth_Bass:
    case ItemType::ITEM_Pufferfish:
    case ItemType::ITEM_Rainbow_Trout:
    case ItemType::ITEM_Sturgeon:
    case ItemType::ITEM_Tilapia:
        return Color3B(192, 192, 192);

    // 矿石
    case ItemType::CopperOre: return Color3B(210, 105, 30);  // Chocolate orange
    case ItemType::IronOre: return Color3B(112, 128, 144);    // Slate grey
    case ItemType::SilverOre: return Color3B(224, 224, 224);  // Bright silver
    case ItemType::GoldOre: return Color3B(255, 215, 0);     // Gold
    case ItemType::DiamondOre: return Color3B(0, 255, 255);  // Cyan/Aqua

    default:
        return Color3B(128, 128, 128);
    }
}

void InventoryUI::handleTransfer()
{
    if (!partnerInventory_ || selectedSlotIndex_ == -1) return;

    const auto& slot = inventory_->getSlot(selectedSlotIndex_);
    if (slot.isEmpty()) return;

    // --- Shipping Bin Logic ---
    if (partnerIsShippingBin_)
    {
        // 1. Check if sellable
        int price = 0;
        if (marketState_) {
            price = marketState_->getSellPrice(slot.type);
        }

        if (price <= 0) {
            infoLabel_->setString("Cannot sell this item!");
            // Shake effect or red flash could be added here
            return;
        }

        // 2. Transfer logic (similar to normal, but with feedback)
        
        // Helper to perform transfer and show message
        auto doTransfer = [this, price](int count) {
            if (this->partnerInventory_->addItem(this->inventory_->getSlot(this->selectedSlotIndex_).type, count)) {
                
                std::string itemName = InventoryManager::getItemName(this->inventory_->getSlot(this->selectedSlotIndex_).type);
                
                this->inventory_->removeItemFromSlot(this->selectedSlotIndex_, count);
                this->refresh();
                
                int totalValue = price * count;
                this->infoLabel_->setString(StringUtils::format("Placed %s in bin (+%dG)", itemName.c_str(), totalValue));
            } else {
                this->infoLabel_->setString("Shipping bin full!");
            }
        };

        if (slot.count > 1) {
            int currentIndex = selectedSlotIndex_;
            // 注意：这里需要重新获取 slot 信息或者传递捕获，因为 QuantityPopup 是异步的
            // 为了简化，我们直接用 currentIndex 重新获取
            // 修正：QuantityPopup 的回调里不能依赖 slot 引用，必须拷贝或者重新获取
            auto popup = QuantityPopup::create(slot.count, [this, currentIndex, price, doTransfer](int count) {
                 // 重新检查 slot
                 // 由于 doTransfer 捕获了 this 和 selectedSlotIndex_ (隐式?) 
                 // 我们需要确保 selectedSlotIndex_ 没变，或者我们在 doTransfer 里用 currentIndex
                 // 简单的做法：直接把逻辑写在这里
                 
                 // 上面的 doTransfer 用了 `this->selectedSlotIndex_`，这可能是不安全的如果用户改选了
                 // 真正安全的做法是重新检查 inventory_->getSlot(currentIndex)
                 
                 const auto& verifySlot = this->inventory_->getSlot(currentIndex);
                 if (verifySlot.isEmpty()) return; 
                 
                 // 复用逻辑
                 if (this->partnerInventory_->addItem(verifySlot.type, count)) {
                     std::string itemName = InventoryManager::getItemName(verifySlot.type);
                     this->inventory_->removeItemFromSlot(currentIndex, count);
                     this->refresh();
                     this->infoLabel_->setString(StringUtils::format("Placed %s in bin (+%dG)", itemName.c_str(), price * count));
                 } else {
                     this->infoLabel_->setString("Shipping bin full!");
                 }
            });
            Director::getInstance()->getRunningScene()->addChild(popup, 3000);
            return;
        } else {
            doTransfer(1);
            return;
        }
    }
    // --------------------------

    // Normal Transfer (Storage Chest)
    if (slot.count > 1) {
        int currentIndex = selectedSlotIndex_; 
        auto popup = QuantityPopup::create(slot.count, [this, currentIndex](int count) { // Remove 'slot' capture to avoid stale ref
            const auto& currentSlot = this->inventory_->getSlot(currentIndex); // Re-fetch
            if (currentSlot.isEmpty()) return;

            if (this->partnerInventory_->addItem(currentSlot.type, count)) {
                this->inventory_->removeItemFromSlot(currentIndex, count);
                this->refresh();
            } else {
                this->infoLabel_->setString("Target inventory full!");
            }
        });
        Director::getInstance()->getRunningScene()->addChild(popup, 3000);
    } else {
        if (partnerInventory_->addItem(slot.type, 1)) {
            inventory_->removeItemFromSlot(selectedSlotIndex_, 1);
            refresh();
        } else {
            infoLabel_->setString("Target inventory full!");
        }
    }
}
