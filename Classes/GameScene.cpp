#include "GameScene.h"

#include <algorithm>
#include <cmath>
#include <queue>
#include <unordered_set>

#include "BarnScene.h"
#include "BeachScene.h"
#include "BlacksmithUI.h"
#include "ElevatorUI.h"
#include "EnergyBar.h"
#include "FarmManager.h"
#include "Fish.h"
#include "HouseScene.h"
#include "MarketUI.h"
#include "MenuScene.h"
#include "QuantityPopup.h"
#include "SkillManager.h"
#include "SkillTreeUI.h"
#include "TimeManager.h"
#include "WeatherManager.h"

USING_NS_CC;

namespace {
const Vec2 kHouseDoorTile(18.0f, 14.0f);
const float kHouseDoorRadius = 40.0f;
const Vec2 kBarnDoorTile(12.0f, 13.0f);
const float kBarnDoorRadius = 60.0f;
const Color4B kDayLightColor(255, 255, 255, 0);
const Color4B kDawnLightColor(255, 180, 120, 70);
const Color4B kDuskLightColor(120, 100, 160, 110);
const Color4B kNightLightColor(20, 30, 60, 160);
const float kToolbarSlotSize = 48.0f;
const float kToolbarSlotPadding = 6.0f;
const float kToolbarIconPadding = 6.0f;
const Color4B kToolbarBarColor(40, 35, 30, 220);
const Color3B kToolbarSlotColor(70, 60, 50);
const Color3B kToolbarSlotSelectedColor(170, 150, 95);

const Vec2 kMerchantTile(35.0f, 9.0f);
const Vec2 kBlacksmithTile(6.0f, 4.0f);
const float kInteractionRadius = 80.0f;

Color4B lerpColor(const Color4B& from, const Color4B& to, float t) {
  t = clampf(t, 0.0f, 1.0f);
  auto lerpByte = [t](GLubyte a, GLubyte b) {
    return static_cast<GLubyte>(a + (b - a) * t);
  };
  return Color4B(lerpByte(from.r, to.r), lerpByte(from.g, to.g),
                 lerpByte(from.b, to.b), lerpByte(from.a, to.a));
}

Color4B getAmbientColorForHour(float hour) {
  if (hour < 5.0f) return kNightLightColor;
  if (hour < 6.0f)
    return lerpColor(kNightLightColor, kDawnLightColor, (hour - 5.0f) / 1.0f);
  if (hour < 7.0f)
    return lerpColor(kDawnLightColor, kDayLightColor, (hour - 6.0f) / 1.0f);
  if (hour < 18.0f) return kDayLightColor;
  if (hour < 19.0f)
    return lerpColor(kDayLightColor, kDuskLightColor, (hour - 18.0f) / 1.0f);
  if (hour < 20.0f)
    return lerpColor(kDuskLightColor, kNightLightColor, (hour - 19.0f) / 1.0f);
  return kNightLightColor;
}

Sprite* createToolbarIcon(ItemType itemType) {
  std::string path = InventoryManager::getItemIconPath(itemType);
  if (path.empty() || !FileUtils::getInstance()->isFileExist(path)) {
    return nullptr;
  }

  auto icon = Sprite::create(path);
  if (!icon) {
    return nullptr;
  }

  auto size = icon->getContentSize();
  float maxSize = kToolbarSlotSize - kToolbarIconPadding * 2.0f;
  float scale = std::min(maxSize / size.width, maxSize / size.height);
  icon->setScale(scale);
  return icon;
}
}  // namespace

Scene* GameScene::createScene()

{
  return GameScene::create();
}

bool GameScene::init()

{
  // ...
  merchantState_ = MerchantState::None;
  activeNpc_ = nullptr;

  if (!Scene::init()) return false;

  // 初始化各个组件
  mapLayer_ = nullptr;
  farmManager_ = nullptr;
  player_ = nullptr;
  weatherManager_ = nullptr;
  uiLayer_ = nullptr;
  dayNightLayer_ = nullptr;
  inventory_ = nullptr;
  inventoryUI_ = nullptr;
  marketUI_ = nullptr;
  skillUI_ = nullptr;
  enteringBeach_ = false;

  initMap();
  initFarm();
  initTrees();
  initPlayer();
  initCamera();
  initUI();
  initControls();

  // 初始化背包系统
  inventory_ = InventoryManager::getInstance();
  if (inventory_) {
    // 不再添加到场景，避免随场景销毁
  }
  SkillManager::getInstance();
  marketState_.init();
  initWeather();
  initNpcs();

  TimeManager::getInstance();

  auto mouseListener = EventListenerMouse::create();
  mouseListener->onMouseDown = CC_CALLBACK_1(GameScene::onMouseDown, this);
  auto mouseUpListener = EventListenerMouse::create();
  mouseUpListener->onMouseUp = CC_CALLBACK_1(GameScene::onMouseUp, this);
  _eventDispatcher->addEventListenerWithSceneGraphPriority(mouseListener, this);
  _eventDispatcher->addEventListenerWithSceneGraphPriority(mouseUpListener,
                                                           this);

  // 启动更新
  this->scheduleUpdate();

  return true;
}

// ========== 初始化地图 ==========

void GameScene::initMap()

{
  // 创建地图层
  FileUtils::getInstance()->purgeCachedEntries();
  SpriteFrameCache::getInstance()->removeUnusedSpriteFrames();
  Director::getInstance()->getTextureCache()->removeUnusedTextures();

  mapLayer_ = MapLayer::create("map/farm.tmx");

  if (mapLayer_) {
    this->addChild(mapLayer_, -1);
  }

  else

  {
  }
}

void GameScene::initFarm()

{
  if (!mapLayer_) {
    return;
  }

  farmManager_ = FarmManager::create(mapLayer_);

  if (farmManager_) {
    mapLayer_->addChild(farmManager_, 5);

    // 设置交易箱回调
    farmManager_->setPriceFunction(
        [this](ItemType type) { return marketState_.getSellPrice(type); });

    farmManager_->setEarningsCallback([this](int earnings) {
      if (inventory_) {
        inventory_->addMoney(earnings);
      }
      showActionMessage(StringUtils::format("Shipping: +%d G", earnings),
                        Color3B(255, 215, 0));
    });

    if (player_) {
      player_->setFarmManager(farmManager_);
    }
  }

  else {
  }
}

// ========== 初始化玩家 ==========

void GameScene::initPlayer()

{
  // 创建玩家

  player_ = Player::create();

  if (player_) {
    if (mapLayer_) {
      Vec2 preferredPos(400.0f, 300.0f);
      if (mapLayer_->isWalkable(preferredPos)) {
        player_->setPosition(preferredPos);
      } else {
        Size mapSize = mapLayer_->getMapSize();
        Vec2 centerPos = Vec2(mapSize.width / 2, mapSize.height / 2);

        bool centerWalkable = mapLayer_->isWalkable(centerPos);
        if (!centerWalkable) {
          Vec2 safePos = centerPos;
          bool foundSafe = false;

          for (int radius = 1; radius < 10 && !foundSafe; radius++) {
            for (int dx = -radius; dx <= radius && !foundSafe; dx++) {
              for (int dy = -radius; dy <= radius && !foundSafe; dy++) {
                Vec2 testPos = centerPos + Vec2(dx * 32, dy * 32);
                if (mapLayer_->isWalkable(testPos)) {
                  safePos = testPos;
                  foundSafe = true;
                }
              }
            }
          }

          if (!foundSafe) {
            safePos = Vec2(100, 100);
          }

          player_->setPosition(safePos);
        } else {
          player_->setPosition(centerPos);
        }
      }
    } else {
      auto visibleSize = Director::getInstance()->getVisibleSize();
      player_->setPosition(Vec2(visibleSize.width / 2, visibleSize.height / 2));
    }

    player_->enableKeyboardControl();

    if (mapLayer_) {
      player_->setMapLayer(mapLayer_);
    }

    if (farmManager_) {
      player_->setFarmManager(farmManager_);
    }

    this->addChild(player_, 10);
  } else {
  }
}

// ========== 初始化摄像机 ==========

void GameScene::initCamera()

{
  // 获取默认摄像机

  auto camera = this->getDefaultCamera();

  if (camera && player_)

  {
    // 摄像机初始位置跟随玩家

    Vec2 playerPos = player_->getPosition();

    camera->setPosition(playerPos.x, playerPos.y);
  }
}

// ========== 初始化界面 ==========

void GameScene::initUI()

{
  auto visibleSize = Director::getInstance()->getVisibleSize();
  auto origin = Director::getInstance()->getVisibleOrigin();

  // 创建界面层（独立于摄像机移动）

  uiLayer_ = Layer::create();

  uiLayer_->setGlobalZOrder(1000);  // 设为较高层级，确保显示在最上层

  this->addChild(uiLayer_, 1000);

  dayNightLayer_ = LayerColor::create(Color4B(0, 0, 0, 0), visibleSize.width,
                                      visibleSize.height);
  dayNightLayer_->setPosition(Vec2(origin.x, origin.y));
  uiLayer_->addChild(dayNightLayer_, -2);

  // ===== 顶部信息栏背景 =====

  auto topBar =
      LayerColor::create(Color4B(0, 0, 0, 180), visibleSize.width, 60);

  topBar->setAnchorPoint(Vec2(0, 1));  // 锚点在左上角

  topBar->setPosition(Vec2(origin.x, origin.y + visibleSize.height));

  uiLayer_->addChild(topBar, 0);

  // ===== 时间显示 =====

  timeLabel_ =
      Label::createWithSystemFont("Day 1 (auto +1 every 5s)", "Arial", 24);

  timeLabel_->setAnchorPoint(Vec2(0, 0.5));

  timeLabel_->setPosition(Vec2(

      origin.x + 20,

      origin.y + visibleSize.height - 30

      ));

  timeLabel_->setColor(Color3B::WHITE);

  uiLayer_->addChild(timeLabel_, 1);

  // ===== 金币显示 =====

  moneyLabel_ = Label::createWithSystemFont("Gold: 500", "Arial", 24);

  moneyLabel_->setAnchorPoint(Vec2(1, 0.5));

  moneyLabel_->setPosition(Vec2(

      origin.x + visibleSize.width - 20,

      origin.y + visibleSize.height - 30

      ));

  moneyLabel_->setColor(Color3B(255, 215, 0));

  uiLayer_->addChild(moneyLabel_, 1);

  // ===== 位置显示（调试用）=====

  positionLabel_ = Label::createWithSystemFont(
      "Position: (0, 0) | Tile: (0, 0)", "Arial", 18);

  positionLabel_->setPosition(Vec2(

      origin.x + visibleSize.width / 2,

      origin.y + visibleSize.height - 70

      ));

  positionLabel_->setColor(Color3B(200, 200, 200));

  uiLayer_->addChild(positionLabel_, 1);

  // ===== 操作提示 =====

  auto hint = Label::createWithSystemFont(
      "1-8: Switch item | J: Use | K: Water | B: Inventory | E: Skills | P: "
      "Market | M: Mine | ESC: Menu",
      "Arial", 18);

  hint->setPosition(Vec2(

      origin.x + visibleSize.width / 2,

      origin.y + 30

      ));

  hint->setColor(Color3B(200, 200, 200));

  uiLayer_->addChild(hint, 1);

  // ===== 农场操作提示 =====

  actionLabel_ = Label::createWithSystemFont("", "Arial", 20);

  actionLabel_->setPosition(Vec2(

      origin.x + visibleSize.width / 2,

      origin.y + 60

      ));

  actionLabel_->setColor(Color3B::WHITE);

  uiLayer_->addChild(actionLabel_, 1);

  // ===== 当前物品显示 =====
  itemLabel_ = Label::createWithSystemFont("Current item: Hoe (1-8 to switch)",
                                           "Arial", 18);
  itemLabel_->setAnchorPoint(Vec2(0, 0.5f));
  itemLabel_->setPosition(Vec2(origin.x + 20, origin.y + 60));
  itemLabel_->setColor(Color3B(200, 255, 200));
  uiLayer_->addChild(itemLabel_, 1);

  initToolbar();
  initToolbarUI();

  // ===== 钓鱼界面初始化（跟随玩家） =====
  if (player_) {
    chargeBarBg_ = Sprite::create();
    chargeBarBg_->setTextureRect(Rect(0, 0, 50, 8));
    chargeBarBg_->setColor(Color3B::GRAY);
    chargeBarBg_->setPosition(Vec2(16, 70));
    chargeBarBg_->setVisible(false);
    player_->addChild(chargeBarBg_);

    chargeBarFg_ = Sprite::create();
    chargeBarFg_->setTextureRect(Rect(0, 0, 0, 8));
    chargeBarFg_->setColor(Color3B::GREEN);
    chargeBarFg_->setAnchorPoint(Vec2(0, 0));
    chargeBarFg_->setPosition(Vec2(0, 0));
    chargeBarBg_->addChild(chargeBarFg_);

    exclamationMark_ = Sprite::create();
    exclamationMark_->setTextureRect(Rect(0, 0, 10, 30));
    exclamationMark_->setColor(Color3B::YELLOW);
    exclamationMark_->setPosition(Vec2(16, 90));
    exclamationMark_->setVisible(false);
    player_->addChild(exclamationMark_);
  }

  updateDayNightLighting();

  // 能量条
  if (player_) {
    auto energyBar = EnergyBar::create(player_);
    if (energyBar) {
      energyBar->setName("EnergyBar");
      this->addChild(energyBar, 100);
    }
  }
}

