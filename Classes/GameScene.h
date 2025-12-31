#ifndef __GAME_SCENE_H__
#define __GAME_SCENE_H__

#include <string>
#include <vector>

#include "DialogueBox.h"
#include "FarmManager.h"
#include "FishingLayer.h"
#include "InventoryManager.h" 
#include "InventoryUI.h"
#include "MapLayer.h"
#include "MarketState.h"
#include "MineScene.h"
#include "Npc.h"
#include "Player.h"
#include "SaveManager.h"
#include "StorageChest.h"
#include "cocos2d.h"

class MarketUI;
class WeatherManager;
class SkillTreeUI;
class BeachScene;
class BarnScene;

class GameScene : public cocos2d::Scene {
 public:
  
  static cocos2d::Scene* createScene();

  static cocos2d::Scene* createScene(bool loadFromSave);

  virtual bool init() override;

  bool init(bool loadFromSave);

  virtual void update(float delta) override;

  CREATE_FUNC(GameScene);

 private:

  MapLayer* mapLayer_;

  FarmManager* farmManager_;

  Player* player_;


  WeatherManager* weatherManager_;

  // 背包与系统
  InventoryManager* inventory_;
  InventoryUI* inventoryUI_;
  MarketState marketState_;
  MarketUI* marketUI_;
  SkillTreeUI* skillUI_;

  // 界面层与元素
  cocos2d::Layer* uiLayer_;
  cocos2d::LayerColor* dayNightLayer_;
  cocos2d::Label* timeLabel_;
  cocos2d::Label* moneyLabel_;
  cocos2d::Label* positionLabel_;  // 显示玩家位置
  cocos2d::Label* actionLabel_;    // 显示农场操作提示
  cocos2d::Label* itemLabel_;      // 显示当前物品
  cocos2d::LayerColor* toolbarUI_ = nullptr;
  std::vector<cocos2d::Sprite*> toolbarSlots_;
  std::vector<cocos2d::Sprite*> toolbarIcons_;
  std::vector<cocos2d::Label*> toolbarCounts_;
  std::vector<int> toolbarCountCache_;
  int toolbarSelectedCache_ = -1;

  void initMap();
  void initFarm();
  void initPlayer();
  void initUI();
  void initCamera();
  void initControls();
  void initTrees();  // 初始化调试用树木标记
  void initWeather();
  void initNpcs();
  void initToolbarUI();
  void refreshToolbarUI();

  // 更新循环函数
  void updateCamera();  // 更新摄像机位置（跟随玩家）
  void updateUI();      // 更新界面显示
  void updateWeather();
  void updateDayNightLighting();

  // 控制与交互
  // 商人交互状态
  enum class MerchantState {
    None,
    Greeting,
    Choice,
    BuyTransition,
    BuyPre,
    Buy,
    SellPre,
    Sell,
    TradeConfirm,
    End,
    Reject
  };
  MerchantState merchantState_;
  Npc* activeNpc_;

  ItemType pendingSellItem_;
  int pendingSellCount_;

  void startMerchantInteraction(Npc* npc);
  void advanceMerchantDialogue();
  void endMerchantInteraction();
  void onMerchantChoice(int choiceIndex);

  void handleSellSelection(int slotIndex, ItemType type, int count);
  void onQuantityConfirmed(int quantity);
  void onTradeConfirmResult(bool confirmed);

  // ==========================================
  void backToMenu();
  void toggleInventory();
  void toggleSkillTree();
  void onInventoryClosed();
  void toggleMarket();
  void onMarketClosed();
  void enterMine();
  void enterHouse();
  void enterBarn();
  void enterBeach();
  void checkBeachEntrance();

  
  //检查玩家是否在电梯附近
  bool isPlayerNearElevator() const;
  bool isPlayerNearHouseDoor() const;
  bool isPlayerNearBarnDoor() const;
  bool isPlayerAtBeachEntrance() const;

  bool enteringBeach_ = false;

  // 矿井入口位置（右下角的房子）
  const cocos2d::Vec2 ELEVATOR_POS = cocos2d::Vec2(1202, 226);

  void handleFarmAction(bool waterOnly);

  void handleChestPlacement();

  void openChestInventory(StorageChest* chest);

  void showActionMessage(const std::string& text,
                         const cocos2d::Color3B& color);

  void onKeyPressed(cocos2d::EventKeyboard::KeyCode keyCode,
                    cocos2d::Event* event);

  void onMouseDown(cocos2d::Event* event);
  void onMouseUp(cocos2d::Event* event);

  void startFishing();
  void updateFishingState(float delta);

  enum class FishingState { NONE, CHARGING, WAITING, BITING, REELING };
  FishingState fishingState_ = FishingState::NONE;
  float chargePower_ = 0.0f;
  float fishingTimer_ = 0.0f;
  float waitTimer_ = 0.0f;
  float biteTimer_ = 0.0f;
  bool isFishing_ = false;
  int fishingRodSlotIndex_ = -1;

  cocos2d::Sprite* chargeBarBg_ = nullptr;
  cocos2d::Sprite* chargeBarFg_ = nullptr;
  cocos2d::Sprite* exclamationMark_ = nullptr;

  DialogueBox* dialogueBox_ = nullptr;
  std::vector<Npc*> npcs_;

  // 快捷栏
  std::vector<ItemType> toolbarItems_;
  int selectedItemIndex_;
  void initToolbar();
  void selectItemByIndex(int idx);
  int getCropIdForItem(ItemType type) const;
  ItemType getItemTypeForCropId(int cropId) const;

  int lastWeatherDay_ = 0;

  // 砍树系统（沿用旧结构）
  struct TreeChopData {
    cocos2d::Vec2 tileCoord;            // 点击的瓦片坐标
    std::vector<cocos2d::Vec2> tiles;   // 整棵树包含的所有瓦片
    cocos2d::Sprite* treeSprite;        // 替换后的树木精灵
    float chopTimer;                    // 计时器（用于超时重置）
    int chopCount;                      // 当前砍伐次数
    static const int CHOPS_NEEDED = 3;  // 砍倒所需的次数
  };
  std::vector<TreeChopData> activeChops_;    // 当前正在被砍的树
  std::vector<cocos2d::Vec2> choppedTrees_;  // 已砍倒的树木位置（用于存档）

  // 调试用树木结构 (旧)
  struct Tree {
    cocos2d::Vec2 tileCoord;
    cocos2d::Node* node;
  };
  std::vector<Tree> trees_;
  int findTreeIndex(const cocos2d::Vec2& tile) const;

  std::vector<cocos2d::Vec2> collectCollisionComponent(
      const cocos2d::Vec2& start) const;

  bool findNearbyCollisionTile(const cocos2d::Vec2& centerTile,
                               cocos2d::Vec2& outTile) const;

  cocos2d::Sprite* createTreeSprite(const std::vector<cocos2d::Vec2>& tiles);

  void playTreeShakeAnimation(cocos2d::Sprite* treeSprite);

  void playTreeFallAnimation(TreeChopData* chopData);

  void spawnItem(ItemType type, const cocos2d::Vec2& position, int count);

  void updateChopping(float delta);
  // 存档系统
  void saveGame();

  void loadGame();

  SaveManager::SaveData collectSaveData();

  void applySaveData(const SaveManager::SaveData& data);
};

#endif
