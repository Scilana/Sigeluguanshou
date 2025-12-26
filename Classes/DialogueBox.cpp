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
    _dialogueLabel->setDimensions(480, 160); // Reduced width for left panel
    // Align text to the center of left panel (even further left as requested)
    _dialogueLabel->setPosition(Vec2(-250, 0)); 
    _dialogueLabel->setTextColor(Color4B::BLACK);
    _dialogueLabel->setAlignment(TextHAlignment::CENTER, TextVAlignment::CENTER);
    this->addChild(_dialogueLabel, 2); // Z=2
    

    
    // Position the whole box (e.g. bottom of screen)
    Size visibleSize = Director::getInstance()->getVisibleSize();
    this->setPosition(Vec2(visibleSize.width / 2, 150));
    
    this->setVisible(false);

    // Touch Event Listener
    auto touchListener = EventListenerTouchOneByOne::create();
    touchListener->setSwallowTouches(true);
    touchListener->onTouchBegan = [this](Touch* touch, Event* event) {
        if (!_isVisible) return false;

        // Handle Choice Click
        if (_waitingForChoice && _choiceNode->isVisible()) {
            
            // Standard Hit Test (Same as InventoryUI)
            Vec2 locationInNode = _choiceNode->convertTouchToNodeSpace(touch);

            // Button sizing: 150x50. Pos: -80 and +80.
            // Option 1 (Left): (-80, 0). Bounds: [-155, -5] x [-40, 40]
            if (locationInNode.x > -155 && locationInNode.x < -5 && locationInNode.y > -40 && locationInNode.y < 40) {
                if (_choiceCallback) _choiceCallback(0); // Buy
                return true; 
            }
            // Option 2 (Right): (80, 0). Bounds: [5, 155] x [-40, 40]
            if (locationInNode.x > 5 && locationInNode.x < 155 && locationInNode.y > -40 && locationInNode.y < 40) {
                if (_choiceCallback) _choiceCallback(1); // Sell
                return true;
            }
        } 
        
        // Normal click to advance text
        if (_onClickCallback) _onClickCallback();
        return true;
    };
    _eventDispatcher->addEventListenerWithSceneGraphPriority(touchListener, this);

    _waitingForChoice = false;

    // Create Choice UI Container
    _choiceNode = Node::create();
    // Let's put choices inside the box but near bottom right or clearly separated
    // Box dimensions are roughly 800x200?
    // Label is at (100, 0) relative to center?
    // Let's place choices at relative coordinates.
    // Box center is (0,0) of this node.
    // Center the choice node near the bottom, moved left by 50
    _choiceNode->setPosition(Vec2(-50, -60)); 
    this->addChild(_choiceNode, 5);
    _choiceNode->setVisible(false);

    // Option 1 (Left/Top)
    auto opt1Bg = DrawNode::create();
    // Darker gray for visibility
    opt1Bg->drawSolidRect(Vec2(-75, -25), Vec2(75, 25), Color4F(0.5f, 0.5f, 0.5f, 1.0f)); 
    opt1Bg->setPosition(Vec2(-80, 0));
    _choiceNode->addChild(opt1Bg);
    _option1Label = Label::createWithSystemFont("Option 1", "Arial", 24); // Larger font
    _option1Label->setPosition(Vec2(-80, 0));
    _option1Label->setTextColor(Color4B::BLACK);
    _choiceNode->addChild(_option1Label);

    // Option 2 (Right/Bottom)
    auto opt2Bg = DrawNode::create();
    opt2Bg->drawSolidRect(Vec2(-75, -25), Vec2(75, 25), Color4F(0.5f, 0.5f, 0.5f, 1.0f)); 
    opt2Bg->setPosition(Vec2(80, 0));
    _choiceNode->addChild(opt2Bg);
    _option2Label = Label::createWithSystemFont("Option 2", "Arial", 24); // Larger font
    _option2Label->setPosition(Vec2(80, 0));
    _option2Label->setTextColor(Color4B::BLACK);
    _choiceNode->addChild(_option2Label);

    return true;
}



void DialogueBox::setNpc(Npc* npc) {
    _npc = npc;
    if (_npc) {
        std::string portraitFile = _npc->getPortraitFile();
        if (FileUtils::getInstance()->isFileExist(portraitFile)) {
             _portrait->setTexture(portraitFile);
        }
        // Move portrait to the left side
        // Move portrait to the right side box
        _portrait->setPosition(Vec2(300, 0));
        
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

void DialogueBox::showDialogue(const std::string& text) {
    _waitingForChoice = false;
    _choiceNode->setVisible(false);
    
    this->setVisible(true);
    _isVisible = true;

    _dialogueLabel->setString(text);
}

// Deprecated old method, keeping for compatibility if needed or redirecting
void DialogueBox::showDialogue() {
    if (_npc) {
        showDialogue(_npc->getDialogue());
    }
}

void DialogueBox::showNextLine() {
    // If waiting for choice, click shouldn't advance text
    if (_waitingForChoice) return;
    
    // In complex dialogue, GameScene controls flow.
    // This simple method was for random multi-line.
    // We'll leave it empty or basic.
    closeDialogue();
}

void DialogueBox::showChoices(const std::string& option1, const std::string& option2, const ChoiceCallback& callback)
{
    _option1Label->setString(option1);
    _option2Label->setString(option2);
    _choiceCallback = callback;
    // Show simple prompt?
    // For now we assume the dialogue text itself explains instructions or we can append.
    // Let's just set the state.
    // Optionally we can show a visual hint.
    
    _choiceCallback = callback;
    _waitingForChoice = true;
    _isVisible = true; // Force visible
    this->setVisible(true); // Force node visible
    _choiceNode->setVisible(true); // Show buttons
}

void DialogueBox::hideChoices()
{
    _choiceNode->setVisible(false);
    _waitingForChoice = false;
}

void DialogueBox::closeDialogue() {
    this->setVisible(false);
    _isVisible = false;
    _waitingForChoice = false;
    this->setVisible(false);
    _isVisible = false;
    _waitingForChoice = false;
    _choiceNode->setVisible(false);
}