void GameScene::initWeather()

{
  if (!uiLayer_)

  {
    return;
  }

  weatherManager_ = WeatherManager::create();

  if (weatherManager_)

  {
    uiLayer_->addChild(weatherManager_, -1);

    lastWeatherDay_ = 0;

    updateWeather();

  }

  else

  {
  }
}

// ========== 初始化非玩家角色 ==========
void GameScene::initNpcs() {
  if (!mapLayer_ || !uiLayer_) return;

  auto wizard = Npc::create("Wizard", "NPC/bussiness_person_processed.png",
                            Npc::NpcType::Merchant);
  if (wizard) {
    wizard->setPosition(mapLayer_->tileCoordToPosition(kMerchantTile));
    mapLayer_->addChild(wizard, 10);
    npcs_.push_back(wizard);
  }

  auto blacksmith = Npc::create("Blacksmith", "NPC/blacksmith_processed.png",
                                Npc::NpcType::Blacksmith);
  if (blacksmith) {
    blacksmith->setPosition(mapLayer_->tileCoordToPosition(kBlacksmithTile));
    mapLayer_->addChild(blacksmith, 10);
    npcs_.push_back(blacksmith);
  }

  dialogueBox_ = DialogueBox::create(nullptr);
  if (dialogueBox_) {
    uiLayer_->addChild(dialogueBox_, 2000);
    dialogueBox_->setVisible(false);
  }
}

// ========== 初始化控制 ==========

void GameScene::initControls()

{
  // 退出键监听（返回菜单）

  auto keyListener = EventListenerKeyboard::create();

  keyListener->onKeyPressed = CC_CALLBACK_2(GameScene::onKeyPressed, this);

  _eventDispatcher->addEventListenerWithSceneGraphPriority(keyListener, this);
}

void GameScene::onKeyPressed(EventKeyboard::KeyCode keyCode, Event* event)

{
  switch (keyCode) {
    case EventKeyboard::KeyCode::KEY_ESCAPE:
      backToMenu();
      break;
    case EventKeyboard::KeyCode::KEY_B:
      if (merchantState_ != MerchantState::None) {
        if (merchantState_ == MerchantState::Buy && marketUI_) {
          marketUI_->close();
          merchantState_ = MerchantState::Choice;
          if (dialogueBox_) {
            dialogueBox_->setVisible(true);
            dialogueBox_->showDialogue("Is there anything else you need?");
            dialogueBox_->showChoices("Buy", "Sell", [this](int choice) {
              this->onMerchantChoice(choice);
            });
          }
        } else if ((merchantState_ == MerchantState::SellPre ||
                    merchantState_ == MerchantState::Sell) &&
                   inventoryUI_) {
          inventoryUI_->close();
          inventoryUI_ = nullptr;

          merchantState_ = MerchantState::Choice;
          if (dialogueBox_) {
            dialogueBox_->setVisible(true);
            dialogueBox_->showDialogue("Is there anything else you need?");
            dialogueBox_->showChoices("Buy", "Sell", [this](int choice) {
              this->onMerchantChoice(choice);
            });
          }
        } else {
          endMerchantInteraction();
        }
      } else {
        toggleInventory();
      }
      break;
    case EventKeyboard::KeyCode::KEY_E:
      toggleSkillTree();
      break;
    case EventKeyboard::KeyCode::KEY_X:
      // 保存游戏
      saveGame();
      break;
    case EventKeyboard::KeyCode::KEY_M:
      if (isPlayerNearElevator()) {
        enterMine();
      } else {
        showActionMessage("Elevator is too far!", Color3B::RED);
        // 调试显示位置
      }
      break;
    case EventKeyboard::KeyCode::KEY_ENTER:
    case EventKeyboard::KeyCode::KEY_KP_ENTER:
      if (isPlayerNearBarnDoor()) {
        enterBarn();
      } else if (isPlayerNearHouseDoor()) {
        enterHouse();
      } else {
        showActionMessage("Door is too far!", Color3B::RED);
      }
      break;
    case EventKeyboard::KeyCode::KEY_J:
      handleFarmAction(false);
      break;
    case EventKeyboard::KeyCode::KEY_K:
      handleChestPlacement();
      break;
    case EventKeyboard::KeyCode::KEY_SPACE: {
      if (npcs_.size() > 0 && farmManager_ && player_) {
        Npc* merchant = nullptr;
        for (auto npc : npcs_) {
          if (npc->isMerchant()) {
            merchant = npc;
            break;
          }
        }

        if (merchant) {
          float dist = player_->getPosition().distance(merchant->getPosition());
          if (dist < kInteractionRadius) {
            startMerchantInteraction(merchant);
            break;
          }
        }

        Npc* blacksmith = nullptr;
        for (auto npc : npcs_) {
          if (npc->getNpcName() == "Blacksmith") {
            blacksmith = npc;
            break;
          }
        }

        if (blacksmith) {
          float dist =
              player_->getPosition().distance(blacksmith->getPosition());
          if (dist < kInteractionRadius) {
            auto ui = BlacksmithUI::create();
            ui->show();
            uiLayer_->addChild(ui, 200);
            break;
          }
        }
      }

      // 2. 查找附近箱子并打开（沿用现有逻辑）
      if (farmManager_ && mapLayer_ && player_) {
        Vec2 tileCoord = mapLayer_->positionToTileCoord(player_->getPosition());
        tileCoord.x = std::round(tileCoord.x);
        tileCoord.y = std::round(tileCoord.y);

        // 检查周围 1 格的箱子
        StorageChest* nearbyChest = nullptr;
        for (float dy = -1; dy <= 1; ++dy) {
          for (float dx = -1; dx <= 1; ++dx) {
            nearbyChest =
                farmManager_->getStorageChestAt(tileCoord + Vec2(dx, dy));
            if (nearbyChest) break;
          }
          if (nearbyChest) break;
        }

        if (nearbyChest) {
          openChestInventory(nearbyChest);
        }
      }
    } break;
    case EventKeyboard::KeyCode::KEY_1:
      selectItemByIndex(0);
      break;
    case EventKeyboard::KeyCode::KEY_2:
      selectItemByIndex(1);
      break;
    case EventKeyboard::KeyCode::KEY_3:
      selectItemByIndex(2);
      break;
    case EventKeyboard::KeyCode::KEY_4:
      selectItemByIndex(3);
      break;
    case EventKeyboard::KeyCode::KEY_5:
      selectItemByIndex(4);
      break;
    case EventKeyboard::KeyCode::KEY_6:
      selectItemByIndex(5);
      break;
    case EventKeyboard::KeyCode::KEY_7:
      selectItemByIndex(6);
      break;
    case EventKeyboard::KeyCode::KEY_8:
      selectItemByIndex(7);
      break;
    case EventKeyboard::KeyCode::KEY_TAB: {
      auto tm = TimeManager::getInstance();
      if (tm) {
        saveGame();

        tm->skipToNextMorning();

        Director::getInstance()->replaceScene(
            TransitionFade::create(1.0f, HouseScene::createScene(true)));
      }
      break;
    }
  }
}

// ========== 更新函数 ==========

void GameScene::update(float delta) {
  // 更新摄像机（跟随玩家）
  updateCamera();

  // 更新界面显示
  updateUI();

  // 天气系统已移除

  updateDayNightLighting();

  // 处理砍树计时
  updateChopping(delta);

  // 更新钓鱼状态
  updateFishingState(delta);

  checkBeachEntrance();

  auto tm = TimeManager::getInstance();
  tm->update(delta);

  if (tm->isMidnight()) {
    if (inventory_) inventory_->removeMoney(200);
    showActionMessage("Passed out...", Color3B::RED);
    Director::getInstance()->replaceScene(
        TransitionFade::create(1.0f, HouseScene::createScene(true)));
    return;
  }
}

void GameScene::updateCamera() {
  if (!player_) return;

  auto camera = this->getDefaultCamera();
  if (!camera) return;

  // 获取玩家位置
  Vec2 playerPos = player_->getPosition();

  // 摄像机跟随玩家（平滑移动）
  Vec3 currentPos = camera->getPosition3D();
  Vec3 targetPos = Vec3(playerPos.x, playerPos.y, currentPos.z);

  // 线性插值实现平滑跟随
  float smoothFactor = 0.1f;  // 平滑系数（0-1，越大越快）
  Vec3 newPos = currentPos + (targetPos - currentPos) * smoothFactor;

  camera->setPosition3D(newPos);

  // 限制摄像机范围（不超出地图边界）

  if (mapLayer_)

  {
    auto visibleSize = Director::getInstance()->getVisibleSize();

    Size mapSize = mapLayer_->getMapSize();

    float minX = visibleSize.width / 2;

    float maxX = mapSize.width - visibleSize.width / 2;

    float minY = visibleSize.height / 2;

    float maxY = mapSize.height - visibleSize.height / 2;

    // 如果地图小于屏幕，居中显示

    if (mapSize.width < visibleSize.width)

    {
      minX = maxX = mapSize.width / 2;
    }

    if (mapSize.height < visibleSize.height)

    {
      minY = maxY = mapSize.height / 2;
    }

    Vec3 clampedPos = newPos;

    clampedPos.x = std::max(minX, std::min(maxX, newPos.x));

    clampedPos.y = std::max(minY, std::min(maxY, newPos.y));

    camera->setPosition3D(clampedPos);
  }

  if (uiLayer_)

  {
    Vec3 cameraPos = camera->getPosition3D();

    auto visibleSize = Director::getInstance()->getVisibleSize();

    // 界面层的位置 = 摄像机位置 - 屏幕中心偏移

    Vec2 uiPos = Vec2(

        cameraPos.x - visibleSize.width / 2,

        cameraPos.y - visibleSize.height / 2

    );

    uiLayer_->setPosition(uiPos);
  }
}

void GameScene::updateUI()

