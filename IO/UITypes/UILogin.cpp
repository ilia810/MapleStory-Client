#include "UILogin.h"

#include "UILoginNotice.h"
#include "UILoginWait.h"

#include "../UI.h"
#include "../Window.h"

#include "../Components/MapleButton.h"
#include "../Components/TwoSpriteButton.h"

#include "../../Audio/Audio.h"

#include "../../Net/Packets/LoginPackets.h"
#include "../../Util/Misc.h"

#include <windows.h>

#ifdef USE_NX
#include <nlnx/nx.hpp>
#endif

namespace ms
{
    UILogin::UILogin() : UIElement(Point<int16_t>(0, 0), Point<int16_t>(800, 600)), title_pos(Point<int16_t>(0, 0)), nexon(false), showLoginHelpText(false)
    {
        LOG(LOG_DEBUG, "[UILogin] Constructor starting...");

        std::string LoginMusicNewtro = Configuration::get().get_login_music_newtro();
        Music(LoginMusicNewtro).play();

        std::string version_text = Configuration::get().get_version();
        version = Text(Text::Font::A12B, Text::Alignment::LEFT, Color::Name::LEMONGRASS, "Ver. " + version_text);
        version_pos = Point<int16_t>(10, 580);

        loginButtonMissingText = Text(Text::Font::A12B, Text::Alignment::CENTER, Color::Name::YELLOW, "Press ENTER to login");

        // Get Login.img->Title node
        nl::node Login = nl::nx::UI["Login.img"];
        nl::node Title = Login["Title"];

        // === BACKGROUND ELEMENTS ===
        // Helper lambda to get origin from node and create sprite at absolute top-left position
        auto add_sprite_abs = [&](nl::node node, Point<int16_t> topleft) {
            if (node) {
                // Get origin from node and add it to position so top-left lands at specified coords
                Point<int16_t> origin(node["origin"]);
                sprites.emplace_back(node, topleft + origin);
            }
        };

        // Background texture (800x600) at (0,0)
        add_sprite_abs(Title["background"], Point<int16_t>(0, 0));

        // Title effect animations - all rays drawn behind everything
        if (Title["effect"])
        {
            for (nl::node effect : Title["effect"])
            {
                sprites.emplace_back(effect, Point<int16_t>(200, 30));
            }
        }

        // Frame/border from Common - stretched to fill 800x600
        if (Login["Common"]["frame"]) {
            frame = Texture(Login["Common"]["frame"]);
            // Stretch to fill screen - add extra pixels to ensure corners touch
            frame_stretch = Point<int16_t>(808, 608);

        }

        // MapleStory logo at (202, 50)
        add_sprite_abs(Title["MSTitle"], Point<int16_t>(202, 50));

        // Signboard (login panel) at (279, 267)
        add_sprite_abs(Title["signboard"], Point<int16_t>(279, 267));

        // ID and PW field backgrounds - stored separately to hide when text is entered
        if (Title["ID"]) {
            id_texture = Texture(Title["ID"]);
            Point<int16_t> origin(Title["ID"]["origin"]);
            id_texture_pos = Point<int16_t>(294, 282) + origin;
        }
        if (Title["PW"]) {
            pw_texture = Texture(Title["PW"]);
            Point<int16_t> origin(Title["PW"]["origin"]);
            pw_texture_pos = Point<int16_t>(294, 308) + origin;
        }

        // === CHECKBOXES ===
        // Checkbox at (489, 329)
        if (Title["check"]) {
            check[false] = Texture(Title["check"]["0"]);
            check[true] = Texture(Title["check"]["1"]);
        }

        // Capslock warning
        if (Title["capslock"]) {
            capslock = Texture(Title["capslock"]);
        }

        // === BUTTONS ===
        // BtLogin at (658, 260) -- Adjusted Right/Up to center on inputs
        if (Title["BtLogin"]) {
            buttons[Buttons::BtLogin] = std::make_unique<MapleButton>(Title["BtLogin"], Point<int16_t>(459, 278));
        }
        else {
            showLoginHelpText = true;
        }

        // BtLoginIDSave at (503, 325)
        if (Title["BtLoginIDSave"]) {
            buttons[Buttons::BtLoginIDSave] = std::make_unique<MapleButton>(Title["BtLoginIDSave"], Point<int16_t>(296, 335));
        }

        // BtLoginIDLost at (580, 325)
        if (Title["BtLoginIDLost"]) {
            buttons[Buttons::BtLoginIDLost] = std::make_unique<MapleButton>(Title["BtLoginIDLost"], Point<int16_t>(368, 335));
        }

        // BtPasswdLost at (645, 325)
        if (Title["BtPasswdLost"]) {
            buttons[Buttons::BtPasswdLost] = std::make_unique<MapleButton>(Title["BtPasswdLost"], Point<int16_t>(440, 335));
        }

        // BtNew at (485, 380)
        if (Title["BtNew"]) {
            buttons[Buttons::BtNew] = std::make_unique<MapleButton>(Title["BtNew"], Point<int16_t>(291, 357));
        }

        // BtHomePage at (563, 380)
        if (Title["BtHomePage"]) {
            buttons[Buttons::BtHomePage] = std::make_unique<MapleButton>(Title["BtHomePage"], Point<int16_t>(363, 357));
        }

        // BtQuit at (641, 380)
        if (Title["BtQuit"]) {
            buttons[Buttons::BtQuit] = std::make_unique<MapleButton>(Title["BtQuit"], Point<int16_t>(435, 357));
        }

        // === TEXT FIELDS ===
        // ID field - text entry inside the ID box at (492, 267) -- Adjusted to fit new box pos
        Point<int16_t> id_pos(303, 281);
        Point<int16_t> id_dim(150, 20); // Slightly smaller dim to prevent spill
        account_src_dim = id_dim;
        account = Textfield(Text::Font::A13M, Text::Alignment::LEFT, Color::Name::WHITE,
            Rectangle<int16_t>(id_pos, id_pos + id_dim), TEXTFIELD_LIMIT);

        account.set_key_callback(KeyAction::Id::TAB, [&] {
            account.set_state(Textfield::State::NORMAL);
            password.set_state(Textfield::State::FOCUSED);
            });

        account.set_enter_callback([&] (std::string msg) { login(); });

        // PW field - text entry inside the PW box at (492, 296) -- Adjusted to fit new box pos
        Point<int16_t> pw_pos(303, 307);
        Point<int16_t> pw_dim(150, 20);
        password_src_dim = pw_dim;
        password = Textfield(Text::Font::A13M, Text::Alignment::LEFT, Color::Name::WHITE,
            Rectangle<int16_t>(pw_pos, pw_pos + pw_dim), TEXTFIELD_LIMIT);

        password.set_key_callback(KeyAction::Id::TAB, [&] {
            account.set_state(Textfield::State::FOCUSED);
            password.set_state(Textfield::State::NORMAL);
            });

        password.set_enter_callback([&] (std::string msg) { login(); });
        password.set_cryptchar('*');

        
        
        

        saveid = Setting<SaveLogin>::get().load();

        if (saveid) {
            account.change_text(Setting<DefaultAccount>::get().load());
            password.set_state(Textfield::State::FOCUSED);
        }
        else {
            account.change_text("admin");
            password.change_text("admin");
            account.set_state(Textfield::State::FOCUSED);
        }

        perform_auto_login = Configuration::get().get_auto_login();
    }

