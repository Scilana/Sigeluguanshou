#include "BlacksmithUI.h"
#include "InventoryManager.h"

USING_NS_CC;

static void review_alert_label(Node* parent, std::string text, Vec2 pos) {
    auto label = Label::createWithSystemFont(text, "Arial", 12);
    label->setPosition(pos);
    label->setColor(Color3B::RED);
    parent->addChild(label);
}

BlacksmithUI* BlacksmithUI::create()
{
    BlacksmithUI* ret = new (std::nothrow) BlacksmithUI();
    if (ret && ret->init())
    {
        ret->autorelease();
        return ret;
    }
    CC_SAFE_DELETE(ret);
    return nullptr;
}

bool BlacksmithUI::init()
{
    if (!Layer::init())
        return false;

    auto visibleSize = Director::getInstance()->getVisibleSize();
    auto origin = Director::getInstance()->getVisibleOrigin();

    // 1. 半透明背景
    background_ = LayerColor::create(Color4B(0, 0, 0, 180));
    this->addChild(background_, 0);

    // 屏蔽下层点击
    auto listener = EventListenerTouchOneByOne::create();
    listener->setSwallowTouches(true);
    listener->onTouchBegan = [](Touch* t, Event* e) { return true; };
    _eventDispatcher->addEventListenerWithSceneGraphPriority(listener, background_);

    // 2. 主面板
    float panelW = 500;
    float panelH = 400;

    panel_ = Sprite::create();
    panel_->setTextureRect(Rect(0, 0, panelW, panelH));
    panel_->setColor(Color3B(60, 50, 40)); // 深棕色
    panel_->setPosition(Vec2(
        origin.x + visibleSize.width / 2,
        origin.y + visibleSize.height / 2
    ));
    this->addChild(panel_, 1);

    // 边框
    auto border = DrawNode::create();
    border->drawRect(Vec2(0, 0), Vec2(panelW, panelH), Color4F(0.4f, 0.3f, 0.2f, 1.0f));
    border->setLineWidth(4);
    panel_->addChild(border, 1);

    // 标题
    auto title = Label::createWithSystemFont("Blacksmith Shop", "Arial", 24);
    title->setPosition(Vec2(panelW / 2, panelH - 30));
    title->setColor(Color3B(255, 200, 100));
    panel_->addChild(title, 2);

    // 关闭按钮
    auto closeLabel = Label::createWithSystemFont("[ X ]", "Arial", 20);
    auto closeItem = MenuItemLabel::create(closeLabel, [this](Ref* sender) {
        close();
        });
    closeItem->setPosition(Vec2(panelW - 30, panelH - 30));
    
    auto menu = Menu::create(closeItem, nullptr);
    menu->setPosition(Vec2::ZERO);
    panel_->addChild(menu, 2);

    // --- TABS ---
    auto btnRepair = MenuItemLabel::create(Label::createWithSystemFont("Repair", "Arial", 20), [this](Ref*){ switchMode(Mode::Repair); });
    btnRepair->setPosition(Vec2(80, panelH - 60)); // Moved left
    auto btnShop = MenuItemLabel::create(Label::createWithSystemFont("Shop", "Arial", 20), [this](Ref*){ switchMode(Mode::Shop); });
    btnShop->setPosition(Vec2(160, panelH - 60));
    
    auto tabMenu = Menu::create(btnRepair, btnShop, nullptr);
    tabMenu->setPosition(Vec2::ZERO);
    panel_->addChild(tabMenu, 2);

    // 列表容器
    listNode_ = Node::create();
    listNode_->setPosition(Vec2(20, panelH - 100));
    panel_->addChild(listNode_, 2);

    switchMode(Mode::Repair);

    return true;
}

void BlacksmithUI::show()
{
    this->setVisible(true);
    refreshList();
}

void BlacksmithUI::close()
{
    this->setVisible(false);
    this->removeFromParent();
}