{
  if (!player_) return;

  // 更新位置显示

  Vec2 playerPos = player_->getPosition();

  char posStr[64];

  sprintf(posStr, "Position: (%.0f, %.0f)", playerPos.x, playerPos.y);

  positionLabel_->setString(posStr);

  auto tm = TimeManager::getInstance();
  if (tm && timeLabel_) {
    std::string timeStr = StringUtils::format("Day %d, %02d:%02d", tm->getDay(),
                                              tm->getHour(), tm->getMinute());
    timeLabel_->setString(timeStr);
  }

  // 更新能量条位置（始终在右下角，跟随摄像机）
  auto energyBar = this->getChildByName("EnergyBar");
  auto camera = this->getDefaultCamera();
  if (energyBar && camera) {
    auto visibleSize = Director::getInstance()->getVisibleSize();
    energyBar->setPosition(
        Vec2(camera->getPositionX() + visibleSize.width / 2 - 50,
             camera->getPositionY() - visibleSize.height / 2 + 110));
  }
  if (moneyLabel_ && inventory_) {
    moneyLabel_->setString(
        StringUtils::format("Gold: %d", inventory_->getMoney()));
  }

  // 如果有地图层，也显示瓦片坐标

  if (mapLayer_)

  {
    Vec2 tileCoord = mapLayer_->positionToTileCoord(playerPos);

    sprintf(posStr, "Position: (%.0f, %.0f) | Tile: (%.0f, %.0f)",

            playerPos.x, playerPos.y, tileCoord.x, tileCoord.y);

    positionLabel_->setString(posStr);
  }

  refreshToolbarUI();
}

void GameScene::updateWeather() {
  int dayCount = 1;
  if (farmManager_) {
    dayCount = farmManager_->getDayCount();
  }
  if (dayCount <= 0) {
    dayCount = 1;
  }
  if (dayCount == lastWeatherDay_) {
    return;
  }

  lastWeatherDay_ = dayCount;
  marketState_.updatePrices(dayCount);
  if (weatherManager_) {
    weatherManager_->updateWeather(marketState_.getWeather());
  }
}

void GameScene::updateDayNightLighting() {
  auto tm = TimeManager::getInstance();
  if (!tm || !dayNightLayer_) return;

  float hour = tm->getHour() + tm->getMinute() / 60.0f;

  Color4B ambient = getAmbientColorForHour(hour);
  dayNightLayer_->setColor(Color3B(ambient.r, ambient.g, ambient.b));
  dayNightLayer_->setOpacity(ambient.a);
}

void GameScene::handleFarmAction(bool waterOnly) {
  // 0. 安全检查
  if (!mapLayer_ || !farmManager_ || !player_) return;

  // 1. 立即播放玩家动画
  // （角色类会根据当前物品类型自动决定播放挥锄或挥斧）
  player_->playSwingAnimation();

  // 计算玩家当前脚下的瓦片坐标
  Vec2 tileCoord = mapLayer_->positionToTileCoord(player_->getPosition());
  tileCoord.x = std::round(tileCoord.x);
  tileCoord.y = std::round(tileCoord.y);

  // 获取当前工具类型
  ItemType current =
      toolbarItems_.empty() ? ItemType::Hoe : toolbarItems_[selectedItemIndex_];

  // ======================================================================================
  // 2. 定义核心逻辑闭包
  //    将所有“改变游戏数据”的代码封装在这里，方便放入延迟回调中执行
  // ======================================================================================
  auto executeAction = [this, current, tileCoord, waterOnly]() {
    FarmManager::ActionResult result{false, "Unavailable", -1};

    // --- 分支一：仅浇水模式（按浇水键） ---
    if (waterOnly) {
      if (current == ItemType::WateringCan) {
        result = farmManager_->waterTile(tileCoord);
        if (result.success) {
          player_->consumeEnergy(2.0f);
          SkillManager::getInstance()->recordAction(
              SkillManager::SkillType::Agriculture);
          if (inventory_->decreaseDurability(selectedItemIndex_, 1)) {
            showActionMessage("Watering Can broke!", Color3B::RED);
            refreshToolbarUI();
          }
        }
      } else {
        result = {false, "Need watering can to water", -1};
      }
    }
    // --- 分支二：通用工具模式（按工具键） ---
    else {
      switch (current) {
          // 情况一：锄头（耕地）
        case ItemType::Hoe:
          result = farmManager_->tillTile(tileCoord);
          if (result.success) {
            player_->consumeEnergy(2.0f);
            SkillManager::getInstance()->recordAction(
                SkillManager::SkillType::Agriculture);
            if (inventory_->decreaseDurability(selectedItemIndex_, 1)) {
              showActionMessage("Hoe broke!", Color3B::RED);
              refreshToolbarUI();
            }
          }
          break;

          // 情况二：水壶（也可用工具键浇水）
        case ItemType::WateringCan:
          result = farmManager_->waterTile(tileCoord);
          if (result.success) {
            player_->consumeEnergy(2.0f);
            SkillManager::getInstance()->recordAction(
                SkillManager::SkillType::Agriculture);
            if (inventory_->decreaseDurability(selectedItemIndex_, 1)) {
              showActionMessage("Watering Can broke!", Color3B::RED);
              refreshToolbarUI();
            }
          }
          break;

          // 情况三：镰刀（收获）
        case ItemType::Scythe:
          result = farmManager_->harvestTile(tileCoord);
          if (result.success && inventory_) {
            ItemType harvestItem = getItemTypeForCropId(result.cropId);
            if (harvestItem != ItemType::ITEM_NONE) {
              if (inventory_->addItem(harvestItem, 1)) {
                int extraCount = 0;
                float bonusChance =
                    SkillManager::getInstance()->getAgricultureBonusChance();
                if (bonusChance > 0.0f && CCRANDOM_0_1() < bonusChance) {
                  if (inventory_->addItem(harvestItem, 1)) extraCount = 1;
                }
                if (extraCount > 0) {
                  result.message = StringUtils::format(
                      "Harvested %s (+%d)",
                      InventoryManager::getItemName(harvestItem).c_str(),
                      1 + extraCount);
                } else {
                  result.message = StringUtils::format(
                      "Harvested %s (+1)",
                      InventoryManager::getItemName(harvestItem).c_str());
                }
                if (inventoryUI_) inventoryUI_->refresh();
              } else {
                result.message = "Inventory full!";
              }
            }
            SkillManager::getInstance()->recordAction(
                SkillManager::SkillType::Agriculture);
            if (inventory_->decreaseDurability(selectedItemIndex_, 1)) {
              showActionMessage("Scythe broke!", Color3B::RED);
              refreshToolbarUI();
            }
            if (inventoryUI_) inventoryUI_->refresh();
          }
          break;

          // 情况四：斧头（砍树，逻辑较复杂）
        case ItemType::Axe: {
          Vec2 target = tileCoord;
          // 尝试寻找附近的树根
          if (!findNearbyCollisionTile(tileCoord, target)) {
            result = {false, "No tree to chop here", -1};
            break;
          }

          std::vector<Vec2> component = collectCollisionComponent(target);
          if (component.empty() || component.size() > 10) {
            result = {false, "No tree to chop here", -1};
            break;
          }

          // 查找是否已经在砍这棵树
          TreeChopData* existingChop = nullptr;
          for (auto& chop : activeChops_) {
            if (chop.tileCoord == component.front()) {
              existingChop = &chop;
              break;
            }
          }

          const int REPLACE_THRESHOLD = 3;  // 第 3 刀替换为精灵

          if (existingChop) {
            // --- 已经在砍了 (第 2, 3, 4... 刀) ---
            existingChop->chopCount++;
            existingChop->chopTimer = 0.0f;

            // 第 3 刀：瓦片替换为精灵
            if (existingChop->chopCount == REPLACE_THRESHOLD &&
                existingChop->treeSprite == nullptr) {
              existingChop->treeSprite = createTreeSprite(existingChop->tiles);
              showActionMessage("Tree is loose!", Color3B::YELLOW);
            }

            // 播放树木受击摇晃动画
            if (existingChop->treeSprite) {
              playTreeShakeAnimation(existingChop->treeSprite);
            } else {
              showActionMessage(
                  "Thump! (" + std::to_string(existingChop->chopCount) + ")",
                  Color3B::WHITE);
            }

            // 检查是否砍倒 (假设 6 刀)
            if (existingChop->chopCount >= TreeChopData::CHOPS_NEEDED) {
              if (existingChop->treeSprite) {
                playTreeFallAnimation(existingChop);
                result = {true, "Timber!", -1};
              } else {
                result = {true, "Tree destroyed", -1};
              }
            } else {
              result = {true, "", -1};  // 砍到了但没倒，不弹提示以免刷屏
            }

            if (inventory_->decreaseDurability(selectedItemIndex_, 1)) {
              showActionMessage("Axe broke!", Color3B::RED);
              refreshToolbarUI();
            }
          } else {
            // --- 第 1 刀 (新砍伐) ---
            TreeChopData newChop;
            newChop.tileCoord = component.front();
            newChop.tiles = component;
            newChop.chopCount = 1;
            newChop.chopTimer = 0.0f;
            newChop.treeSprite = nullptr;  // 第 1 刀不生成精灵

            activeChops_.push_back(newChop);

            showActionMessage("Thump! (1)", Color3B::WHITE);
            player_->consumeEnergy(2.0f);
            result = {true, "", -1};

            if (inventory_->decreaseDurability(selectedItemIndex_, 1)) {
              showActionMessage("Axe broke!", Color3B::RED);
              refreshToolbarUI();
            }
          }
          break;
        }

        // 情况五：镐子（碎石）
        case ItemType::Pickaxe: {
          Vec2 target = tileCoord;
          if (!findNearbyCollisionTile(tileCoord, target)) {
            result = {false, "No rock to mine here", -1};
            break;
          }

          // 石头图块编号列表（请确保编号与地图一致）
          static const std::unordered_set<int> rockGids = {
              45019, 45020, 45021, 45025, 45026, 45027, 45028, 45030, 45069,
              45070, 45071, 45072, 45073, 45170, 45171, 45172
              // 如果还有更多石头编号，请补全
          };

          int baseGid = mapLayer_->getBaseTileGID(target);

          // 宽容判断：只要有碰撞且像石头（也可去掉石头编号检查，只依赖碰撞）
          // 这里保留你的逻辑，石头编号不全可能导致敲不碎

          std::vector<Vec2> component = collectCollisionComponent(target);
          if (component.empty()) {
            result = {false, "No rock here", -1};
            break;
          }

          for (const auto& t : component) {
            mapLayer_->clearCollisionAt(t);
            mapLayer_->clearBaseTileAt(t);
          }

          int reduction = SkillManager::getInstance()->getMiningHitReduction();
          // 每级减少 0.4 体力消耗（最高 5 级，共减少 2.0）
          float cost = 4.0f - (reduction * 0.4f);
          cost = std::max(1.0f, cost);

          player_->consumeEnergy(cost);
          result = {true, "Rock broken!", -1};
          SkillManager::getInstance()->recordAction(
              SkillManager::SkillType::Mining);
          if (inventory_->decreaseDurability(selectedItemIndex_, 1)) {
            showActionMessage("Pickaxe broke!", Color3B::RED);
            refreshToolbarUI();
          }
          if (inventoryUI_) inventoryUI_->refresh();
          break;
        }

        // 情况六：各种种子（播种）
        case ItemType::SeedTurnip:
        case ItemType::SeedPotato:
        case ItemType::SeedCorn:
        case ItemType::SeedTomato:
        case ItemType::SeedPumpkin:
        case ItemType::SeedBlueberry: {
          if (!inventory_ || !inventory_->hasItem(current, 1)) {
            result = {false, "No seeds left", -1};
            break;
          }
          int cropId = getCropIdForItem(current);
          result = farmManager_->plantSeed(tileCoord, cropId);
          if (result.success && inventory_) {
            inventory_->removeItem(current, 1);
            player_->consumeEnergy(2.0f);
            result.message = StringUtils::format(
                "Planted %s (-1)",
                InventoryManager::getItemName(current).c_str());
            if (inventoryUI_) inventoryUI_->refresh();
            SkillManager::getInstance()->recordAction(
                SkillManager::SkillType::Agriculture);
          }
          break;
        }

        default:
          result = {false, "Unknown item action", -1};
          break;
      }
    }

    // 统一显示操作结果提示
    if (!result.message.empty()) {
      Color3B color =
          result.success ? Color3B(120, 230, 140) : Color3B(255, 180, 120);
      showActionMessage(result.message, color);
    }
  };

  // ======================================================================================
  // 3. 动作执行策略：延迟或立即
  // ======================================================================================

  float energyPercent = player_->getCurrentEnergy() / player_->getMaxEnergy();

  // 红色能量 (20%)：禁止使用工具
  if (energyPercent <= 0.2f &&
      (current == ItemType::Hoe || current == ItemType::Pickaxe ||
       current == ItemType::Axe || current == ItemType::WateringCan ||
       current == ItemType::Scythe)) {
    showActionMessage("Exhausted!", Color3B::RED);
    return;
  }

  // 如果是挥动类工具（锄头、镐子、斧头），我们需要配合动画的“打击点”
  // 动画总长约 0.45 秒（3 帧 × 0.15 秒），打击点通常在中间，延迟 0.2 秒
  // 黄色能量 (50%)：动作变慢，延迟增加
  float delay = 0.2f;
  if (energyPercent <= 0.5f) {
    delay = 0.4f;  // 变慢一倍
  }

  if (current == ItemType::Hoe || current == ItemType::Pickaxe ||
      current == ItemType::Axe) {
    auto seq = Sequence::create(DelayTime::create(delay),
                                CallFunc::create(executeAction), nullptr);
    this->runAction(seq);
  } else {
    // 种子、水壶等其他物品，立即执行逻辑
    executeAction();
  }
}

