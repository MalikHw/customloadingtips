#include <Geode/Geode.hpp>
#include <Geode/loader/SettingV3.hpp>
#include <Geode/modify/LoadingLayer.hpp>
#include <Geode/ui/Popup.hpp>
#include <Geode/binding/ButtonSprite.hpp>
#include <Geode/binding/CCMenuItemSpriteExtra.hpp>
#include <Geode/binding/CCTextInputNode.hpp>
#include <Geode/binding/TextArea.hpp>

using namespace geode::prelude;

// helpers
static std::vector<std::string> getLines() {
    auto raw = Mod::get()->getSavedValue<std::string>("tips-text", "");
    std::vector<std::string> lines;
    std::stringstream ss(raw);
    std::string line;
    while (std::getline(ss, line)) {
        // trim whitespace
        auto start = line.find_first_not_of(" \t\r");
        auto end   = line.find_last_not_of(" \t\r");
        if (start != std::string::npos)
            lines.push_back(line.substr(start, end - start + 1));
    }
    return lines;
}

// notepad-ahh popup
class NotepadPopup : public geode::Popup {
protected:
    CCTextInputNode* m_input = nullptr;

    bool init() {
        if (!Popup::init(360.f, 280.f))
            return false;

        this->setTitle("Custom Loading Tips");

        auto winSize = m_mainLayer->getContentSize();

        // instructions label
        auto hint = CCLabelBMFont::create("One tip per line. Replaces RobTop's loading tips.", "chatFont.fnt");
        hint->setScale(0.45f);
        hint->setColor({180, 180, 180});
        hint->setPosition({winSize.width / 2.f, winSize.height - 42.f});
        m_mainLayer->addChild(hint);

        // text area bg
        auto bg = CCScale9Sprite::create("square02b_001.png");
        bg->setColor({0, 0, 0});
        bg->setOpacity(100);
        bg->setContentSize({winSize.width - 40.f, winSize.height - 100.f});
        bg->setPosition({winSize.width / 2.f, (winSize.height - 100.f) / 2.f + 10.f});
        m_mainLayer->addChild(bg);

        // multi-line text input
        float inputWidth  = winSize.width - 60.f;
        float inputHeight = winSize.height - 120.f;

        m_input = CCTextInputNode::create(inputWidth, inputHeight, "Type your tips here...", "chatFont.fnt");
        m_input->setPosition({winSize.width / 2.f, (winSize.height - 100.f) / 2.f + 10.f});
        m_input->setMaxLabelWidth(inputWidth);

        // load existing saved text
        auto saved = Mod::get()->getSavedValue<std::string>("tips-text", "");
        if (!saved.empty())
            m_input->setString(saved.c_str());

        m_mainLayer->addChild(m_input);

        // save button
        auto saveSpr = ButtonSprite::create("Save", "goldFont.fnt", "GJ_button_01.png", 0.8f);
        saveSpr->setScale(0.75f);
        auto saveBtn = CCMenuItemSpriteExtra::create(saveSpr, this,
            menu_selector(NotepadPopup::onSave));

        auto menu = CCMenu::create();
        menu->addChild(saveBtn);
        menu->setPosition({winSize.width / 2.f, 18.f});
        m_mainLayer->addChild(menu);

        return true;
    }

    void onSave(CCObject*) {
        if (m_input) {
            Mod::get()->setSavedValue<std::string>("tips-text", std::string(m_input->getString()));
        }
        this->onClose(nullptr);
    }

public:
    static NotepadPopup* create() {
        auto ret = new NotepadPopup();
        // 360 wide, 280 tall so it'll be comfortable for multi-line text
        if (ret->init()) {
            ret->autorelease();
            return ret;
        }
        CC_SAFE_DELETE(ret);
        return nullptr;
    }
};

// custom setting (button)

class OpenNotepadSettingV3 : public SettingV3 {
public:
    static Result<std::shared_ptr<SettingV3>> parse(
        std::string const& key,
        std::string const& modID,
        matjson::Value const& json
    ) {
        auto res  = std::make_shared<OpenNotepadSettingV3>();
        auto root = checkJson(json, "OpenNotepadSettingV3");
        res->init(key, modID, root);
        res->parseNameAndDescription(root);
        res->parseEnableIf(root);
        root.checkUnknownKeys();
        return root.ok(std::static_pointer_cast<SettingV3>(res));
    }

    bool load(matjson::Value const&) override { return true; }
    bool save(matjson::Value&) const override  { return true; }
    bool isDefaultValue() const override       { return true; }
    void reset() override {}

    SettingNodeV3* createNode(float width) override;
};

class OpenNotepadSettingNodeV3 : public SettingNodeV3 {
protected:
    ButtonSprite*          m_buttonSprite = nullptr;
    CCMenuItemSpriteExtra* m_button       = nullptr;

    bool init(std::shared_ptr<OpenNotepadSettingV3> setting, float width) {
        if (!SettingNodeV3::init(setting, width))
            return false;

        m_buttonSprite = ButtonSprite::create("Edit Tips", "goldFont.fnt", "GJ_button_01.png", 0.8f);
        m_buttonSprite->setScale(0.5f);
        m_button = CCMenuItemSpriteExtra::create(
            m_buttonSprite, this,
            menu_selector(OpenNotepadSettingNodeV3::onOpen)
        );
        this->getButtonMenu()->addChildAtPosition(m_button, Anchor::Center);
        this->getButtonMenu()->setContentWidth(80.f);
        this->getButtonMenu()->updateLayout();
        this->updateState(nullptr);
        return true;
    }

    void updateState(CCNode* invoker) override {
        SettingNodeV3::updateState(invoker);
        auto ok = this->getSetting()->shouldEnable();
        m_button->setEnabled(ok);
        m_buttonSprite->setCascadeColorEnabled(true);
        m_buttonSprite->setCascadeOpacityEnabled(true);
        m_buttonSprite->setOpacity(ok ? 255 : 155);
        m_buttonSprite->setColor(ok ? ccWHITE : ccGRAY);
    }

    void onOpen(CCObject*) {
        NotepadPopup::create()->show();
    }

    void onCommit() override {}
    void onResetToDefault() override {}

public:
    static OpenNotepadSettingNodeV3* create(
        std::shared_ptr<OpenNotepadSettingV3> setting, float width
    ) {
        auto ret = new OpenNotepadSettingNodeV3();
        if (ret->init(setting, width)) {
            ret->autorelease();
            return ret;
        }
        delete ret;
        return nullptr;
    }

    bool hasUncommittedChanges() const override { return false; }
    bool hasNonDefaultValue()   const override  { return false; }
};

SettingNodeV3* OpenNotepadSettingV3::createNode(float width) {
    return OpenNotepadSettingNodeV3::create(
        std::static_pointer_cast<OpenNotepadSettingV3>(shared_from_this()), width
    );
}

// register the custom setting type
$on_mod(Loaded) {
    (void)Mod::get()->registerCustomSettingType("open-notepad", &OpenNotepadSettingV3::parse);
}

// LoadingLayer
class $modify(MyLoadingLayer, LoadingLayer) {
    bool init(bool refresh) {
        if (!LoadingLayer::init(refresh))
            return false;

        auto lines = getLines();
        if (lines.empty())
            return true; // keep RobTop's tips

        // pick a random line
        int idx = rand() % static_cast<int>(lines.size());
        auto chosen = lines[idx];

        // m_textArea holds the loading tip so replace its string
        if (m_textArea) {
            m_textArea->setString(chosen.c_str());
        }

        return true;
    }
};
