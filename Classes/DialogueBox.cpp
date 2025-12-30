#include "DialogueBox.h"

USING_NS_CC;

DialogueBox* DialogueBox::create(Npc* npc) {
  DialogueBox* node = new (std::nothrow) DialogueBox();
  if (node && node->init(npc)) {
    node->autorelease();
    return node;
  }
  CC_SAFE_DELETE(node);
  return nullptr;
}

bool DialogueBox::init(Npc* npc) {
  if (!Node::init()) return false;

  npc_ = npc;
  is_visible_ = false;

  background_ = Sprite::create("npcImages/dialogueBox.png");
  if (!background_) {
    return false;
  }
  this->addChild(background_);

  portrait_ = Sprite::create();
  this->addChild(portrait_, 1);
  if (npc) {
    setNpc(npc);
  }

  dialogue_label_ = Label::createWithSystemFont("", "Arial", 28);
  dialogue_label_->setDimensions(480, 160);
  dialogue_label_->setPosition(Vec2(-250, 0));
  dialogue_label_->setTextColor(Color4B::BLACK);
  dialogue_label_->setAlignment(TextHAlignment::CENTER, TextVAlignment::CENTER);
  this->addChild(dialogue_label_, 2);

  Size visibleSize = Director::getInstance()->getVisibleSize();
  this->setPosition(Vec2(visibleSize.width / 2, 150));

  this->setVisible(false);

  auto touchListener = EventListenerTouchOneByOne::create();
  touchListener->setSwallowTouches(true);
  touchListener->onTouchBegan = [this](Touch* touch, Event* event) {
    if (!is_visible_) return false;

    if (waiting_for_choice_ && choice_node_->isVisible()) {
      Vec2 locationInNode = choice_node_->convertTouchToNodeSpace(touch);

      if (locationInNode.x > -155 && locationInNode.x < -5 &&
          locationInNode.y > -257 && locationInNode.y < -177) {
        if (choice_callback_) choice_callback_(0);
        return true;
      }
      if (locationInNode.x > 5 && locationInNode.x < 155 &&
          locationInNode.y > -257 && locationInNode.y < -177) {
        if (choice_callback_) choice_callback_(1);
        return true;
      }
    }

    if (on_click_callback_) on_click_callback_();
    return true;
  };
  _eventDispatcher->addEventListenerWithSceneGraphPriority(touchListener, this);

  waiting_for_choice_ = false;

  choice_node_ = Node::create();
  choice_node_->setPosition(Vec2(-50, -60));
  this->addChild(choice_node_, 5);
  choice_node_->setVisible(false);

  auto opt1Bg = DrawNode::create();
  opt1Bg->drawSolidRect(Vec2(-75, -25), Vec2(75, 25),
                        Color4F(0.5f, 0.5f, 0.5f, 1.0f));
  opt1Bg->setPosition(Vec2(-80, 0));
  choice_node_->addChild(opt1Bg);
  option1_label_ = Label::createWithSystemFont("Option 1", "Arial", 24);
  option1_label_->setPosition(Vec2(-80, 0));
  option1_label_->setTextColor(Color4B::BLACK);
  choice_node_->addChild(option1_label_);

  auto opt2Bg = DrawNode::create();
  opt2Bg->drawSolidRect(Vec2(-75, -25), Vec2(75, 25),
                        Color4F(0.5f, 0.5f, 0.5f, 1.0f));
  opt2Bg->setPosition(Vec2(80, 0));
  choice_node_->addChild(opt2Bg);
  option2_label_ = Label::createWithSystemFont("Option 2", "Arial", 24);
  option2_label_->setPosition(Vec2(80, 0));
  option2_label_->setTextColor(Color4B::BLACK);
  choice_node_->addChild(option2_label_);

  return true;
}

void DialogueBox::setNpc(Npc* npc) {
  npc_ = npc;
  if (npc_) {
    std::string portraitFile = npc_->getPortraitFile();
    if (FileUtils::getInstance()->isFileExist(portraitFile)) {
      portrait_->setTexture(portraitFile);
    }
    portrait_->setPosition(Vec2(300, 0));

    if (portrait_->getContentSize().height > 200) {
      float scale = 180.0f / portrait_->getContentSize().height;
      portrait_->setScale(scale);
    } else {
      portrait_->setScale(1.0f);
    }
    portrait_->setVisible(true);
  } else {
    portrait_->setVisible(false);
  }
}

void DialogueBox::showDialogue(const std::string& text) {
  waiting_for_choice_ = false;
  choice_node_->setVisible(false);

  this->setVisible(true);
  is_visible_ = true;

  dialogue_label_->setString(text);
}

void DialogueBox::showDialogue() {
  if (npc_) {
    showDialogue(npc_->getDialogue());
  }
}

void DialogueBox::showNextLine() {
  if (waiting_for_choice_) return;

  closeDialogue();
}

void DialogueBox::showChoices(const std::string& option1,
                              const std::string& option2,
                              const ChoiceCallback& callback) {
  option1_label_->setString(option1);
  option2_label_->setString(option2);
  choice_callback_ = callback;

  choice_callback_ = callback;
  waiting_for_choice_ = true;
  is_visible_ = true;
  this->setVisible(true);
  choice_node_->setVisible(true);
}

void DialogueBox::hideChoices() {
  choice_node_->setVisible(false);
  waiting_for_choice_ = false;
}

void DialogueBox::closeDialogue() {
  this->setVisible(false);
  is_visible_ = false;
  waiting_for_choice_ = false;
  this->setVisible(false);
  is_visible_ = false;
  waiting_for_choice_ = false;
  choice_node_->setVisible(false);
}