void GameScene::handleChestPlacement() {
  if (!farmManager_ || !player_ || !inventory_) return;

  // 获取玩家面前的格子
  Vec2 tileCoord = mapLayer_->positionToTileCoord(player_->getPosition());
  tileCoord.x = std::round(tileCoord.x);
  tileCoord.y = std::round(tileCoord.y);

  // 仅允许背包中有对应物品时放置（当前用宝箱物品占位）。
  // 如果需要更精细的道具判断，可新增储物箱类型。

  if (farmManager_->isTileClearForPlacement(tileCoord)) {
    auto chest = StorageChest::create(tileCoord);
    farmManager_->addStorageChest(chest);
    showActionMessage("Chest placed!", Color3B::GREEN);
  } else {
    showActionMessage("Cannot place here!", Color3B::RED);
  }
}

void GameScene::openChestInventory(StorageChest* chest) {
  if (!chest || !inventory_) return;

  // 如果已经打开了界面，先关闭
  if (inventoryUI_) {
    inventoryUI_->close();
  }

  // 创建背包界面，指向箱子库存
  inventoryUI_ = InventoryUI::create(chest->getInventory(), &marketState_);
  if (inventoryUI_) {
    // 伙伴背包设为玩家背包，便于快捷转移
    inventoryUI_->setPartnerInventory(inventory_, false);
    // 交易箱的卖出逻辑由背包界面状态判定。
    // 这里打开的是箱子界面，主要用于取出物品。
    inventoryUI_->setCloseCallback([this]() { onInventoryClosed(); });

    if (uiLayer_) {
      uiLayer_->addChild(inventoryUI_, 2000);
      inventoryUI_->setPosition(Vec2::ZERO);
    } else {
      this->addChild(inventoryUI_, 2000);
    }
    inventoryUI_->show();
  }
}

// ========== 砍树相关函数 ==========

// ========== 辅助函数定义（需位于农田操作函数外部） ==========

std::vector<Vec2> GameScene::collectCollisionComponent(
    const Vec2& start) const {
  std::vector<Vec2> out;
  if (!mapLayer_) return out;

  // 树木图块编号白名单
  static const std::vector<int> treeGids = {43557, 43558, 43559, 43607, 43608,
                                            43609, 43657, 43658, 43659};

  // [修改] 检查起点是否为树（树层）
  int startGid = mapLayer_->getTreeGIDAt(start);
  bool isStartTree = false;
  for (int id : treeGids)
    if (id == startGid) isStartTree = true;
  if (!isStartTree) return out;

  Size mapSize = mapLayer_->getMapSizeInTiles();
  auto key = [](int x, int y) -> long long {
    return (static_cast<long long>(y) << 32) |
           (static_cast<unsigned long long>(x) & 0xffffffffULL);
  };

  std::queue<Vec2> q;
  std::unordered_set<long long> visited;

  q.push(start);
  visited.insert(key(static_cast<int>(start.x), static_cast<int>(start.y)));
  const Vec2 dirs[4] = {Vec2(1, 0), Vec2(-1, 0), Vec2(0, 1), Vec2(0, -1)};

  while (!q.empty()) {
    Vec2 t = q.front();
    q.pop();
    out.push_back(t);

    for (const auto& d : dirs) {
      Vec2 nt(t.x + d.x, t.y + d.y);

      // 边界检查
      if (nt.x < 0 || nt.y < 0 || nt.x >= mapSize.width ||
          nt.y >= mapSize.height)
        continue;

      const long long k = key(static_cast<int>(nt.x), static_cast<int>(nt.y));
      if (visited.count(k)) continue;

      // [修改] 仅在树层判断树木（不再检查碰撞）
      int nextGid = mapLayer_->getTreeGIDAt(nt);
      bool isNextTree = false;
      for (int id : treeGids)
        if (id == nextGid) isNextTree = true;

      if (isNextTree) {
        visited.insert(k);
        q.push(nt);
      }
    }
  }
  return out;
}

bool GameScene::findNearbyCollisionTile(const Vec2& centerTile,
                                        Vec2& outTile) const {
  if (!mapLayer_) return false;

  const Vec2 offsets[5] = {Vec2(0, 0), Vec2(1, 0), Vec2(-1, 0), Vec2(0, 1),
                           Vec2(0, -1)};

  const int TREE_ROOT_GID = 43658;  // 树根图块编号

  for (const auto& off : offsets) {
    Vec2 candidate = centerTile + off;

    // 逻辑：碰撞层有碰撞且树层是树根
    if (mapLayer_->hasCollisionAt(candidate)) {
      // 使用封装接口获取树层图块编号
      int gid = mapLayer_->getTreeGIDAt(candidate);

      if (gid == TREE_ROOT_GID) {
        outTile = candidate;
        return true;
      }
    }
  }
  return false;
}

Sprite* GameScene::createTreeSprite(const std::vector<Vec2>& tiles) {
  if (tiles.empty() || !mapLayer_) return nullptr;

  Size tileSize = mapLayer_->getTileSize();

  // 1. 找到真正的树根（编号 43658）
  Vec2 rootTile(-1, -1);
  const int TREE_ROOT_GID = 43658;

  for (const auto& t : tiles) {
    int gid = mapLayer_->getTreeGIDAt(t);
    if (gid == TREE_ROOT_GID) {
      rootTile = t;
      break;
    }
  }

  if (rootTile.x < 0) {
    rootTile = tiles[0];
    for (const auto& t : tiles) {
      if (t.y > rootTile.y) rootTile = t;
    }
  }

  Vec2 rootTilePos = mapLayer_->tileCoordToPosition(rootTile);
  Vec2 spawnPosition =
      Vec2(rootTilePos.x + tileSize.width / 2.0f, rootTilePos.y);

  // ==========================================
  // 2. 直接在树层填充编号 234
  // ==========================================
  int rootTx = static_cast<int>(rootTile.x);
  int rootTy = static_cast<int>(rootTile.y);
  int targetGID = 42991;  // 指定的色块编号

  for (int x = rootTx - 1; x <= rootTx + 1; ++x) {
    for (int y = rootTy - 2; y <= rootTy; ++y) {
      Vec2 target(x, y);

      if (x >= 0 && y >= 0 && x < mapLayer_->getMapSizeInTiles().width &&
          y < mapLayer_->getMapSizeInTiles().height) {
        // 不清空不改地面，直接把树层改成编号 234
        mapLayer_->setTreeGID(target, targetGID);
      }
    }
  }

  // ==========================================
  // 3. 创建并缩放替身 (保持不变)
  // ==========================================
  auto treeSprite = Sprite::create("images/items/tree_full.png");
  if (!treeSprite) {
    return nullptr;
  }

  float targetWidth = 96.0f;
  float targetHeight = 96.0f;
  Size textureSize = treeSprite->getContentSize();
  treeSprite->setScaleX(targetWidth / textureSize.width);
  treeSprite->setScaleY(targetHeight / textureSize.height);

  treeSprite->setAnchorPoint(Vec2(0.5f, 0.0f));
  treeSprite->setPosition(spawnPosition);
  treeSprite->setUserData(static_cast<void*>(new Vec2(rootTile)));

  this->addChild(treeSprite, 100);

  return treeSprite;
}

void GameScene::playTreeShakeAnimation(Sprite* treeSprite) {
  if (!treeSprite) return;

  // 停止之前的动画
  treeSprite->stopAllActions();

  // 增强版震动动画：更长时间、更大幅度
  auto shake1 = RotateTo::create(0.08f, -12);  // 增加角度和时间
  auto shake2 = RotateTo::create(0.08f, 12);
  auto shake3 = RotateTo::create(0.08f, -10);
  auto shake4 = RotateTo::create(0.08f, 10);
  auto shake5 = RotateTo::create(0.08f, -6);
  auto shake6 = RotateTo::create(0.08f, 6);
  auto shake7 = RotateTo::create(0.08f, -3);
  auto shake8 = RotateTo::create(0.08f, 3);
  auto shake9 = RotateTo::create(0.08f, 0);

  // 增加树叶飘落效果（可选）
  auto scaleUp = ScaleTo::create(0.1f, 1.05f, 1.05f);
  auto scaleDown = ScaleTo::create(0.1f, 1.0f, 1.0f);
  auto scaleSeq = Sequence::create(scaleUp, scaleDown, nullptr);

  auto shakeSeq = Sequence::create(shake1, shake2, shake3, shake4, shake5,
                                   shake6, shake7, shake8, shake9, nullptr);

  // 同时执行摇晃和缩放
  auto spawn = Spawn::create(shakeSeq, scaleSeq, nullptr);
  treeSprite->runAction(spawn);

  // 添加震动提示文字
  auto visibleSize = Director::getInstance()->getVisibleSize();
  auto camera = this->getDefaultCamera();
  Vec3 camPos = camera->getPosition3D();

  auto hitLabel = Label::createWithSystemFont("CHOP!", "Arial", 30);
  hitLabel->setPosition(
      Vec2(treeSprite->getPosition().x, treeSprite->getPosition().y + 120));
  hitLabel->setColor(Color3B(255, 100, 100));
  this->addChild(hitLabel, 200);

  auto labelSeq =
      Sequence::create(ScaleTo::create(0.1f, 1.3f), ScaleTo::create(0.1f, 1.0f),
                       DelayTime::create(0.3f), FadeOut::create(0.3f),
                       RemoveSelf::create(), nullptr);
  hitLabel->runAction(labelSeq);
}

