#include "HouseScene.h"

#include "DialogueBox.h"
#include "FarmManager.h"
#include "GameScene.h"
#include "InventoryManager.h"
#include "InventoryUI.h"
#include "MarketState.h"
#include "Player.h"
#include "TimeManager.h"

USING_NS_CC;

HouseScene* HouseScene::createScene(bool isPassedOut) {
  HouseScene* ret = new (std::nothrow) HouseScene();
  if (ret && ret->init(isPassedOut)) {
    ret->autorelease();
    return ret;
  }
  CC_SAFE_DELETE(ret);
  return nullptr;
}

bool HouseScene::init() { return init(false); }

bool HouseScene::init(bool isPassedOut) {
  if (!Scene::init()) return false;

  isPassedOut_ = isPassedOut;

  initBackground();
  initPlayer();
  initControls();
  initUI();
  this->scheduleUpdate();

  // 初始化背包引用
  inventory_ = InventoryManager::getInstance();

  if (isPassedOut_) {
    // 如果是晕倒模式，直接开始睡觉流程
    isSleeping_ = true;
    passedMidnightDuringSleep_ = false;
    if (player_ && background_) {
      player_->setVisible(false);
      player_->disableKeyboardControl();
      // 设为床的位置 (本地坐标 165, 77)
      player_->setPosition(background_->convertToWorldSpace(Vec2(165, 77)));
    }
    if (sleepSprite_) sleepSprite_->setVisible(true);
  } else {
    // 正常进入，生成在门口 (本地坐标 79, 25 左右)
    if (player_ && background_) {
      player_->setPosition(background_->convertToWorldSpace(Vec2(79, 25)));
    }
    if (sleepSprite_) sleepSprite_->setVisible(false);
  }

  return true;
}

void HouseScene::initUI() {
  auto visibleSize = Director::getInstance()->getVisibleSize();
  Vec2 origin = Director::getInstance()->getVisibleOrigin();

  timeLabel_ = Label::createWithSystemFont("Time: --:--", "Arial", 24);
  timeLabel_->setPosition(Vec2(origin.x + visibleSize.width * 0.5f,
                               origin.y + visibleSize.height - 30));
  timeLabel_->setColor(Color3B::WHITE);
  this->addChild(timeLabel_, 10);

  // 初始化对话框
  dialogueBox_ = DialogueBox::create(nullptr);
  if (dialogueBox_) {
    this->addChild(dialogueBox_, 100);
    dialogueBox_->setVisible(false);
  }
}

void HouseScene::updateUI() {
  auto tm = TimeManager::getInstance();
  if (tm && timeLabel_) {
    std::string timeStr = StringUtils::format("Day %d, %02d:%02d", tm->getDay(),
                                              tm->getHour(), tm->getMinute());

    if (isSleeping_) {
      timeLabel_->setString(timeStr + " (Sleeping...)");
      timeLabel_->setColor(Color3B::YELLOW);
    } else {
      timeLabel_->setString(timeStr);
      timeLabel_->setColor(Color3B::WHITE);
    }
  }
}

// 【新增】起床核心逻辑
void HouseScene::wakeUp() {
  if (!isSleeping_) return;

  // 1. 标记状态为清醒
  isSleeping_ = false;
  passedMidnightDuringSleep_ = false;

  // 2. 玩家角色：显示、恢复控制、回满体力
  if (player_) {
    player_->setVisible(true);         // ★ 显示站立的玩家
    player_->enableKeyboardControl();  // 恢复键盘
    player_->recoverEnergy(270.0f);    // 回满体力
  }

  // 3. 睡觉贴图：隐藏
  if (sleepSprite_) {
    sleepSprite_->setVisible(false);  // ★ 去除睡觉图
  }

  TimeManager::getInstance()->advanceToNextDay();
}

void HouseScene::toggleInventory() {
  if (inventoryUI_) {
    inventoryUI_->close();
    return;
  }

  if (!inventory_) return;

  inventoryUI_ = InventoryUI::create(inventory_);
  if (inventoryUI_) {
    inventoryUI_->setCloseCallback(
        CC_CALLBACK_0(HouseScene::onInventoryClosed, this));
    this->addChild(inventoryUI_, 100);
    inventoryUI_->show();
  }
}

