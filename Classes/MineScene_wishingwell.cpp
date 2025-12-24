// 这是临时文件，用于添加许愿池实现
// 将以下代码复制到 MineScene.cpp 文件末尾

void MineScene::initWishingWell()
{
    wishingWell_ = nullptr;
    if (!mineLayer_) return;

    // 在地图上找一个位置放置许愿池
    Vec2 pos = getRandomWalkablePosition();
    
    // 创建许愿池视觉 (蓝色圆形)
    auto wellNode = DrawNode::create();
    wellNode->drawDot(Vec2::ZERO, 20, Color4F(0.2f, 0.4f, 1.0f, 0.8f)); // 水池
    wellNode->drawCircle(Vec2::ZERO, 22, 0, 30, false, Color4F::GRAY); // 边框
    
    wishingWell_ = Node::create();
    wishingWell_->setPosition(pos);
    wishingWell_->addChild(wellNode);
    this->addChild(wishingWell_, 5); // 与宝箱同层
    
    // 提示文字
    auto label = Label::createWithSystemFont("Wishing Well\n(Press K)", "Arial", 12);
    label->setPosition(Vec2(0, 30));
    label->setAlignment(TextHAlignment::CENTER);
    wishingWell_->addChild(label);
    
    CCLOG("Wishing Well initialized at (%.1f, %.1f)", pos.x, pos.y);
}

void MineScene::handleWishAction()
{
    if (!player_ || !wishingWell_ || !inventory_) return;
    
    // 检查距离
    float dist = player_->getPosition().distance(wishingWell_->getPosition());
    if (dist > 60.0f)
    {
        showActionMessage("Too far from Wishing Well!", Color3B::GRAY);
        return;
    }
    
    // 检查当前手持物品
    ItemType currentItem = inventory_->getSlot(selectedItemIndex_).type;
    if (currentItem == ItemType::None)
    {
        showActionMessage("Hold an item to wish!", Color3B::YELLOW);
        return;
    }
    
    // 扣除物品
    if (inventory_->removeItem(currentItem, 1))
    {
        // 播放效果（简单震动或颜色变化）
        wishingWell_->runAction(Sequence::create(
            ScaleTo::create(0.1f, 1.2f),
            ScaleTo::create(0.1f, 1.0f),
            nullptr
        ));
        
        // 随机奖励
        int randVal = rand() % 100;
        if (randVal < 30) // 30% 啥也没有
        {
            showActionMessage("The well is silent...", Color3B::GRAY);
        }
        else if (randVal < 70) // 40% 金币 / 回血
        {
            if (rand() % 2 == 0)
            {
                int gold = 50 + rand() % 151; // 50-200
                inventory_->addMoney(gold);
                showActionMessage(StringUtils::format("Well grants %d Gold!", gold), Color3B::YELLOW);
            }
            else
            {
                int heal = 20 + rand() % 31; // 20-50
                player_->heal(heal);
                showActionMessage("You feel refreshed!", Color3B::GREEN);
            }
        }
        else // 30% 好东西
        {
            // 随机给个矿石或更稀有的
            ItemType rewards[] = {ItemType::GoldOre, ItemType::DiamondSword, ItemType::GoldSword};
            ItemType reward = rewards[rand() % 3];
            
            // 如果是武器且已有，折算成钱
            if ((reward == ItemType::DiamondSword || reward == ItemType::GoldSword) && inventory_->hasItem(reward, 1))
            {
                 int gold = 500;
                 inventory_->addMoney(gold);
                 showActionMessage("Great fortune! (500 Gold)", Color3B::ORANGE);
            }
            else
            {
                if (inventory_->addItem(reward, 1))
                {
                    showActionMessage(StringUtils::format("Received %s!", InventoryManager::getItemName(reward).c_str()), Color3B::MAGENTA);
                }
                else
                {
                    //背包满了
                     int gold = Weapon::getWeaponPrice(reward); 
                     inventory_->addMoney(gold);
                     showActionMessage("Bag full, took Gold instead.", Color3B::ORANGE);
                }
            }
        }
        
        // 刷新UI
        updateUI(); 
    }
}