void GameScene::playTreeFallAnimation(TreeChopData* chopData) {
  if (!chopData || !chopData->treeSprite) return;

  auto treeSprite = chopData->treeSprite;

  // 保存关键数据
  Vec2 savedTileCoord = chopData->tileCoord;
  std::vector<Vec2> savedTiles = chopData->tiles;
  Vec2* pRootTile = static_cast<Vec2*>(treeSprite->getUserData());
  Vec2 actualRootTile = pRootTile ? *pRootTile : savedTileCoord;

  // ==========================================
  // 动画部分 (保持不变)
  // ==========================================
  auto tiltStart = RotateTo::create(0.3f, -15);
  auto fallDown = RotateTo::create(0.8f, 90);
  auto bounce = Sequence::create(RotateTo::create(0.1f, 95),
                                 RotateTo::create(0.1f, 90), nullptr);

  Vec2 currentPos = treeSprite->getPosition();
  auto moveRight = MoveTo::create(0.8f, Vec2(currentPos.x + 48, currentPos.y));

  auto scaleSeq = Sequence::create(DelayTime::create(0.3f),
                                   ScaleTo::create(0.8f, 1.0f, 0.95f), nullptr);
  auto fadeSeq =
      Sequence::create(DelayTime::create(1.1f), FadeOut::create(0.5f), nullptr);

  auto rotateSeq = Sequence::create(tiltStart, fallDown, bounce, nullptr);
  auto spawnAnim =
      Spawn::create(rotateSeq, moveRight, scaleSeq, fadeSeq, nullptr);

  // 文字特效
  auto timberLabel = Label::createWithSystemFont("TIMBER!", "Arial", 40);
  timberLabel->setPosition(
      Vec2(treeSprite->getPosition().x, treeSprite->getPosition().y + 150));
  timberLabel->setColor(Color3B(255, 200, 50));
  this->addChild(timberLabel, 200);
  auto labelAnim = Sequence::create(
      Spawn::create(ScaleTo::create(0.3f, 1.5f),
                    JumpBy::create(0.3f, Vec2(0, 0), 30, 1), nullptr),
      DelayTime::create(0.5f), FadeOut::create(0.5f), RemoveSelf::create(),
      nullptr);
  timberLabel->runAction(labelAnim);

  // ==========================================
  // 清理回调 (已移除碰撞删除逻辑)
  // ==========================================
  auto cleanup = CallFunc::create([this, actualRootTile, savedTiles,
                                   treeSprite]() {
    // 1. 清理用户数据
    Vec2* pData = static_cast<Vec2*>(treeSprite->getUserData());
    if (pData) {
      delete pData;
      treeSprite->setUserData(nullptr);
    }

    // 2. 移除倒下的树精灵
    treeSprite->removeFromParent();

    // [删除] 不再清除碰撞瓦片
    // 碰撞层保持原样，意味着玩家走过去还是会撞到隐形的墙

    // 3. 生成树桩
    Vec2 rootTilePos = mapLayer_->tileCoordToPosition(actualRootTile);
    Size tileSize = mapLayer_->getTileSize();
    Vec2 stumpPos = Vec2(rootTilePos.x + tileSize.width / 2.0f, rootTilePos.y);

    auto stump = Sprite::create("images/items/tree_stump.png");
    if (stump) {
      stump->setPosition(stumpPos);
      stump->setAnchorPoint(Vec2(0.5f, 0.0f));
      Size stumpSize = stump->getContentSize();
      float stumpScale = 32.0f / std::max(stumpSize.width, stumpSize.height);
      stump->setScale(stumpScale);
      mapLayer_->addChild(stump, 5);
      stump->setOpacity(0);
      stump->runAction(FadeIn::create(0.3f));
    }

    // 4. 掉落物品
    int woodCount = 3 + (rand() % 3);
    spawnItem(ItemType::Wood, stumpPos, woodCount);

    // 5. 数据记录
    for (const auto& tile : savedTiles) {
      auto it = std::find(choppedTrees_.begin(), choppedTrees_.end(), tile);
      if (it == choppedTrees_.end()) choppedTrees_.push_back(tile);
    }

    activeChops_.erase(std::remove_if(activeChops_.begin(), activeChops_.end(),
                                      [actualRootTile](const TreeChopData& c) {
                                        return c.tileCoord == actualRootTile;
                                      }),
                       activeChops_.end());

    showActionMessage("Tree chopped! Got wood!", Color3B(200, 255, 200));
  });

  auto fullSequence =
      Sequence::create(spawnAnim, DelayTime::create(0.3f), cleanup, nullptr);
  treeSprite->runAction(fullSequence);
}

void GameScene::spawnItem(ItemType type, const Vec2& position, int count) {
  // 创建掉落物精灵
  std::string itemImage;
  switch (type) {
    case ItemType::Wood:
      itemImage = "items/wood.png";  // ← 需要这张图片
      break;
    default:
      itemImage = "items/unknown.png";
      break;
  }

  auto item = Sprite::create(itemImage);
  if (!item) {
    // 如果没有图片，创建占位符
    item = Sprite::create();
    auto drawNode = DrawNode::create();
    drawNode->drawSolidCircle(Vec2::ZERO, 10, 0, 16,
                              Color4F(0.6f, 0.4f, 0.2f, 1.0f));
    item->addChild(drawNode);
  }

  item->setPosition(position);
  this->addChild(item, 50);

  // 掉落动画：向上抛起后落下
  auto jumpUp = JumpBy::create(0.5f, Vec2(0, 0), 30, 1);
  auto fadeOut = FadeOut::create(0.3f);
  auto remove = RemoveSelf::create();

  auto sequence = Sequence::create(jumpUp, fadeOut, remove, nullptr);
  item->runAction(sequence);

  // 显示数量标签
  if (count > 1) {
    auto countLabel = Label::createWithSystemFont(
        StringUtils::format("+%d", count), "Arial", 20);
    countLabel->setPosition(Vec2(position.x, position.y + 40));
    countLabel->setColor(Color3B(255, 255, 100));
    this->addChild(countLabel, 51);

    auto labelFade =
        Sequence::create(DelayTime::create(0.5f), FadeOut::create(0.5f),
                         RemoveSelf::create(), nullptr);
    countLabel->runAction(labelFade);
  }

  // 实际游戏中应将物品添加到背包系统
}

void GameScene::updateChopping(float delta) {
  // 这个函数现在主要用于清理超时的砍树任务（可选）
  // 新版本中通过点击次数触发，不需要倒计时

  // 可选：添加超时机制（如果10秒内没继续砍，自动取消）
  for (int i = static_cast<int>(activeChops_.size()) - 1; i >= 0; --i) {
    activeChops_[i].chopTimer += delta;

    // 如果超过10秒没继续砍，取消砍树
    if (activeChops_[i].chopTimer > 10.0f) {
      // 恢复瓦片
      for (const auto& tile : activeChops_[i].tiles) {
        // 这里需要恢复原始编号，可在砍树数据中保存
      }

      // 移除精灵
      if (activeChops_[i].treeSprite) {
        activeChops_[i].treeSprite->removeFromParent();
      }

      activeChops_.erase(activeChops_.begin() + i);
      showActionMessage("Chopping cancelled (timeout)", Color3B(200, 200, 200));
    }
  }
}

// ========== 树木调试显示 ==========
void GameScene::initTrees() {
  trees_.clear();
  activeChops_.clear();

  if (!mapLayer_) return;

  // 这里可以放置一些调试用的树木标记
  // 实际游戏中树木应该从地图中读取
  std::vector<Vec2> sampleTiles = {Vec2(8, 8), Vec2(10, 12), Vec2(12, 9)};

  Size tileSize = mapLayer_->getTileSize();
  float halfW = tileSize.width / 2.0f;
  float halfH = tileSize.height / 2.0f;

  for (const auto& t : sampleTiles) {
    Vec2 pos = mapLayer_->tileCoordToPosition(t);
    auto node = DrawNode::create();
    Vec2 bl(pos.x - halfW + 2, pos.y - halfH + 2);
    Vec2 tr(pos.x + halfW - 2, pos.y + halfH - 2);
    node->drawSolidRect(bl, tr, Color4F(0.2f, 0.6f, 0.2f, 0.9f));
    node->drawSolidCircle(pos, 10.0f, 0, 12, 1.0f, 1.0f,
                          Color4F(0.1f, 0.5f, 0.1f, 0.9f));
    mapLayer_->addChild(node, 15);

    trees_.push_back(Tree{t, node});
  }
}

int GameScene::findTreeIndex(const Vec2& tile) const {
  for (int i = 0; i < static_cast<int>(trees_.size()); ++i) {
    if (trees_[i].tileCoord == tile) return i;
  }
  return -1;
}

void GameScene::showActionMessage(const std::string& text, const Color3B& color)

{
  if (!actionLabel_) return;

  actionLabel_->setString(text);

  actionLabel_->setColor(color);

  actionLabel_->stopAllActions();

  actionLabel_->setOpacity(255);

  auto seq = Sequence::create(

      DelayTime::create(0.2f),

      FadeOut::create(1.0f),

      nullptr);

  actionLabel_->runAction(seq);
}

void GameScene::initToolbar()

{
  toolbarItems_ = {ItemType::Hoe,        ItemType::WateringCan,
                   ItemType::Scythe,     ItemType::Axe,
                   ItemType::Pickaxe,    ItemType::FishingRod,
                   ItemType::SeedTurnip, ItemType::SeedPotato};

  selectedItemIndex_ = 0;
  selectItemByIndex(0);
}

void GameScene::initToolbarUI() {
  if (!uiLayer_ || toolbarItems_.empty()) {
    return;
  }

  if (toolbarUI_) {
    toolbarUI_->removeFromParent();
    toolbarUI_ = nullptr;
    toolbarSlots_.clear();
    toolbarIcons_.clear();
    toolbarCounts_.clear();
    toolbarCountCache_.clear();
    toolbarSelectedCache_ = -1;
  }

  auto visibleSize = Director::getInstance()->getVisibleSize();
  auto origin = Director::getInstance()->getVisibleOrigin();

  int slotCount = static_cast<int>(toolbarItems_.size());
  float barWidth =
      slotCount * kToolbarSlotSize + (slotCount + 1) * kToolbarSlotPadding;
  float barHeight = kToolbarSlotSize + kToolbarSlotPadding * 2.0f;

  toolbarUI_ = LayerColor::create(kToolbarBarColor, barWidth, barHeight);
  toolbarUI_->setPosition(
      Vec2(origin.x + (visibleSize.width - barWidth) * 0.5f, origin.y + 8.0f));
  uiLayer_->addChild(toolbarUI_, 2);

  auto border = DrawNode::create();
  border->drawRect(Vec2(0, 0), Vec2(barWidth, barHeight),
                   Color4F(0.5f, 0.45f, 0.4f, 1.0f));
  border->setLineWidth(2);
  toolbarUI_->addChild(border, 1);

  toolbarSlots_.reserve(slotCount);
  toolbarIcons_.reserve(slotCount);
  toolbarCounts_.reserve(slotCount);
  toolbarCountCache_.assign(slotCount, -1);

  for (int i = 0; i < slotCount; ++i) {
    auto slotBg = Sprite::create();
    slotBg->setAnchorPoint(Vec2(0, 0));
    slotBg->setTextureRect(Rect(0, 0, kToolbarSlotSize, kToolbarSlotSize));
    slotBg->setColor(kToolbarSlotColor);
    slotBg->setPosition(
        Vec2(kToolbarSlotPadding + i * (kToolbarSlotSize + kToolbarSlotPadding),
             kToolbarSlotPadding));

    auto slotBorder = DrawNode::create();
    slotBorder->drawRect(Vec2(0, 0), Vec2(kToolbarSlotSize, kToolbarSlotSize),
                         Color4F(0.35f, 0.3f, 0.25f, 1.0f));
    slotBorder->setLineWidth(2);
    slotBg->addChild(slotBorder, 1);

    toolbarUI_->addChild(slotBg, 2);
    toolbarSlots_.push_back(slotBg);

    auto icon = createToolbarIcon(toolbarItems_[i]);
    if (icon) {
      icon->setPosition(Vec2(kToolbarSlotSize * 0.5f, kToolbarSlotSize * 0.5f));
      slotBg->addChild(icon, 0);
    }
    toolbarIcons_.push_back(icon);

    auto countLabel = Label::createWithSystemFont("", "Arial", 14);
    countLabel->setAnchorPoint(Vec2(1, 0));
    countLabel->setPosition(Vec2(kToolbarSlotSize - 4.0f, 4.0f));
    countLabel->setColor(Color3B::WHITE);
    slotBg->addChild(countLabel, 2);
    toolbarCounts_.push_back(countLabel);
  }

  refreshToolbarUI();
}

