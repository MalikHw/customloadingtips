#include <Geode/Geode.hpp>
#include <Geode/loader/SettingV3.hpp>
#include <Geode/modify/LoadingLayer.hpp>
#include <Geode/ui/Popup.hpp>
#include <Geode/ui/TextInput.hpp>
#include <Geode/binding/ButtonSprite.hpp>
#include <Geode/binding/CCMenuItemSpriteExtra.hpp>
#include <Geode/binding/TextArea.hpp>

using namespace geode::prelude;

// helpers — splits on literal \n in the saved string
static std::vector<std::string> getLines() {
    auto raw = Mod::get()->getSavedValue<std::string>("tips-text", "");
    std::vector<std::string> lines;
    size_t pos = 0;
    while (pos < raw.size()) {
        auto next = raw.find("\\n", pos);
        if (next == std::string::npos) next = raw.size();
        auto chunk = raw.substr(pos, next - pos);
        // trim whitespace
        auto start = chunk.find_first_not_of(" \t\r\n");
        auto end   = chunk.find_last_not_of(" \t\r\n");
        if (start != std::string::npos)
            lines.push_back(chunk.substr(start, end - start + 1));
        pos = next + 2; // skip past "\n"
    }
    return lines;
}

// notepad-ahh popup
class NotepadPopup : public geode::Popup {
protected:
    geode::TextInput* m_textInput = nullptr;

    bool init() {
        if (!Popup::init(380.f, 160.f))
            return false;

        this->setTitle("Custom Loading Tips");

        float W = 380.f;
        float H = 160.f;

        // instructions label
        auto hint = CCLabelBMFont::create("Separate tips with \\n  (e.g.  tip1\\ntip2\\ntip3)", "chatFont.fnt");
        hint->setScale(0.40f);
        hint->setColor({180, 180, 180});
        hint->setPosition({W / 2.f, H - 30.f});
        m_mainLayer->addChild(hint);

        // wide single-line input
        float inputW = W - 32.f;
        m_textInput = geode::TextInput::create(inputW, "tip1\\ntip2\\ntip3...", "chatFont.fnt");
        m_textInput->setCommonFilter(CommonFilter::Any);
        m_textInput->setMaxCharCount(0); // unlimited

        // load saved text
        auto saved = Mod::get()->getSavedValue<std::string>("tips-text", "");
        if (!saved.empty())
            m_textInput->setString(saved);

        m_mainLayer->addChildAtPosition(m_textInput, Anchor::Center, {0.f, 10.f});

        // save button
        auto saveSpr = ButtonSprite::create("Save", "goldFont.fnt", "GJ_button_01.png", 0.8f);
        saveSpr->setScale(0.75f);
        auto saveBtn = CCMenuItemSpriteExtra::create(saveSpr, this,
            menu_selector(NotepadPopup::onSave));

        auto menu = CCMenu::create();
        menu->addChild(saveBtn);
        menu->setPosition({W / 2.f, 18.f});
        m_mainLayer->addChild(menu);

        return true;
    }

    void onSave(CCObject*) {
        if (m_textInput) {
            Mod::get()->setSavedValue<std::string>("tips-text",
                std::string(m_textInput->getString()));
        }
        this->onClose(nullptr);
    }

public:
    static NotepadPopup* create() {
        auto ret = new NotepadPopup();
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
    srand(static_cast<unsigned int>(time(nullptr))); // seed so tips r random
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
