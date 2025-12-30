#include "Npc.h"

USING_NS_CC;

Npc* Npc::create(const std::string& name, const std::string& spriteFile,
                 NpcType type) {
  Npc* npc = new (std::nothrow) Npc();
  if (npc && npc->init(name, spriteFile, type)) {
    npc->autorelease();
    return npc;
  }
  CC_SAFE_DELETE(npc);
  return nullptr;
}

bool Npc::init(const std::string& name, const std::string& spriteFile,
               NpcType type) {
  if (!Sprite::initWithFile(spriteFile)) {
    return false;
  }
  name_ = name;
  type_ = type;

  dialogues_.push_back("Hello there, traveler!");
  dialogues_.push_back("Nice weather today.");
  dialogues_.push_back("I love this island.");

  return true;
}

std::string Npc::getDialogue() const {
  if (dialogues_.empty()) return "...";
  int idx = cocos2d::random(0, static_cast<int>(dialogues_.size()) - 1);
  return dialogues_[idx];
}

std::string Npc::getPortraitFile() const {
  if (name_ == "Wizard") return "NPC/bussiness_head_processed.png";
  if (name_ == "Blacksmith") return "NPC/blacksmith_processed.png";
  if (name_ == "Cleaner") return "npcImages/Cleaner LeviTalk.png";
  return "npcImages/Wizard YuuuTalk.png";
}

void Npc::interact() {}