void GameScene::refreshToolbarUI() {
  if (toolbarSlots_.empty()) {
    return;
  }

  if (inventory_) {
    int maxSlots = std::min(static_cast<int>(toolbarItems_.size()),
                            inventory_->getSlotCount());
    for (int i = 0; i < maxSlots; ++i) {
      const auto& slot = inventory_->getSlot(i);
      ItemType newType = slot.isEmpty() ? ItemType::ITEM_NONE : slot.type;

      if (toolbarItems_[i] != newType) {
        toolbarItems_[i] = newType;

        if (i < static_cast<int>(toolbarIcons_.size()) && toolbarIcons_[i]) {
          toolbarIcons_[i]->removeFromParent();
          toolbarIcons_[i] = nullptr;
        }

        auto icon = createToolbarIcon(newType);
        if (icon && i < static_cast<int>(toolbarSlots_.size())) {
          icon->setPosition(
              Vec2(kToolbarSlotSize * 0.5f, kToolbarSlotSize * 0.5f));
          toolbarSlots_[i]->addChild(icon, 0);
        }

        if (i < static_cast<int>(toolbarIcons_.size())) {
          toolbarIcons_[i] = icon;
        }

        if (i < static_cast<int>(toolbarCountCache_.size())) {
          toolbarCountCache_[i] = -1;
        }

        if (i == selectedItemIndex_) {
          if (player_) {
            player_->setCurrentTool(newType);
          }
          if (itemLabel_) {
            std::string name = InventoryManager::getItemName(newType);
            itemLabel_->setString(StringUtils::format(
                "Current item: %s (1-8 to switch)", name.c_str()));
          }
        }
      }
    }
  }

  if (toolbarSelectedCache_ != selectedItemIndex_) {
    for (int i = 0; i < static_cast<int>(toolbarSlots_.size()); ++i) {
      bool isSelected = (i == selectedItemIndex_);
      toolbarSlots_[i]->setColor(isSelected ? kToolbarSlotSelectedColor
                                            : kToolbarSlotColor);
    }
    toolbarSelectedCache_ = selectedItemIndex_;
  }

  if (toolbarCountCache_.size() != toolbarItems_.size()) {
    toolbarCountCache_.assign(toolbarItems_.size(), -1);
  }

  for (int i = 0; i < static_cast<int>(toolbarItems_.size()); ++i) {
    if (i >= static_cast<int>(toolbarCounts_.size())) {
      break;
    }

    int count = -1;
    if (inventory_ && i < inventory_->getSlotCount()) {
      const auto& slot = inventory_->getSlot(i);
      if (!slot.isEmpty() && InventoryManager::isStackable(slot.type)) {
        count = slot.count;
      }
    }

    if (toolbarCountCache_[i] != count) {
      toolbarCountCache_[i] = count;
      if (count > 1) {
        toolbarCounts_[i]->setString(StringUtils::format("%d", count));
      } else {
        toolbarCounts_[i]->setString("");
      }
    }
  }
}

void GameScene::selectItemByIndex(int idx)  // 用来选择工具的函数
{
  // 1. 基本安全检查
  if (toolbarItems_.empty()) return;

  if (idx < 0 || idx >= static_cast<int>(toolbarItems_.size())) return;

  // 2. 更新选中的索引
  selectedItemIndex_ = idx;

  // 【关键修复】这里必须定义当前物品变量！
  // 从工具栏数组中取出当前选中的物品类型
  ItemType currentItem = toolbarItems_[selectedItemIndex_];

  // 3. 通知玩家切换工具（当前物品已定义）
  if (player_) {
    player_->setCurrentTool(currentItem);  // 传入物品类型，让角色选择对应动画
  }

  // 4. 更新界面显示
  std::string name = InventoryManager::getItemName(
      currentItem);  // 这里也可以直接用当前物品类型

  if (itemLabel_) {
    itemLabel_->setString(
        StringUtils::format("Current item: %s (1-8 to switch)", name.c_str()));
  }

  showActionMessage(StringUtils::format("Switched to %s", name.c_str()),
                    Color3B(180, 220, 255));
  refreshToolbarUI();
}

int GameScene::getCropIdForItem(ItemType type) const {
  switch (type) {
    case ItemType::SeedTurnip:
      return 0;
    case ItemType::SeedPotato:
      return 1;
    case ItemType::SeedCorn:
      return 2;
    case ItemType::SeedTomato:
      return 3;
    case ItemType::SeedPumpkin:
      return 4;
    case ItemType::SeedBlueberry:
      return 5;
    default:
      return -1;
  }
}

ItemType GameScene::getItemTypeForCropId(int cropId) const {
  switch (cropId) {
    case 0:
      return ItemType::Turnip;
    case 1:
      return ItemType::Potato;
    case 2:
      return ItemType::Corn;
    case 3:
      return ItemType::Tomato;
    case 4:
      return ItemType::Pumpkin;
    case 5:
      return ItemType::Blueberry;
    default:
      return ItemType::ITEM_NONE;
  }
}

void GameScene::onMouseDown(Event* event) {
  if (dialogueBox_ && dialogueBox_->isVisible()) return;

  auto* e = static_cast<EventMouse*>(event);
  if (e->getMouseButton() == EventMouse::MouseButton::BUTTON_LEFT) {
    auto camera = getDefaultCamera();
    Vec2 camPos = Vec2(camera->getPositionX(), camera->getPositionY());
    Size visibleSize = Director::getInstance()->getVisibleSize();

    Vec2 mouseLoc = e->getLocationInView();
    mouseLoc.y = visibleSize.height - mouseLoc.y;

    Vec2 worldPos =
        camPos - Vec2(visibleSize.width / 2, visibleSize.height / 2) + mouseLoc;

    for (auto npc : npcs_) {
      Vec2 mapLayerClickPos = mapLayer_->convertToNodeSpace(worldPos);

      if (npc->getBoundingBox().containsPoint(mapLayerClickPos)) {
        if (npc->getNpcName() == "Blacksmith") {
          auto ui = BlacksmithUI::create();
          ui->show();
          uiLayer_->addChild(ui, 200);
          return;
        }

        if (dialogueBox_) {
          dialogueBox_->setNpc(npc);
          dialogueBox_->showDialogue();
        }
        return;
      }
    }

    if (toolbarItems_.empty()) return;

    ItemType current = ItemType::Hoe;
    if (selectedItemIndex_ >= 0 &&
        selectedItemIndex_ < static_cast<int>(toolbarItems_.size())) {
      current = toolbarItems_[selectedItemIndex_];
    }

    if (current == ItemType::FishingRod) {
      if (fishingState_ == FishingState::NONE) {
        if (isFishing_) return;
        if (player_ && player_->isFishingAnimationPlaying()) return;
        fishingState_ = FishingState::CHARGING;
        fishingRodSlotIndex_ = selectedItemIndex_;
        chargePower_ = 0.0f;
      } else if (fishingState_ == FishingState::BITING) {
        if (exclamationMark_) exclamationMark_->setVisible(false);
        fishingRodSlotIndex_ = selectedItemIndex_;
        startFishing();
      } else if (fishingState_ == FishingState::WAITING) {
        fishingState_ = FishingState::NONE;
        fishingRodSlotIndex_ = -1;
        if (exclamationMark_) exclamationMark_->setVisible(false);
        if (player_) player_->startFishingReel();
        showActionMessage("Too early!", Color3B::RED);
      }
      return;
    }

    Vec2 clickPos = e->getLocationInView();
    clickPos.y = Director::getInstance()->getWinSize().height - clickPos.y;
    Vec3 cameraPos = this->getDefaultCamera()->getPosition3D();
    Vec2 worldPos2 = clickPos + Vec2(cameraPos.x, cameraPos.y) -
                     Director::getInstance()->getVisibleSize() / 2;

    if (mapLayer_) {
      Vec2 t = mapLayer_->positionToTileCoord(worldPos2);
    }
  }
}

void GameScene::onMouseUp(Event* event) {
  auto* e = static_cast<EventMouse*>(event);
  if (e->getMouseButton() == EventMouse::MouseButton::BUTTON_LEFT) {
    if (fishingState_ == FishingState::CHARGING) {
      fishingState_ = FishingState::WAITING;
      if (chargeBarBg_) chargeBarBg_->setVisible(false);

      waitTimer_ = CCRANDOM_0_1() * 3.0f + 1.0f;
      if (player_) player_->startFishingCast();
    }
  }
}

void GameScene::updateFishingState(float delta) {
  if (fishingState_ == FishingState::CHARGING) {
    // 钓鱼技能提高充能速度 (可选，或者影响条的大小)
    chargePower_ += delta * 1.5f;
    if (chargePower_ > 1.0f) chargePower_ = 1.0f;

    if (chargeBarBg_) chargeBarBg_->setVisible(true);
    if (chargeBarFg_) {
      chargeBarFg_->setTextureRect(Rect(0, 0, 50 * chargePower_, 8));
      chargeBarFg_->setColor(
          Color3B(255 * chargePower_, 255 * (1 - chargePower_) + 100, 0));
    }
  } else if (fishingState_ == FishingState::WAITING) {
    waitTimer_ -= delta;
    if (waitTimer_ <= 0) {
      fishingState_ = FishingState::BITING;
      biteTimer_ = 1.0f;
      if (exclamationMark_) exclamationMark_->setVisible(true);
    }
  } else if (fishingState_ == FishingState::BITING) {
    biteTimer_ -= delta;
    if (biteTimer_ <= 0) {
      fishingState_ = FishingState::NONE;
      fishingRodSlotIndex_ = -1;
      if (exclamationMark_) exclamationMark_->setVisible(false);
      if (player_) player_->startFishingReel();
      showActionMessage("Missed...", Color3B::GRAY);
    }
  }
}

void GameScene::startFishing() {
  isFishing_ = true;
  fishingState_ = FishingState::REELING;

  if (player_) {
    player_->setMoveSpeed(0);
    player_->startFishingWait();
  }

  int rodSlotIndex = fishingRodSlotIndex_;
  fishingRodSlotIndex_ = -1;

  // 1. 根据环境随机生成一条鱼（淡水鱼）
  std::vector<ItemType> freshFish = {
      ItemType::ITEM_Carp, ItemType::ITEM_Largemouth_Bass,
      ItemType::ITEM_Rainbow_Trout, ItemType::ITEM_Eel};

  const int idx = cocos2d::random(0, static_cast<int>(freshFish.size()) - 1);
  ItemType typeToCatch = freshFish[idx];
  Fish* fishObj = Fish::createByType(typeToCatch);

  // 2. 创建钓鱼图层
  auto fishingLayer = FishingLayer::create(fishObj);
  fishingLayer->setFinishCallback([this, fishObj, rodSlotIndex](bool success) {
    auto finish = [this, success, fishObj, rodSlotIndex]() {
      this->isFishing_ = false;
      this->fishingState_ = FishingState::NONE;

      if (this->chargeBarBg_) this->chargeBarBg_->setVisible(false);
      if (this->exclamationMark_) this->exclamationMark_->setVisible(false);
      if (this->player_) this->player_->setMoveSpeed(150.0f);

      if (success) {
        ItemType type = fishObj ? fishObj->getType() : ItemType::Fish;
        std::string name = fishObj ? fishObj->getName() : "Fish";

        showActionMessage("Caught a " + name + "!", Color3B(255, 215, 0));

        // 加入背包
        auto inv = InventoryManager::getInstance();
        if (inv) {
          inv->addItem(type, 1);
        }

        SkillManager::getInstance()->recordAction(
            SkillManager::SkillType::Fishing);
      } else {
        showActionMessage("Fish got away...", Color3B::RED);
      }

      if (this->inventory_ && rodSlotIndex >= 0 &&
          rodSlotIndex < this->inventory_->getSlotCount()) {
        const auto& rodSlot = this->inventory_->getSlot(rodSlotIndex);
        if (rodSlot.type == ItemType::FishingRod) {
          if (this->inventory_->decreaseDurability(rodSlotIndex, 1)) {
            showActionMessage("Fishing Rod broke!", Color3B::RED);
            refreshToolbarUI();
          }
          if (inventoryUI_) inventoryUI_->refresh();
        }
      }

      if (fishObj) delete fishObj;
    };

    if (this->player_) {
      this->player_->startFishingReel(finish);
    } else {
      finish();
    }
  });

  this->addChild(fishingLayer, 100);
}
// ========== 返回菜单 ==========