void BlacksmithUI::refreshList()
{
    listNode_->removeAllChildren();

    auto inventory = InventoryManager::getInstance();
    if (!inventory) return;

    int yOffset = 0;
    const int ITEM_HEIGHT = 60;

    if (currentMode_ == Mode::Shop)
    {
        struct ShopItem { ItemType type; std::string name; int price; };
        std::vector<ShopItem> shopItems = {
            {ItemType::Hoe, "Hoe", 500},
            {ItemType::WateringCan, "Water Can", 500},
            {ItemType::Scythe, "Scythe", 500},
            {ItemType::Axe, "Axe", 1000},
            {ItemType::Pickaxe, "Pickaxe", 1000},
            {ItemType::SeedTurnip, "Turnip Seed", 20},
            {ItemType::SeedCorn, "Corn Seed", 50}
        };

        for (const auto& item : shopItems) {
            auto itemNode = Node::create();
            itemNode->setPosition(Vec2(0, -yOffset));
            listNode_->addChild(itemNode);

             // Icon
            std::string iconPath = InventoryManager::getItemIconPath(item.type);
             Sprite* icon = nullptr;
            if (!iconPath.empty() && FileUtils::getInstance()->isFileExist(iconPath)) {
                icon = Sprite::create(iconPath);
            } else {
                 icon = Sprite::create();
                icon->setTextureRect(Rect(0,0, 40, 40));
                icon->setColor(Color3B::WHITE);
            }
            if(icon){
                icon->setScale(40.0f / icon->getContentSize().width);
                icon->setPosition(Vec2(30, -30));
                itemNode->addChild(icon);
            }
            
            // Name
            auto nameLabel = Label::createWithSystemFont(item.name, "Arial", 18);
            nameLabel->setAnchorPoint(Vec2(0, 0.5f));
            nameLabel->setPosition(Vec2(70, -20));
            itemNode->addChild(nameLabel);
            
            // Price
            auto priceLabel = Label::createWithSystemFont(StringUtils::format("%d G", item.price), "Arial", 14);
            priceLabel->setAnchorPoint(Vec2(0, 0.5f));
            priceLabel->setPosition(Vec2(70, -40));
            priceLabel->setColor(Color3B::YELLOW);
            itemNode->addChild(priceLabel);
            
            // Buy Button
            auto buyLabel = Label::createWithSystemFont("Buy", "Arial", 16);
            auto buyBtn = MenuItemLabel::create(buyLabel, [this, item](Ref*){
                 this->onBuyClicked(item.type, item.price);
            });
            buyBtn->setPosition(Vec2(350, -30));
             
            auto menu = Menu::create(buyBtn, nullptr);
            menu->setPosition(Vec2::ZERO);
            itemNode->addChild(menu);

            yOffset += ITEM_HEIGHT;
        }
        return;
    }

    const int COST_PER_POINT = 2; // 修复每点花费 2 金币

    bool hasItems = false;

    // 遍历背包寻找受损工具
    for (int i = 0; i < inventory->getSlotCount(); ++i)
    {
        const auto& slot = inventory->getSlot(i);
        if (slot.isTool() && slot.maxDurability > 0 && slot.durability < slot.maxDurability)
        {
            hasItems = true;
            
            // 计算花费
            int lost = slot.maxDurability - slot.durability;
            int cost = lost * COST_PER_POINT;
            
            // --- 绘制条目 ---
            auto itemNode = Node::create();
            itemNode->setPosition(Vec2(0, -yOffset));
            listNode_->addChild(itemNode);

            // 1. 图标
            std::string iconPath = InventoryManager::getItemIconPath(slot.type);
            Sprite* icon = nullptr;
            if (!iconPath.empty() && FileUtils::getInstance()->isFileExist(iconPath)) {
                icon = Sprite::create(iconPath);
            } else {
                icon = Sprite::create();
                icon->setTextureRect(Rect(0,0, 40, 40));
                icon->setColor(Color3B::GRAY);
            }
            if (icon) {
                icon->setScale(40.0f / icon->getContentSize().width);
                icon->setPosition(Vec2(30, -30));
                itemNode->addChild(icon);
            }

            // 2. 名称和耐久
            std::string name = InventoryManager::getItemName(slot.type);
            auto nameLabel = Label::createWithSystemFont(name, "Arial", 18);
            nameLabel->setAnchorPoint(Vec2(0, 0.5f));
            nameLabel->setPosition(Vec2(70, -20));
            itemNode->addChild(nameLabel);

            auto durLabel = Label::createWithSystemFont(
                StringUtils::format("Durability: %d/%d", slot.durability, slot.maxDurability),
                "Arial", 14);
            durLabel->setAnchorPoint(Vec2(0, 0.5f));
            durLabel->setPosition(Vec2(70, -40));
            durLabel->setColor(Color3B(200, 200, 200));
            itemNode->addChild(durLabel);

            // 3. 修复按钮
            auto btnLabel = Label::createWithSystemFont(
                StringUtils::format("Repair (%dg)", cost), "Arial", 10); // Font size increased by implementation but kept logical
            // Note: Menu Label
            
            auto repairBtn = MenuItemLabel::create(btnLabel, [this, i, cost, &slot](Ref* sender) { // Capture slot by ref is dangerous if vector resizes! Capture by index 'i'.
                // Re-fetch inventory to be safe
                auto inv = InventoryManager::getInstance();
                if (inv && inv->getMoney() >= cost) {
                    if (inv->removeMoney(cost)) {
                        // Restore durability
                        // We need a method to set durability, or just hack it via setSlot/access.
                        // Currently InventoryManager doesn't expose setDurability.
                        // I need to add that or assume I can modify slot reference via getSlot (it returns const&).
                        // I added 'decreaseDurability'. I should add 'repairItem'.
                        // For now, I will add 'repairItem' to InventoryManager or use a workaround.
                        // But I cannot modify const ref.
                        // I will add 'repairSlot(int index)' to InventoryManager.
                        // Wait, I can't edit InventoryManager now easily without context switch.
                        // Actually I can edit InventoryManager.cpp.
                        // Let's implement onRepairClicked logic via a helper in UI for now, calling a new method in Manager.
                        this->onRepairClicked(i);
                    }
                } else {
                    // Show "Not enough money"
                }
            });
            
            repairBtn->setPosition(Vec2(350, -30));
            
            // Check money
            if (inventory->getMoney() < cost) {
                repairBtn->setEnabled(false);
                review_alert_label(itemNode, "Not enough gold", Vec2(350, -10));
            }

            auto menu = Menu::create(repairBtn, nullptr);
            menu->setPosition(Vec2::ZERO);
            itemNode->addChild(menu);

            yOffset += ITEM_HEIGHT;
        }
    }

    if (!hasItems)
    {
        auto label = Label::createWithSystemFont("All tools are in good condition!", "Arial", 18);
        label->setPosition(Vec2(250, -100));
        label->setColor(Color3B::YELLOW);
        listNode_->addChild(label);
    }
}

void BlacksmithUI::onBuyClicked(ItemType type, int price)
{
    auto inventory = InventoryManager::getInstance();
    if (!inventory) return;

    if (inventory->getMoney() >= price) {
        if (inventory->addItem(type, 1)) { // Check if added
            inventory->removeMoney(price);
            review_alert_label(panel_, "Purchased!", Vec2(250, -320)); // Temp feedback
            refreshList(); // Refresh money display if we had one (we don't currently show money in UI except implicitly)
            // Ideally update title with money
        } else {
             review_alert_label(panel_, "Inventory Full", Vec2(250, -320));
        }
    } else {
         review_alert_label(panel_, "Not enough Gold", Vec2(250, -320));
    }
}

void BlacksmithUI::switchMode(Mode mode)
{
    currentMode_ = mode;
    refreshList();
}

void BlacksmithUI::onRepairClicked(int slotIndex)
{
    auto inventory = InventoryManager::getInstance();
    if (inventory && inventory->repairSlot(slotIndex))
    {
        refreshList();
    }
}


