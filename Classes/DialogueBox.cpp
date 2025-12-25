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

    _npc = npc;
    _isVisible = false;

    // Background
    _background = Sprite::create("npcImages/dialogueBox.png");
    if (!_background) {
        CCLOG("Failed to load dialogueBox.png");
        return false;
    }
    // _background->setPosition(Vec2(0, 0)); // Center relative to node
    this->addChild(_background);

    // Portrait
    // Default portrait placeholder or none
    _portrait = Sprite::create();
    this->addChild(_portrait, 1); // Ensure above background, Z=1
    if (npc) {
        setNpc(npc);
    }

    // Label
    _dialogueLabel = Label::createWithSystemFont("", "Arial", 28);
    _dialogueLabel->setDimensions(700, 160);
    // Align text to the right of the portrait
    _dialogueLabel->setPosition(Vec2(100, 0)); 
    _dialogueLabel->setTextColor(Color4B::BLACK);
    _dialogueLabel->setAlignment(TextHAlignment::LEFT, TextVAlignment::CENTER);
    this->addChild(_dialogueLabel, 2); // Z=2
    
    // Position the whole box (e.g. bottom of screen)
    Size visibleSize = Director::getInstance()->getVisibleSize();
    this->setPosition(Vec2(visibleSize.width / 2, 150));
    
    this->setVisible(false);

    // Mouse Listener
    auto listener = EventListenerMouse::create();
    listener->onMouseDown = CC_CALLBACK_1(DialogueBox::onMouseDown, this);
    _eventDispatcher->addEventListenerWithSceneGraphPriority(listener, this);
    _mouseListener = listener;

    return true;
}

void DialogueBox::setNpc(Npc* npc) {
    _npc = npc;
    if (_npc) {
        std::string portraitFile = _npc->getPortraitFile();
        _portrait->setTexture(portraitFile);
        // Move portrait to the left side
        _portrait->setPosition(Vec2(-350, 0));
        // Scale portrait if too large (assuming 256x256 or similar, fit in box height ~200)
        // If box is 150-200 high, portrait should be scaled.
        // Let's protect against huge portraits:
        if (_portrait->getContentSize().height > 200) {
            float scale = 180.0f / _portrait->getContentSize().height;
            _portrait->setScale(scale);
        } else {
             _portrait->setScale(1.0f);
        }
        _portrait->setVisible(true);
    } else {
        _portrait->setVisible(false);
    }
}

void DialogueBox::showDialogue() {
    if (!_npc) return;
    
    // Load full conversation
    _lines = _npc->getDialogues();
    if (_lines.empty()) {
        _lines.push_back("...");
    }
    _currentLineIndex = -1;
    
    this->setVisible(true);
    _isVisible = true;

    showNextLine();
}

void DialogueBox::showNextLine() {
    _currentLineIndex++;
    if (_currentLineIndex < (int)_lines.size()) {
        _dialogueLabel->setString(_lines[_currentLineIndex]);
    } else {
        closeDialogue();
    }
}

void DialogueBox::closeDialogue() {
    this->setVisible(false);
    _isVisible = false;
}

void DialogueBox::onMouseDown(Event* event) {
    if (!_isVisible) return;
    
    EventMouse* e = (EventMouse*)event;
    if (e->getMouseButton() == EventMouse::MouseButton::BUTTON_LEFT) {
        // Advance to next line instead of closing immediately
        showNextLine();
    }
}