void GameScene::backToMenu()

{
  saveGame();

  auto scene = MenuScene::createScene();

  Director::getInstance()->replaceScene(TransitionFade::create(1.0f, scene));
}

// ========== 背包系统相关 ==========

void GameScene::toggleInventory() {
  if (!inventory_) {
    return;
  }

  // 如果背包已经打开，关闭它
  if (inventoryUI_) {
    inventoryUI_->close();
    return;
  }

  // 创建背包界面
  inventoryUI_ = InventoryUI::create(inventory_, &marketState_);
  if (!inventoryUI_) {
    return;
  }

  // 对于玩家自己的背包，当它打开时，如果玩家站在箱子旁边，我们可以设置合作伙伴
  // 这样玩家就能把东西存进箱子。
  if (farmManager_) {
    Vec2 tileCoord = mapLayer_->positionToTileCoord(player_->getPosition());
    tileCoord.x = std::round(tileCoord.x);
    tileCoord.y = std::round(tileCoord.y);

    // 检查周围 1 格的箱子
    StorageChest* nearbyChest = nullptr;
    for (float dy = -1; dy <= 1; ++dy) {
      for (float dx = -1; dx <= 1; ++dx) {
        nearbyChest = farmManager_->getStorageChestAt(tileCoord + Vec2(dx, dy));
        if (nearbyChest) break;
      }
      if (nearbyChest) break;
    }

    if (nearbyChest) {
      inventoryUI_->setPartnerInventory(nearbyChest->getInventory(),
                                        nearbyChest->isShippingBin());
    }
  }

  inventoryUI_->setCloseCallback([this]() { onInventoryClosed(); });

  if (uiLayer_) {
    uiLayer_->addChild(inventoryUI_, 2000);
    inventoryUI_->setPosition(Vec2::ZERO);
  } else {
    this->addChild(inventoryUI_, 2000);
  }

  inventoryUI_->show();
}

void GameScene::toggleSkillTree() {
  if (skillUI_) {
    skillUI_->close();
    skillUI_ = nullptr;
    return;
  }

  skillUI_ = SkillTreeUI::create();
  if (!skillUI_) {
    return;
  }

  if (uiLayer_) {
    uiLayer_->addChild(skillUI_, 2000);
    skillUI_->setPosition(Vec2::ZERO);
  } else {
    this->addChild(skillUI_, 2000);
  }
  skillUI_->show();
}

void GameScene::onInventoryClosed() { inventoryUI_ = nullptr; }

void GameScene::toggleMarket() {
  if (!inventory_) {
    return;
  }

  if (marketUI_) {
    marketUI_->close();
    return;
  }

  marketUI_ = MarketUI::create(inventory_, &marketState_, farmManager_);
  if (!marketUI_) {
    return;
  }

  marketUI_->setCloseCallback([this]() { onMarketClosed(); });

  this->addChild(marketUI_, 2200);
  marketUI_->setGlobalZOrder(2200);

  auto camera = this->getDefaultCamera();
  if (camera) {
    Vec3 cameraPos = camera->getPosition3D();
    auto visibleSize = Director::getInstance()->getVisibleSize();
    auto origin = Director::getInstance()->getVisibleOrigin();
    Vec2 uiPos = Vec2(cameraPos.x - visibleSize.width / 2,
                      cameraPos.y - visibleSize.height / 2);
    marketUI_->setPosition(uiPos);
  }

  marketUI_->show();
}

void GameScene::onMarketClosed() { marketUI_ = nullptr; }

void GameScene::enterHouse() {
  if (!isPlayerNearHouseDoor()) {
    showActionMessage("Door is too far!", Color3B::RED);
    return;
  }

  auto houseScene = HouseScene::createScene();
  if (houseScene) {
    houseScene->setFarmManager(farmManager_);
  } else {
    return;
  }

  auto transition = TransitionFade::create(0.4f, houseScene);
  Director::getInstance()->pushScene(transition);
}

void GameScene::enterBarn() {
  auto barnScene = BarnScene::createScene();
  if (!barnScene) {
    return;
  }

  auto transition = TransitionFade::create(0.4f, barnScene);
  Director::getInstance()->pushScene(transition);
}

void GameScene::enterBeach() {
  if (enteringBeach_) return;

  enteringBeach_ = true;

  SaveManager::SaveData data = collectSaveData();
  if (SaveManager::getInstance()->saveGame(data)) {
  } else {
  }

  int dayCount = farmManager_ ? farmManager_->getDayCount() : 1;
  float accumulatedSeconds = 0.0f;
  if (farmManager_) {
    accumulatedSeconds = farmManager_->getDayProgress() * 120.0f;
  }

  auto beachScene =
      BeachScene::createScene(inventory_, dayCount, accumulatedSeconds);
  if (beachScene) {
    auto transition = TransitionFade::create(0.5f, beachScene);
    Director::getInstance()->replaceScene(transition);
  } else {
    enteringBeach_ = false;
  }
}

void GameScene::checkBeachEntrance() {
  if (enteringBeach_ || !player_ || !mapLayer_) return;

  if (inventoryUI_ || marketUI_ || skillUI_) return;

  if (isPlayerAtBeachEntrance()) {
    enterBeach();
  }
}

bool GameScene::isPlayerAtBeachEntrance() const {
  if (!player_ || !mapLayer_) return false;

  Vec2 tileCoord = mapLayer_->positionToTileCoord(player_->getPosition());
  Size mapSize = mapLayer_->getMapSizeInTiles();

  const float entranceWidth = 4.0f;
  const float entranceHeight = 6.0f;

  return tileCoord.x >= mapSize.width - entranceWidth &&
         tileCoord.y <= entranceHeight;
}

void GameScene::enterMine() {
  // 创建电梯楼层选择界面
  auto elevatorUI = ElevatorUI::create();
  if (!elevatorUI) {
    return;
  }

  // 设置楼层选择回调
  elevatorUI->setFloorSelectCallback([this](int floor) {
    if (floor == 0) {
      // 楼层0是农场（地面），不需要切换场景
      showActionMessage("Already on the farm!", Color3B(200, 200, 200));
      return;
    }

    // 进入矿洞前自动保存游戏
    SaveManager::SaveData data = collectSaveData();
    if (SaveManager::getInstance()->saveGame(data)) {
    } else {
    }

    // 创建矿洞场景，传入背包实例和选择的楼层
    auto mineScene = MineScene::createScene(inventory_, floor);

    if (mineScene) {
      auto transition = TransitionFade::create(0.5f, mineScene);
      Director::getInstance()->replaceScene(transition);
    } else {
    }
  });

  // 添加到界面层（界面层会随摄像机移动，直接添加即可）
  if (uiLayer_) {
    elevatorUI->setPosition(Vec2::ZERO);
    uiLayer_->addChild(elevatorUI, 2000);
  } else {
    // 回退逻辑
    this->addChild(elevatorUI, 2500);
    auto camera = this->getDefaultCamera();
    if (camera) {
      Vec3 cameraPos = camera->getPosition3D();
      auto visibleSize = Director::getInstance()->getVisibleSize();
      auto origin = Director::getInstance()->getVisibleOrigin();
      Vec2 uiPos = Vec2(cameraPos.x - visibleSize.width / 2,
                        cameraPos.y - visibleSize.height / 2);
      elevatorUI->setPosition(uiPos);
    }
  }

  elevatorUI->show();
}

bool GameScene::isPlayerNearElevator() const {
  if (!player_) return false;

  // 检查距离
  float distance = player_->getPosition().distance(ELEVATOR_POS);
  // 允许100像素误差
  return distance < 100.0f;
}

bool GameScene::isPlayerNearHouseDoor() const {
  if (!player_ || !mapLayer_) return false;

  Vec2 doorPos = mapLayer_->tileCoordToPosition(kHouseDoorTile);
  return player_->getPosition().distance(doorPos) <= kHouseDoorRadius;
}

bool GameScene::isPlayerNearBarnDoor() const {
  if (!player_ || !mapLayer_) return false;

  Vec2 doorPos = mapLayer_->tileCoordToPosition(kBarnDoorTile);
  return player_->getPosition().distance(doorPos) <= kBarnDoorRadius;
}

// ==========================================
// 存档系统实现
// ==========================================

Scene* GameScene::createScene(bool loadFromSave) {
  auto scene = GameScene::create();
  if (scene && loadFromSave) {
    // 通过带参数的初始化加载存档
    // 但创建时已调用初始化，需要手动加载
    GameScene* gameScene = dynamic_cast<GameScene*>(scene);
    if (gameScene) {
      gameScene->loadGame();
    }
  }
  return scene;
}

bool GameScene::init(bool loadFromSave) {
  if (!init()) return false;

  if (loadFromSave) {
    loadGame();
  }

  return true;
}

void GameScene::saveGame() {
  SaveManager::SaveData data = collectSaveData();

  if (SaveManager::getInstance()->saveGame(data)) {
    // 显示保存成功提示
    showActionMessage("Game Saved!", Color3B::GREEN);
  } else {
    showActionMessage("Save Failed!", Color3B::RED);
  }
}

void GameScene::loadGame() {
  SaveManager::SaveData data;

  if (SaveManager::getInstance()->loadGame(data)) {
    applySaveData(data);

    // 延迟显示消息，确保界面已初始化
    if (actionLabel_) {
      showActionMessage("Game Loaded!", Color3B::GREEN);
    }
  } else {
    if (actionLabel_) {
      showActionMessage("Load Failed!", Color3B::RED);
    }
  }
}

