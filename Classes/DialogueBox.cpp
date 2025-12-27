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

    // Background
    background_ = Sprite::create("npcImages/dialogueBox.png");
    if (!background_) {
        CCLOG("Failed to load dialogueBox.png");
        return false;
    }
    // background_->setPosition(Vec2(0, 0)); // Center relative to node
    this->addChild(background_);

    // Portrait
    // Default portrait placeholder or none
    portrait_ = Sprite::create();
    this->addChild(portrait_, 1); // Ensure above background, Z=1
    if (npc) {
        setNpc(npc);
    }

    // Label
    dialogue_label_ = Label::createWithSystemFont("", "Arial", 28);
    dialogue_label_->setDimensions(480, 160); // Reduced width for left panel
    // Align text to the center of left panel (even further left as requested)
    dialogue_label_->setPosition(Vec2(-250, 0));
    dialogue_label_->setTextColor(Color4B::BLACK);
    dialogue_label_->setAlignment(TextHAlignment::CENTER, TextVAlignment::CENTER);
    this->addChild(dialogue_label_, 2); // Z=2
    

    
    // Position the whole box (e.g. bottom of screen)
    Size visibleSize = Director::getInstance()->getVisibleSize();
    this->setPosition(Vec2(visibleSize.width / 2, 150));
    
    this->setVisible(false);

    // Touch Event Listener
    auto touchListener = EventListenerTouchOneByOne::create();
    touchListener->setSwallowTouches(true);
    touchListener->onTouchBegan = [this](Touch* touch, Event* event) {
        if (!is_visible_) return false;

        // Handle Choice Click
        if (waiting_for_choice_ && choice_node_->isVisible()) {

            // Standard Hit Test (Same as InventoryUI)
            Vec2 locationInNode = choice_node_->convertTouchToNodeSpace(touch);

            // Button sizing: 150x50. Pos: -80 and +80.
            // Option 1 (Left): (-80, 0). Bounds: [-155, -5] x [-40, 40]
            if (locationInNode.x > -155 && locationInNode.x < -5 && locationInNode.y > -40 && locationInNode.y < 40) {
                if (choice_callback_) choice_callback_(0); // Buy
                return true;
            }
            // Option 2 (Right): (80, 0). Bounds: [5, 155] x [-40, 40]
            if (locationInNode.x > 5 && locationInNode.x < 155 && locationInNode.y > -40 && locationInNode.y < 40) {
                if (choice_callback_) choice_callback_(1); // Sell
                return true;
            }
        }

        // Normal click to advance text
        if (on_click_callback_) on_click_callback_();
        return true;
    };
    _eventDispatcher->addEventListenerWithSceneGraphPriority(touchListener, this);

    waiting_for_choice_ = false;

    // Create Choice UI Container
    choice_node_ = Node::create();
    // Let's put choices inside the box but near bottom right or clearly separated
    // Box dimensions are roughly 800x200?
    // Label is at (100, 0) relative to center?
    // Let's place choices at relative coordinates.
    // Box center is (0,0) of this node.
    // Center the choice node near the bottom, moved left by 50
    choice_node_->setPosition(Vec2(-50, -60));
    this->addChild(choice_node_, 5);
    choice_node_->setVisible(false);

    // Option 1 (Left/Top)
    auto opt1Bg = DrawNode::create();
    // Darker gray for visibility
    opt1Bg->drawSolidRect(Vec2(-75, -25), Vec2(75, 25), Color4F(0.5f, 0.5f, 0.5f, 1.0f));
    opt1Bg->setPosition(Vec2(-80, 0));
    choice_node_->addChild(opt1Bg);
    option1_label_ = Label::createWithSystemFont("Option 1", "Arial", 24); // Larger font
    option1_label_->setPosition(Vec2(-80, 0));
    option1_label_->setTextColor(Color4B::BLACK);
    choice_node_->addChild(option1_label_);

    // Option 2 (Right/Bottom)
    auto opt2Bg = DrawNode::create();
    opt2Bg->drawSolidRect(Vec2(-75, -25), Vec2(75, 25), Color4F(0.5f, 0.5f, 0.5f, 1.0f));
    opt2Bg->setPosition(Vec2(80, 0));
    choice_node_->addChild(opt2Bg);
    option2_label_ = Label::createWithSystemFont("Option 2", "Arial", 24); // Larger font
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
        // Move portrait to the left side
        // Move portrait to the right side box
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

// Deprecated old method, keeping for compatibility if needed or redirecting
void DialogueBox::showDialogue() {
    if (npc_) {
        showDialogue(npc_->getDialogue());
    }
}

void DialogueBox::showNextLine() {
    // If waiting for choice, click shouldn't advance text
    if (waiting_for_choice_) return;

    // In complex dialogue, GameScene controls flow.
    // This simple method was for random multi-line.
    // We'll leave it empty or basic.
    closeDialogue();
}

void DialogueBox::showChoices(const std::string& option1, const std::string& option2, const ChoiceCallback& callback)
{
    option1_label_->setString(option1);
    option2_label_->setString(option2);
    choice_callback_ = callback;
    // Show simple prompt?
    // For now we assume the dialogue text itself explains instructions or we can append.
    // Let's just set the state.
    // Optionally we can show a visual hint.

    choice_callback_ = callback;
    waiting_for_choice_ = true;
    is_visible_ = true; // Force visible
    this->setVisible(true); // Force node visible
    choice_node_->setVisible(true); // Show buttons
}

void DialogueBox::hideChoices()
{
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