void HouseScene::onInventoryClosed() { inventoryUI_ = nullptr; }

void HouseScene::showWeatherForecast() {
  if (!dialogueBox_) return;

  auto tm = TimeManager::getInstance();
  int currentDay = tm->getDay();

  std::string forecast = u8"未来一周天气预报：\n";
  for (int i = 1; i <= 7; ++i) {
    int targetDay = currentDay + i;
    MarketState::Weather w = MarketState::predictWeather(targetDay);
    std::string wName = MarketState::getWeatherName(w);

    // 翻译
    std::string cnName;
    if (wName == "Sunny")
      cnName = u8"晴天";
    else if (wName == "Light Rain")
      cnName = u8"小雨";
    else if (wName == "Heavy Rain")
      cnName = u8"大雨";
    else if (wName == "Snowy")
      cnName = u8"下雪";
    else
      cnName = wName;

    forecast += StringUtils::format("Day %d: %s\n", targetDay, cnName.c_str());
  }

  dialogueBox_->showDialogue(forecast);
  dialogueBox_->setOnClickCallback([this]() { dialogueBox_->closeDialogue(); });
}

void HouseScene::initBackground() {
  auto visibleSize = Director::getInstance()->getVisibleSize();
  Vec2 origin = Director::getInstance()->getVisibleOrigin();

  background_ = Sprite::create("myhouse/Myhouse.png");
  if (!background_) return;

  float scaleX = visibleSize.width / background_->getContentSize().width;
  float scaleY = visibleSize.height / background_->getContentSize().height;
  float scale = MIN(scaleX, scaleY);

  background_->setPosition(Vec2(origin.x + visibleSize.width / 2,
                                origin.y + visibleSize.height / 2));
  background_->setScale(scale);
  this->addChild(background_, 0);

  sleepSprite_ = Sprite::create("myhouse/sleeping.png");
  if (sleepSprite_) {
    // 1. 设置中心点位置
    sleepSprite_->setPosition(Vec2(180.5f, 77.5f));

    // 2. 可选：如果图片尺寸不是 53乘55，可以强制缩放适配这个框
    float targetW = 53.0f;
    float targetH = 55.0f;
    sleepSprite_->setScaleX(targetW / sleepSprite_->getContentSize().width);
    sleepSprite_->setScaleY(targetH / sleepSprite_->getContentSize().height);

    // 3. 初始隐藏
    sleepSprite_->setVisible(false);

    // 4. 加到背景上
    background_->addChild(sleepSprite_, 10);
  }
}

void HouseScene::initPlayer() {
  auto visibleSize = Director::getInstance()->getVisibleSize();
  Vec2 origin = Director::getInstance()->getVisibleOrigin();

  player_ = Player::create();
  if (!player_) return;

  player_->setScale(0.9f);

  player_->setPosition(Vec2(origin.x + visibleSize.width * 0.5f,
                            origin.y + visibleSize.height * 0.2f));
  player_->enableKeyboardControl();
  this->addChild(player_, 1);
}

void HouseScene::update(float delta) {
  if (!player_ || !background_) return;

  auto tm = TimeManager::getInstance();
  if (tm) {
    float speedMultiplier = isSleeping_ ? 40.0f : 1.0f;
    tm->update(delta * speedMultiplier);

    if (isSleeping_ && tm->isMidnight()) {
      passedMidnightDuringSleep_ = true;
    }

    if (isSleeping_ && passedMidnightDuringSleep_ && tm->getHour() == 6) {
      wakeUp();
    }

    if (!isSleeping_ && tm->isMidnight()) {
      isSleeping_ = true;
      passedMidnightDuringSleep_ = true;
      InventoryManager::getInstance()->removeMoney(200);
      if (player_) {
        player_->setVisible(false);
        player_->disableKeyboardControl();
      }
      if (sleepSprite_) sleepSprite_->setVisible(true);
    }
  }

  updateUI();

  if (isSleeping_) return;

  Rect bounds = background_->getBoundingBox();
  float margin = 6.0f;
  float halfW = player_->getContentSize().width * player_->getScaleX() * 0.5f;
  float halfH = player_->getContentSize().height * player_->getScaleY() * 0.5f;

  float minX = bounds.getMinX() + halfW + margin;
  float maxX = bounds.getMaxX() - halfW - margin;
  float minY = bounds.getMinY() + halfH + margin;
  float maxY = bounds.getMaxY() - halfH - margin;

  Vec2 pos = player_->getPosition();
  pos.x = clampf(pos.x, minX, maxX);
  pos.y = clampf(pos.y, minY, maxY);
  player_->setPosition(pos);
}

