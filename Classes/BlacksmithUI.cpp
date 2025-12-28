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

    // 1. 
    background_ = LayerColor::create(Color4B(0, 0, 0, 180));
    this->addChild(background_, 0);

    // 
    auto listener = EventListenerTouchOneByOne::create();
    listener->setSwallowTouches(true);
    listener->onTouchBegan = [](Touch* t, Event* e) { return true; };
    _eventDispatcher->addEventListenerWithSceneGraphPriority(listener, background_);

    // 2. 
    float panelW = 500;
    float panelH = 400;

    panel_ = Sprite::create();
    panel_->setTextureRect(Rect(0, 0, panelW, panelH));
    panel_->setColor(Color3B(60, 50, 40)); // 
    panel_->setPosition(Vec2(
        origin.x + visibleSize.width / 2,
        origin.y + visibleSize.height / 2
    ));
    this->addChild(panel_, 1);

    // 
    auto border = DrawNode::create();
    border->drawRect(Vec2(0, 0), Vec2(panelW, panelH), Color4F(0.4f, 0.3f, 0.2f, 1.0f));
    border->setLineWidth(4);
    panel_->addChild(border, 1);

    // 
    auto title = Label::createWithSystemFont("Blacksmith Shop", "Arial", 24);
    title->setPosition(Vec2(panelW / 2, panelH - 30));
    title->setColor(Color3B(255, 200, 100));
    panel_->addChild(title, 2);

    // 
    auto closeLabel = Label::createWithSystemFont("[ X ]", "Arial", 20);
    auto closeItem = MenuItemLabel::create(closeLabel, [this](Ref* sender) {
        close();
        });
    closeItem->setPosition(Vec2(panelW - 30, panelH - 30));
    
    auto menu = Menu::create(closeItem, nullptr);
    menu->setPosition(Vec2::ZERO);
    panel_->addChild(menu, 2);

    goldLabel_ = Label::createWithSystemFont("Gold: 0", "Arial", 18);
    goldLabel_->setAnchorPoint(Vec2(1, 0.5f));
    goldLabel_->setPosition(Vec2(panelW - 20, panelH - 60));
    goldLabel_->setColor(Color3B(255, 215, 0));
    panel_->addChild(goldLabel_, 2);

    // --- TABS ---
    auto btnRepair = MenuItemLabel::create(Label::createWithSystemFont("Repair", "Arial", 20), [this](Ref*){ switchMode(Mode::Repair); });
    btnRepair->setPosition(Vec2(80, panelH - 60)); // Moved left
    auto btnShop = MenuItemLabel::create(Label::createWithSystemFont("Shop", "Arial", 20), [this](Ref*){ switchMode(Mode::Shop); });
    btnShop->setPosition(Vec2(160, panelH - 60));
    
    auto tabMenu = Menu::create(btnRepair, btnShop, nullptr);
    tabMenu->setPosition(Vec2::ZERO);
    panel_->addChild(tabMenu, 2);

    // 
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

    if (goldLabel_) {
        goldLabel_->setString(StringUtils::format("Gold: %d", inventory->getMoney()));
    }

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

    const int COST_PER_POINT = 2;
    struct RepairEntry {
        int slotIndex;
        ItemType type;
        int durability;
        int maxDurability;
        int cost;
    };

    std::vector<RepairEntry> repairItems;
    int totalCost = 0;

    for (int i = 0; i < inventory->getSlotCount(); ++i)
    {
        const auto& slot = inventory->getSlot(i);
        if (slot.isTool() && slot.maxDurability > 0 && slot.durability < slot.maxDurability)
        {
            int lost = slot.maxDurability - slot.durability;
            int cost = lost * COST_PER_POINT;
            repairItems.push_back({ i, slot.type, slot.durability, slot.maxDurability, cost });
            totalCost += cost;
        }
    }

    if (!repairItems.empty())
    {
        auto summaryNode = Node::create();
        summaryNode->setPosition(Vec2(0, -yOffset));
        listNode_->addChild(summaryNode);

        auto summaryLabel = Label::createWithSystemFont("Repair All Damaged Tools", "Arial", 16);
        summaryLabel->setAnchorPoint(Vec2(0, 0.5f));
        summaryLabel->setPosition(Vec2(70, -30));
        summaryNode->addChild(summaryLabel);

        auto repairAllLabel = Label::createWithSystemFont(
            StringUtils::format("Repair All (%dg)", totalCost), "Arial", 12);
        auto repairAllBtn = MenuItemLabel::create(repairAllLabel, [this](Ref*) {
            this->onRepairAllClicked();
        });
        repairAllBtn->setPosition(Vec2(350, -30));
        if (inventory->getMoney() < totalCost) {
            repairAllBtn->setEnabled(false);
            review_alert_label(summaryNode, "Not enough gold", Vec2(350, -10));
        }

        auto repairAllMenu = Menu::create(repairAllBtn, nullptr);
        repairAllMenu->setPosition(Vec2::ZERO);
        summaryNode->addChild(repairAllMenu);

        yOffset += ITEM_HEIGHT;
    }

    for (const auto& entry : repairItems)
    {
        auto itemNode = Node::create();
        itemNode->setPosition(Vec2(0, -yOffset));
        listNode_->addChild(itemNode);

        std::string iconPath = InventoryManager::getItemIconPath(entry.type);
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

        std::string name = InventoryManager::getItemName(entry.type);
        auto nameLabel = Label::createWithSystemFont(name, "Arial", 18);
        nameLabel->setAnchorPoint(Vec2(0, 0.5f));
        nameLabel->setPosition(Vec2(70, -20));
        itemNode->addChild(nameLabel);

        auto durLabel = Label::createWithSystemFont(
            StringUtils::format("Durability: %d/%d", entry.durability, entry.maxDurability),
            "Arial", 14);
        durLabel->setAnchorPoint(Vec2(0, 0.5f));
        durLabel->setPosition(Vec2(70, -40));
        durLabel->setColor(Color3B(200, 200, 200));
        itemNode->addChild(durLabel);

        auto btnLabel = Label::createWithSystemFont(
            StringUtils::format("Repair (%dg)", entry.cost), "Arial", 10);

        int slotIndex = entry.slotIndex;
        int cost = entry.cost;
        auto repairBtn = MenuItemLabel::create(btnLabel, [this, slotIndex, cost](Ref* sender) {
            auto inv = InventoryManager::getInstance();
            if (inv && inv->getMoney() >= cost) {
                if (inv->removeMoney(cost)) {
                    this->onRepairClicked(slotIndex);
                }
            }
        });

        repairBtn->setPosition(Vec2(350, -30));
        if (inventory->getMoney() < cost) {
            repairBtn->setEnabled(false);
            review_alert_label(itemNode, "Not enough gold", Vec2(350, -10));
        }

        auto menu = Menu::create(repairBtn, nullptr);
        menu->setPosition(Vec2::ZERO);
        itemNode->addChild(menu);

        yOffset += ITEM_HEIGHT;
    }

    if (repairItems.empty())
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

void BlacksmithUI::onRepairAllClicked()
{
    auto inventory = InventoryManager::getInstance();
    if (!inventory) return;

    const int costPerPoint = 2;
    int totalCost = 0;
    std::vector<int> slotsToRepair;

    for (int i = 0; i < inventory->getSlotCount(); ++i)
    {
        const auto& slot = inventory->getSlot(i);
        if (slot.isTool() && slot.maxDurability > 0 && slot.durability < slot.maxDurability)
        {
            int lost = slot.maxDurability - slot.durability;
            totalCost += lost * costPerPoint;
            slotsToRepair.push_back(i);
        }
    }

    if (slotsToRepair.empty())
        return;

    if (inventory->getMoney() < totalCost)
    {
        review_alert_label(panel_, "Not enough gold", Vec2(250, -320));
        return;
    }

    if (!inventory->removeMoney(totalCost))
        return;

    for (int slotIndex : slotsToRepair)
    {
        inventory->repairSlot(slotIndex);
    }

    refreshList();
}

