#ifndef __FISHING_LAYER_H__
#define __FISHING_LAYER_H__

#include "cocos2d.h"
#include "ui/CocosGUI.h"

USING_NS_CC;

class Fish;

class FishingLayer : public Layer {
 public:
  static FishingLayer* create(Fish* fish);
  virtual bool init(Fish* fish);
  void update(float delta);

  // 设置回调
  void setFinishCallback(std::function<void(bool)> callback);

 private:
  void initUI();
  void initInput();
  void updateBarPhysics(float delta);
  void updateFishMovement(float delta);
  void updateProgress(float delta);
  void finishFishing(bool success);

 private:
  Node* barBase_;
  Node* greenBar_;
  Node* fishSprite_;
  ui::LoadingBar* progressBar_;

  float barHeight_;
  float greenBarHeight_;

  float barPosition_;
  float barSpeed_;
  float gravity_;
  float thrust_;
  float bounce_;

  float fishPosition_;  // 0.0 - 1.0
  float fishSpeed_;
  float fishTargetPos_;
  float moveTimer_;

  float catchProgress_;  // 0.0 - 1.0
  bool isHolding_;
  bool isGameOver_;

  Fish* currentFish_{nullptr};
  std::function<void(bool)> finishCallback_;
};

#endif