    UILogin::~UILogin() {}

    void UILogin::draw(float alpha) const
    {
        draw_sprites(alpha);

        // Draw frame stretched to fill 800x600
        if (frame.is_valid()) {
            frame.draw(DrawArgument(
                position + frame.get_origin(),   // position = origin
                Point<int16_t>(0, 0),            // center = (0,0) so cx = origin.x
                frame_stretch,                    // stretch (800, 600)
                1.0f, 1.0f, 1.0f, 0.0f           // xscale, yscale, opacity, angle
            ));
        }

        draw_buttons(alpha);

        version.draw(position + version_pos);
        
        // Draw ID/PW field backgrounds only when fields are empty
        if (account.empty()) {
            id_texture.draw(DrawArgument(position + id_texture_pos));
        }
        if (password.empty()) {
            pw_texture.draw(DrawArgument(position + pw_texture_pos));
        }
        
        account.draw(position);
        password.draw(position);

        if (showLoginHelpText) {
            loginButtonMissingText.draw(position + Point<int16_t>(370, 320));
        }

        // Checkbox at (489, 329) -- Adjusted to match Texture logic
        check[saveid].draw(position + Point<int16_t>(284, 335));

        if (UI::get().has_capslocks() && password.get_state() == Textfield::State::FOCUSED) {
            capslock.draw(position + Point<int16_t>(291, 320));
        }
    }