SaveManager::SaveData GameScene::collectSaveData() {
  SaveManager::SaveData data;

  // 保存玩家位置
  if (player_) {
    data.playerPosition = player_->getPosition();
  }

  // 保存背包数据
  if (inventory_) {
    data.inventory.money = inventory_->getMoney();

    const auto& slots = inventory_->getAllSlots();
    for (size_t i = 0; i < slots.size(); i++) {
      const auto& slot = slots[i];
      if (!slot.isEmpty()) {
        SaveManager::SaveData::InventoryData::ItemSlotData slotData;
        slotData.type = static_cast<int>(slot.type);
        slotData.count = slot.count;
        slotData.durability = slot.durability;
        slotData.maxDurability = slot.maxDurability;
        data.inventory.slots.push_back(slotData);
      } else {
        // 空槽位也要保存，保持索引一致
        SaveManager::SaveData::InventoryData::ItemSlotData slotData;
        slotData.type = static_cast<int>(ItemType::ITEM_NONE);
        slotData.count = 0;
        slotData.durability = -1;
        slotData.maxDurability = -1;
        data.inventory.slots.push_back(slotData);
      }
    }
  }

  // 保存游戏天数
  if (farmManager_) {
    data.dayCount = farmManager_->getDayCount();
  }

  // 保存农作物数据
  if (farmManager_) {
    const auto& farmTiles = farmManager_->getAllTiles();
    const Size mapSize = farmManager_->getMapSize();
    const int width = static_cast<int>(mapSize.width);
    const int height = static_cast<int>(mapSize.height);

    for (int y = 0; y < height; y++) {
      for (int x = 0; x < width; x++) {
        const int index = y * width + x;
        if (index < 0 || static_cast<size_t>(index) >= farmTiles.size()) {
          continue;
        }

        const auto& tile = farmTiles[index];

        // 只保存有状态的瓦片
        if (tile.tilled || tile.hasCrop) {
          SaveManager::SaveData::FarmTileData tileData;
          tileData.x = x;
          tileData.y = y;
          tileData.tilled = tile.tilled;
          tileData.watered = tile.watered;
          tileData.hasCrop = tile.hasCrop;
          tileData.cropId = tile.cropId;
          tileData.stage = tile.stage;
          tileData.progressDays = tile.progressDays;
          data.farmTiles.push_back(tileData);
        }
      }
    }

    // 保存储物箱数据
    const auto& chests = farmManager_->getStorageChests();
    for (auto chest : chests) {
      SaveManager::SaveData::StorageChestData chestData;
      chestData.x = static_cast<int>(chest->getTileCoord().x);
      chestData.y = static_cast<int>(chest->getTileCoord().y);

      const auto& slots = chest->getInventory()->getAllSlots();
      for (const auto& slot : slots) {
        SaveManager::SaveData::StorageChestData::SlotData slotData;
        slotData.type = static_cast<int>(slot.type);
        slotData.count = slot.count;
        slotData.durability = slot.durability;
        slotData.maxDurability = slot.maxDurability;
        chestData.slots.push_back(slotData);
      }
      data.storageChests.push_back(chestData);
    }
  }

  // 树木存档功能已禁用（避免崩溃问题）
  // 注意：砍倒的树木在重新加载游戏后会恢复

  // 树木存档功能已禁用（避免崩溃）
  // 注意：砍倒的树木在重新加载游戏后会恢复

  // 保存技能数据
  if (auto skillMgr = SkillManager::getInstance()) {
    for (int i = 0; i < static_cast<int>(SkillManager::SkillType::Count); ++i) {
      const auto type = static_cast<SkillManager::SkillType>(i);
      const auto& sd = skillMgr->getSkillData(type);

      SaveManager::SaveData::SkillData skillSave;
      skillSave.type = static_cast<int>(type);
      skillSave.level = sd.level;
      skillSave.actionCount = sd.actionCount;
      data.skills.push_back(skillSave);
    }
  }

  return data;
}

void GameScene::applySaveData(const SaveManager::SaveData& data) {
  // 恢复玩家位置
  if (player_) {
    player_->setPosition(data.playerPosition);
  } else {
  }

  // 恢复背包数据
  if (inventory_) {
    // 清空现有背包
    inventory_->clear();

    // 恢复金币，使用一次性增加
    if (data.inventory.money > 0) {
      inventory_->addMoney(data.inventory.money);
    }

    // 恢复物品槽位
    const size_t slotCount = static_cast<size_t>(inventory_->getSlotCount());
    for (size_t i = 0; i < data.inventory.slots.size() && i < slotCount; i++) {
      const auto& slotData = data.inventory.slots[i];
      if (slotData.type != static_cast<int>(ItemType::ITEM_NONE) &&
          slotData.count > 0) {
        ItemType type = static_cast<ItemType>(slotData.type);
        inventory_->setSlotData(i, type, slotData.count, slotData.durability,
                                slotData.maxDurability);
      }
    }
  } else {
  }

  // 恢复游戏天数
  if (farmManager_) {
    farmManager_->setDayCount(data.dayCount);
  } else {
  }

  // 恢复农作物数据
  if (farmManager_) {
    // 获取当前地图尺寸
    const Size mapSize = farmManager_->getMapSize();
    const int width = static_cast<int>(mapSize.width);
    const int height = static_cast<int>(mapSize.height);
    std::vector<FarmManager::FarmTile> tiles(width * height);

    // 应用保存的瓦片数据
    for (const auto& tileData : data.farmTiles) {
      const int index = tileData.y * width + tileData.x;
      if (index >= 0 && static_cast<size_t>(index) < tiles.size()) {
        tiles[index].tilled = tileData.tilled;
        tiles[index].watered = tileData.watered;
        tiles[index].hasCrop = tileData.hasCrop;
        tiles[index].cropId = tileData.cropId;
        tiles[index].stage = tileData.stage;
        tiles[index].progressDays = tileData.progressDays;
      }
    }

    farmManager_->setAllTiles(tiles);

    // 恢复储物箱数据
    for (const auto& chestData : data.storageChests) {
      auto chest = StorageChest::create(Vec2(chestData.x, chestData.y));
      if (chest) {
        // 恢复箱子里的物品
        const size_t chestSlotCount =
            static_cast<size_t>(chest->getInventory()->getSlotCount());
        for (size_t i = 0; i < chestData.slots.size() && i < chestSlotCount;
             ++i) {
          const auto& slot = chestData.slots[i];
          if (slot.type != static_cast<int>(ItemType::ITEM_NONE) &&
              slot.count > 0) {
            chest->getInventory()->setSlotData(
                i, static_cast<ItemType>(slot.type), slot.count,
                slot.durability, slot.maxDurability);
          }
        }
        farmManager_->addStorageChest(chest);
      }
    }
  }

  // 树木恢复功能已禁用（避免崩溃问题）
  // 注意：砍倒的树木在重新加载游戏后会恢复
  choppedTrees_.clear();

  // 恢复技能数据
  if (auto skillMgr = SkillManager::getInstance()) {
    for (const auto& skillData : data.skills) {
      skillMgr->setSkillData(
          static_cast<SkillManager::SkillType>(skillData.type), skillData.level,
          skillData.actionCount);
    }
  }
}

// ==========================================
// ==========================================

void GameScene::startMerchantInteraction(Npc* npc) {
  if (!npc || !npc->isMerchant()) return;

  activeNpc_ = npc;
  merchantState_ = MerchantState::Greeting;

  if (dialogueBox_) {
    dialogueBox_->setNpc(npc);
    dialogueBox_->setVisible(true);
    dialogueBox_->showDialogue("Welcome! What can I do for you?");

    dialogueBox_->setOnClickCallback(
        [this]() { this->advanceMerchantDialogue(); });
  }
}

void GameScene::advanceMerchantDialogue() {
  if (merchantState_ == MerchantState::Greeting) {
    merchantState_ = MerchantState::Choice;
    if (dialogueBox_) {
      dialogueBox_->showDialogue("Would you like to buy or sell?");
      dialogueBox_->showChoices("Buy", "Sell", [this](int choice) {
        this->onMerchantChoice(choice);
      });
    }
  } else if (merchantState_ == MerchantState::End) {
    endMerchantInteraction();
  } else if (merchantState_ == MerchantState::BuyTransition) {
    merchantState_ = MerchantState::Buy;
    if (dialogueBox_) dialogueBox_->setVisible(false);
    toggleMarket();
  } else if (merchantState_ == MerchantState::SellPre) {
    merchantState_ = MerchantState::Sell;

    if (dialogueBox_) {
      dialogueBox_->hideChoices();
      dialogueBox_->closeDialogue();
    }

    if (!inventoryUI_) {
      if (inventory_) {
        inventoryUI_ = InventoryUI::create(inventory_, &marketState_);
        if (inventoryUI_) {
          this->addChild(inventoryUI_, 1100);
          inventoryUI_->setGlobalZOrder(1100);
          auto camera = this->getDefaultCamera();
          if (camera) {
            Vec3 cameraPos = camera->getPosition3D();
            auto visibleSize = Director::getInstance()->getVisibleSize();
            Vec2 uiPos = Vec2(cameraPos.x - visibleSize.width / 2,
                              cameraPos.y - visibleSize.height / 2);
            inventoryUI_->setPosition(uiPos);
          }
        }
      }
    }

    if (inventoryUI_) {
      inventoryUI_->show();
      inventoryUI_->setSelectionMode(true);
      inventoryUI_->setOnItemSelectedCallback(
          [this](int slotIndex, ItemType type, int count) {
            this->handleSellSelection(slotIndex, type, count);
          });

      showActionMessage("Select an item to sell", Color3B::YELLOW);
    }
  }
}

void GameScene::onMerchantChoice(int choice) {
  if (merchantState_ != MerchantState::Choice) return;

  if (choice == 0) {
    merchantState_ = MerchantState::BuyTransition;
    if (dialogueBox_) {
      dialogueBox_->hideChoices();
      dialogueBox_->showDialogue(
          "We just got a fresh shipment in. Take a look!");
    }
  } else {
    merchantState_ = MerchantState::SellPre;
    if (dialogueBox_) {
      dialogueBox_->hideChoices();
      dialogueBox_->showDialogue("Let me see what fine goods you've brought.");
    }
  }
}

void GameScene::handleSellSelection(int slotIndex, ItemType type, int count) {
  if (merchantState_ != MerchantState::Sell &&
      merchantState_ != MerchantState::SellPre)
    return;

  pendingSellItem_ = type;
  pendingSellCount_ = count;

  if (count > 1) {
    auto popup = QuantityPopup::create(
        count, [this](int qty) { this->onQuantityConfirmed(qty); });

    this->addChild(popup, 3000);
    popup->setGlobalZOrder(3000);

    auto camera = this->getDefaultCamera();
    if (camera) {
      Vec3 camPos = camera->getPosition3D();
      auto visibleSize = Director::getInstance()->getVisibleSize();
      popup->setPosition(Vec2(camPos.x - visibleSize.width / 2,
                              camPos.y - visibleSize.height / 2));
    }
  } else {
    onQuantityConfirmed(1);
  }
}

void GameScene::onQuantityConfirmed(int qty) {
  if (qty <= 0) return;

  if (inventoryUI_) {
    inventoryUI_->close();
    inventoryUI_ = nullptr;
  }

  MarketState marketState;
  int unitPrice = marketState.getSellPrice(pendingSellItem_);

  if (unitPrice <= 0) {
    if (dialogueBox_) {
      dialogueBox_->setVisible(true);
      dialogueBox_->showDialogue("I cannot buy that.");

      merchantState_ = MerchantState::End;
      dialogueBox_->setOnClickCallback(
          [this]() { this->endMerchantInteraction(); });
    }
    return;
  }

  int totalPrice = unitPrice * qty;

  std::string itemName = InventoryManager::getItemName(pendingSellItem_);
  std::string text = StringUtils::format("I'll take %d %s for %d G. Deal?", qty,
                                         itemName.c_str(), totalPrice);

  merchantState_ = MerchantState::TradeConfirm;
  pendingSellCount_ = qty;

  if (dialogueBox_) {
    dialogueBox_->setVisible(true);
    dialogueBox_->showDialogue(text);
    dialogueBox_->showChoices("Yes", "No", [this](int choice) {
      this->onTradeConfirmResult(choice == 0);
    });
  }
}

void GameScene::onTradeConfirmResult(bool confirmed) {
  if (!confirmed) {
    if (dialogueBox_) dialogueBox_->showDialogue("Maybe next time.");
    merchantState_ = MerchantState::End;
  } else {
    if (inventory_) {
      int currentMoney = inventory_->getMoney();
      MarketState marketState;
      int unitPrice = marketState.getSellPrice(pendingSellItem_);
      int total = unitPrice * pendingSellCount_;

      inventory_->addMoney(total);
      inventory_->removeItem(pendingSellItem_, pendingSellCount_);

      if (inventoryUI_) inventoryUI_->refresh();
    }

    if (dialogueBox_)
      dialogueBox_->showDialogue("Thank you! Here is your gold.");
    merchantState_ = MerchantState::End;
  }

  if (dialogueBox_) {
    dialogueBox_->hideChoices();
    dialogueBox_->setOnClickCallback(
        [this]() { this->endMerchantInteraction(); });
  }
}

void GameScene::endMerchantInteraction() {
  activeNpc_ = nullptr;
  merchantState_ = MerchantState::None;

  if (dialogueBox_) {
    dialogueBox_->closeDialogue();
    dialogueBox_->hideChoices();
    dialogueBox_->setOnClickCallback(nullptr);
  }

  if (inventoryUI_) {
    inventoryUI_->setSelectionMode(false);
    inventoryUI_->close();
  }
}