void HouseScene::initControls() {
  auto keyListener = EventListenerKeyboard::create();
  keyListener->onKeyPressed = CC_CALLBACK_2(HouseScene::onKeyPressed, this);
  _eventDispatcher->addEventListenerWithSceneGraphPriority(keyListener, this);
}

void HouseScene::onKeyPressed(EventKeyboard::KeyCode keyCode, Event* event) {
  // 如果正在睡觉
  if (isSleeping_) {
    // 按起床键手动起床
    if (keyCode == EventKeyboard::KeyCode::KEY_K) {
      wakeUp();  // 直接调用我们刚才写的函数
    }
    return;  // 锁定其他按键
  }

  switch (keyCode) {
    case EventKeyboard::KeyCode::KEY_J: {
      if (!background_ || !player_) break;

      // 1. 获取玩家在背景图内的坐标
      Vec2 playerPosLocal =
          background_->convertToNodeSpace(player_->getPosition());

      // 2. 定义触发区域
      // 只在横坐标 154~177 范围内触发
      // 纵坐标范围与床一致：50~105
      float triggerX = 154.0f;
      float triggerY = 50.0f;
      float triggerW = 177.0f - 154.0f;  // 23 像素宽
      float triggerH = 105.0f - 50.0f;   // 55 像素高

      Rect triggerArea(triggerX, triggerY, triggerW, triggerH);

      // 3. 判定
      if (triggerArea.containsPoint(playerPosLocal)) {
        isSleeping_ = true;
        passedMidnightDuringSleep_ = false;
        player_->disableKeyboardControl();
        player_->setVisible(false);  // 隐藏玩家

        if (sleepSprite_) {
          sleepSprite_->setVisible(true);  // 显示刚才定好位置的贴图
        }

      } else {
        // 2. 定义电视机触发区域
        // 用户输入坐标（左上角）：(160, 64) 到 (196, 79)
        // 转换后（引擎左下角）:
        // 纵坐标：192 - 79 = 113 到 192 - 64 = 128
        Rect tvArea(160.0f, 113.0f, 36.0f, 15.0f);

        if (tvArea.containsPoint(playerPosLocal)) {
          showWeatherForecast();
        } else {
        }
      }
    } break;

    case EventKeyboard::KeyCode::KEY_ENTER: {
      if (!background_ || !player_) break;

      // 1. 获取玩家在背景图内的坐标
      Vec2 playerPosLocal =
          background_->convertToNodeSpace(player_->getPosition());

      // 2. 定义区域
      // 原坐标（画图软件）：横坐标[61, 97]，纵坐标[170, 190]
      // 转换后（引擎）：横坐标[61, 97]，纵坐标[2, 22]，即图片底边

      Rect doorArea(61.0f, 2.0f, 36.0f, 20.0f);

      // 3. 判定
      if (doorArea.containsPoint(playerPosLocal)) {
        if (isPassedOut_) {
          Director::getInstance()->replaceScene(
              TransitionFade::create(0.5f, GameScene::createScene()));
        } else {
          Director::getInstance()->popScene();
        }
      } else {
        // 调试日志：显示玩家当前坐标与门的位置
      }
    } break;
    case EventKeyboard::KeyCode::KEY_TAB: {
      auto tm = TimeManager::getInstance();
      if (tm) {
        this->unscheduleUpdate();

        tm->skipToNextMorning();
        Director::getInstance()->replaceScene(
            TransitionFade::create(1.0f, HouseScene::createScene(true)));
      }
      break;
    }
    case EventKeyboard::KeyCode::KEY_B:
    case EventKeyboard::KeyCode::KEY_I:
      toggleInventory();
      break;

    default:
      break;
  }
}