    void UILogin::update()
    {
        UIElement::update();
        account.update();
        password.update();

        if (perform_auto_login) {
            perform_auto_login = false;
            std::string auto_account = Configuration::get().get_auto_acc();
            std::string auto_password = Configuration::get().get_auto_pass();
            UI::get().emplace<UILoginWait>([] () {});
            LoginPacket(auto_account, auto_password).dispatch();
        }
    }

    void UILogin::login()
    {
        account.set_state(Textfield::State::DISABLED);
        password.set_state(Textfield::State::DISABLED);

        std::string account_text = account.get_text();
        std::string password_text = password.get_text();

        std::function<void()> okhandler = [&, password_text] () {
            account.set_state(Textfield::State::NORMAL);
            password.set_state(Textfield::State::NORMAL);
            if (!password_text.empty())
                password.set_state(Textfield::State::FOCUSED);
            else
                account.set_state(Textfield::State::FOCUSED);
            };

        if (account_text.empty()) {
            UI::get().emplace<UILoginNotice>(UILoginNotice::Message::NOT_REGISTERED, okhandler);
            return;
        }

        if (password_text.length() <= 4) {
            UI::get().emplace<UILoginNotice>(UILoginNotice::Message::WRONG_PASSWORD, okhandler);
            return;
        }

        UI::get().emplace<UILoginWait>(okhandler);
        auto loginwait = UI::get().get_element<UILoginWait>();

        if (loginwait && loginwait->is_active()) {
            LoginPacket(account_text, password_text).dispatch();
        }
    }

    void UILogin::open_url(uint16_t id)
    {
        std::string url;
        switch (id) {
        case Buttons::BtNew: url = Configuration::get().get_joinlink(); break;
        case Buttons::BtHomePage: url = Configuration::get().get_website(); break;
        case Buttons::BtPasswdLost: url = Configuration::get().get_findpass(); break;
        case Buttons::BtLoginIDLost: url = Configuration::get().get_findid(); break;
        default: return;
        }
        ShellExecuteA(NULL, "open", url.c_str(), NULL, NULL, SW_SHOWNORMAL);
    }

    Button::State UILogin::button_pressed(uint16_t id)
    {
        switch (id) {
        case Buttons::BtLogin:
            login();
            return Button::State::NORMAL;
        case Buttons::BtNew:
        case Buttons::BtHomePage:
        case Buttons::BtPasswdLost:
        case Buttons::BtLoginIDLost:
            open_url(id);
            return Button::State::NORMAL;
        case Buttons::BtLoginIDSave:
            saveid = !saveid;
            Setting<SaveLogin>::get().save(saveid);
            return Button::State::MOUSEOVER;
        case Buttons::BtQuit:
            UI::get().quit();
            return Button::State::PRESSED;
        default:
            return Button::State::DISABLED;
        }
    }

    Cursor::State UILogin::send_cursor(bool clicked, Point<int16_t> cursorpos)
    {
        if (Cursor::State new_state = account.send_cursor(cursorpos, clicked))
            return new_state;
        if (Cursor::State new_state = password.send_cursor(cursorpos, clicked))
            return new_state;
        return UIElement::send_cursor(clicked, cursorpos);
    }

    UIElement::Type UILogin::get_type() const { return TYPE; }
}
