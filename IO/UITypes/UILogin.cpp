//////////////////////////////////////////////////////////////////////////////////
//	This file is part of the continued Journey MMORPG client					//
//	Copyright (C) 2015-2019  Daniel Allendorf, Ryan Payton						//
//																				//
//	This program is free software: you can redistribute it and/or modify		//
//	it under the terms of the GNU Affero General Public License as published by	//
//	the Free Software Foundation, either version 3 of the License, or			//
//	(at your option) any later version.											//
//																				//
//	This program is distributed in the hope that it will be useful,				//
//	but WITHOUT ANY WARRANTY; without even the implied warranty of				//
//	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the				//
//	GNU Affero General Public License for more details.							//
//																				//
//	You should have received a copy of the GNU Affero General Public License	//
//	along with this program.  If not, see <https://www.gnu.org/licenses/>.		//
//////////////////////////////////////////////////////////////////////////////////
#include "UILogin.h"

#include "UILoginNotice.h"
#include "UILoginWait.h"

#include "../UI.h"
#include "../Window.h"

#include "../Components/MapleButton.h"
#include "../Components/TwoSpriteButton.h"

#include "../../Audio/Audio.h"

#include "../../Net/Packets/LoginPackets.h"
#include "../../Util/AssetRegistry.h"
#include "../../Util/Assets.h"
#include "../../Util/V83UIAssets.h"

#include <windows.h>
#include <iostream>
#include "../../Util/Misc.h"

// Forward declaration for NX exploration
extern void quick_nx_explore();

#ifdef USE_NX
#include <nlnx/nx.hpp>
#endif

namespace ms
{
	UILogin::UILogin() : UIElement(Point<int16_t>(0, 0), Point<int16_t>(1024, 768)), title_pos(Point<int16_t>(344, 246)), nexon(false), showLoginHelpText(false)
	{
		// UILogin constructor
		LOG(LOG_DEBUG, "[UILogin] Constructor starting...");
		
		// Run NX exploration for debugging
		// quick_nx_explore();
		
		std::string LoginMusicNewtro = Configuration::get().get_login_music_newtro();

		Music(LoginMusicNewtro).play();

		std::string version_text = Configuration::get().get_version();
		version = Text(Text::Font::A12B, Text::Alignment::LEFT, Color::Name::LEMONGRASS, "Ver. " + version_text);
		
		// Initialize login help text (will only be shown if buttons are missing)
		loginButtonMissingText = Text(Text::Font::A12B, Text::Alignment::CENTER, Color::Name::YELLOW, "Press ENTER to login");
		
		// Use AssetRegistry for version position
		version_pos = GetAsset(AssetID::UI_Login_Version_Position);

		// Get base login nodes through AssetRegistry
		nl::node Login = nl::nx::UI["Login.img"];  // Keep this for backward compatibility during transition

		nl::node Title = Login["Title"];  // CORRECTED: v83 uses "Title" not "Title_new"
		
		
		// Use AssetRegistry for UI elements
		capslock = GetAsset(AssetID::UI_Login_Capslock_Warning);
		check[false] = GetAsset(AssetID::UI_Login_Checkbox_Unchecked);
		check[true] = GetAsset(AssetID::UI_Login_Checkbox_Checked);

		// Use AssetRegistry for login background
		
		// Use V83 compatibility layer for background
		LOG(LOG_DEBUG, "[UILogin] Loading login background...");
		nl::node login_bg = V83UIAssets::getLoginBackground();
		if (login_bg) {
			LOG(LOG_DEBUG, "[UILogin] Login background found, creating sprites...");
			sprites.emplace_back(login_bg, Point<int16_t>(0, 0));
			// Title area might use same background in v83
			sprites.emplace_back(login_bg, title_pos);
			LOG(LOG_DEBUG, "[UILogin] Login background sprites created successfully");
		} else {
			LOG(LOG_ERROR, "[UILogin] Failed to load login background");
			// Use fallback from AssetRegistry
			AssetRegistry& registry = AssetRegistry::get();
			nl::node fallback_bg = registry.resolve(AssetID::UI_Login_Background);
			if (fallback_bg) {
				LOG(LOG_DEBUG, "[UILogin] Using fallback background from AssetRegistry");
				sprites.emplace_back(fallback_bg, Point<int16_t>(0, 0));
				sprites.emplace_back(fallback_bg, title_pos);
			}
		}

		// Get tab nodes - v83 might not have tabs
		// Login already defined above
		nl::node TabD = Login["TabD"];
		nl::node TabE = Login["TabE"];
		
		if (!TabD || !TabE) {
			// v83 fallback - no tabs
			LOG(LOG_DEBUG, "[UILogin] No tab nodes found (v83 mode)");
		}

		// Create buttons using V83 compatibility
		bool hasLoginButton = false;
		nl::node btnLogin = V83UIAssets::getLoginButton("Login");
		if (btnLogin) {
			buttons[Buttons::BtLogin] = std::make_unique<MapleButton>(btnLogin, title_pos);
			hasLoginButton = true;
		} else {
			LOG(LOG_ERROR, "[UILogin] Login button not found - login via Enter key only");
			// Show the help text since login button is missing
			showLoginHelpText = true;
		}
		
		nl::node btnNew = V83UIAssets::getLoginButton("New");
		if (btnNew) {
			buttons[Buttons::BtNew] = std::make_unique<MapleButton>(btnNew, title_pos);
		}
		
		nl::node btnQuit = V83UIAssets::getLoginButton("Quit");
		if (btnQuit) {
			buttons[Buttons::BtQuit] = std::make_unique<MapleButton>(btnQuit, title_pos);
		} else {
			// No quit button - user can use Alt+F4 or close window
		}
		
		// v83 might not have these buttons
		nl::node btnHomePage = V83UIAssets::getLoginButton("HomePage");
		if (btnHomePage) {
			buttons[Buttons::BtHomePage] = std::make_unique<MapleButton>(btnHomePage, title_pos);
		}
		
		// Email/Password buttons might not exist in v83
		nl::node btnPasswdLost = V83UIAssets::getLoginButton("PasswdLost");
		if (btnPasswdLost) {
			buttons[Buttons::BtPasswdLost] = std::make_unique<MapleButton>(btnPasswdLost, title_pos);
		}
		
		nl::node btnEmailLost = V83UIAssets::getLoginButton("EmailLost");
		if (btnEmailLost) {
			buttons[Buttons::BtEmailLost] = std::make_unique<MapleButton>(btnEmailLost, title_pos);
		}
		
		nl::node btnEmailSave = V83UIAssets::getLoginButton("EmailSave");
		if (btnEmailSave) {
			buttons[Buttons::BtEmailSave] = std::make_unique<MapleButton>(btnEmailSave, title_pos);
		}
		
		// Tab buttons - only create if tabs exist
		if (TabD && TabE) {
			buttons[Buttons::BtMapleID] = std::make_unique<TwoSpriteButton>(TabD["0"], TabE["0"], Point<int16_t>(344, 246));
			buttons[Buttons::BtNexonID] = std::make_unique<TwoSpriteButton>(TabD["1"], TabE["1"], Point<int16_t>(344, 246));
		}

		if (nexon)
		{
			if (buttons[Buttons::BtNexonID]) buttons[Buttons::BtNexonID]->set_state(Button::State::PRESSED);
			if (buttons[Buttons::BtMapleID]) buttons[Buttons::BtMapleID]->set_state(Button::State::NORMAL);
		}
		else
		{
			if (buttons[Buttons::BtNexonID]) buttons[Buttons::BtNexonID]->set_state(Button::State::NORMAL);
			if (buttons[Buttons::BtMapleID]) buttons[Buttons::BtMapleID]->set_state(Button::State::PRESSED);
		}

		background = ColorBox(dimension.x(), dimension.y(), Color::Name::BLACK, 1.0f);

		Point<int16_t> textfield_pos = title_pos + Point<int16_t>(27, 69);

#pragma region Account
		// v83 uses common frame for all text fields
		nl::node frame_node = Login["Common"]["frame"];
		Texture account_src = V83UIAssets::getTexture(frame_node, "Account Field");
		if (!account_src.is_valid()) {
			// Fallback to asset registry
			account_src = Texture(GetAsset(AssetID::UI_Login_Field_MapleID));
		}
		account_src_dim = account_src.get_dimensions();

		account = Textfield(Text::Font::A13M, Text::Alignment::LEFT, Color::Name::JAMBALAYA, Rectangle<int16_t>(textfield_pos, textfield_pos + account_src_dim), TEXTFIELD_LIMIT);

		account.set_key_callback
		(
			KeyAction::Id::TAB, [&]
			{
				account.set_state(Textfield::State::NORMAL);
				password.set_state(Textfield::State::FOCUSED);
			}
		);

		account.set_enter_callback
		(
			[&](std::string msg)
			{
				login();
			}
		);

		account_bg[false] = account_src;
		// v83 uses same frame for both MapleID and NexonID
		account_bg[true] = account_src;
#pragma endregion

#pragma region Password
		textfield_pos.shift_y(account_src_dim.y() + 1);

		// v83 uses same frame for password field
		Texture password_src = account_src;
		password_src_dim = password_src.get_dimensions();

		password = Textfield(Text::Font::A13M, Text::Alignment::LEFT, Color::Name::JAMBALAYA, Rectangle<int16_t>(textfield_pos, textfield_pos + password_src_dim), TEXTFIELD_LIMIT);

		password.set_key_callback
		(
			KeyAction::Id::TAB, [&]
			{
				account.set_state(Textfield::State::FOCUSED);
				password.set_state(Textfield::State::NORMAL);
			}
		);

		password.set_enter_callback
		(
			[&](std::string msg)
			{
				login();
			}
		);

		password.set_cryptchar('*');
		password_bg = password_src;
#pragma endregion

		saveid = Setting<SaveLogin>::get().load();

		if (saveid)
		{
			account.change_text(Setting<DefaultAccount>::get().load());
			password.set_state(Textfield::State::FOCUSED);
		}
		else
		{
			account.change_text("admin");
			password.change_text("admin");
			account.set_state(Textfield::State::FOCUSED);
		}

		// Initialize auto-login flag for later use
		perform_auto_login = Configuration::get().get_auto_login();
		if (perform_auto_login)
		{
			LOG(LOG_DEBUG, "[UILogin] Auto-login enabled, will perform after construction");
		}
		else
		{
			LOG(LOG_DEBUG, "[UILogin] Auto-login disabled");
		}
	}
	
	UILogin::~UILogin()
	{
		LOG(LOG_DEBUG, "[UILogin] Destructor called");
	}

	void UILogin::draw(float alpha) const
	{
		// Drawing login screen
		background.draw(position + Point<int16_t>(0, 7));

		// Draw sprites normally
		draw_sprites(alpha);

		// Draw buttons
		draw_buttons(alpha);

		version.draw(position + version_pos - Point<int16_t>(0, 5));
		account.draw(position + Point<int16_t>(5, 10));
		password.draw(position + Point<int16_t>(5, 13));

		if (account.get_state() == Textfield::State::NORMAL && account.empty())
			account_bg[nexon].draw(position + title_pos);

		if (password.get_state() == Textfield::State::NORMAL && password.empty())
			password_bg.draw(position + title_pos);
			
		// Draw missing button text if needed
		if (showLoginHelpText) {
			loginButtonMissingText.draw(position + Point<int16_t>(400, 380));
		}

		bool has_capslocks = UI::get().has_capslocks();

		check[saveid].draw(position + title_pos);

		if (has_capslocks && account.get_state() == Textfield::State::FOCUSED)
			capslock.draw(position + title_pos - Point<int16_t>(0, account_src_dim.y()));

		if (has_capslocks && password.get_state() == Textfield::State::FOCUSED)
			capslock.draw(position + title_pos + Point<int16_t>(password_src_dim.x() - account_src_dim.x(), 0));
	}

	void UILogin::update()
	{
		UIElement::update();

		account.update();
		password.update();
		
		// Perform auto-login on first update after construction
		if (perform_auto_login)
		{
			perform_auto_login = false;  // Only do this once
			
			LOG(LOG_DEBUG, "[UILogin] Performing auto-login...");
			
			std::string auto_account = Configuration::get().get_auto_acc();
			std::string auto_password = Configuration::get().get_auto_pass();
			
			LOG(LOG_DEBUG, "[UILogin] Auto-login account: " + auto_account);
			LOG(LOG_DEBUG, "[UILogin] Creating UILoginWait for auto-login...");
			
			// Create UILoginWait for auto-login - needed for LoginResultHandler
			UI::get().emplace<UILoginWait>([]() {
				LOG(LOG_DEBUG, "[UILogin] Auto-login UILoginWait closed");
			});
			
			LOG(LOG_DEBUG, "[UILogin] Dispatching auto-login packet...");
			LoginPacket(auto_account, auto_password).dispatch();
		}
	}

	void UILogin::login()
	{
		account.set_state(Textfield::State::DISABLED);
		password.set_state(Textfield::State::DISABLED);

		std::string account_text = account.get_text();
		std::string password_text = password.get_text();

		std::function<void()> okhandler = [&, password_text]()
		{
			account.set_state(Textfield::State::NORMAL);
			password.set_state(Textfield::State::NORMAL);

			if (!password_text.empty())
				password.set_state(Textfield::State::FOCUSED);
			else
				account.set_state(Textfield::State::FOCUSED);
		};

		if (account_text.empty())
		{
			UI::get().emplace<UILoginNotice>(UILoginNotice::Message::NOT_REGISTERED, okhandler);
			return;
		}

		if (password_text.length() <= 4)
		{
			UI::get().emplace<UILoginNotice>(UILoginNotice::Message::WRONG_PASSWORD, okhandler);
			return;
		}

		UI::get().emplace<UILoginWait>(okhandler);

		auto loginwait = UI::get().get_element<UILoginWait>();

		if (loginwait && loginwait->is_active())
		{
			// TODO: Implement login with email
			if (nexon)
				LoginEmailPacket(account_text, password_text).dispatch();
			else
				LoginPacket(account_text, password_text).dispatch();
		}
	}

	void UILogin::open_url(uint16_t id)
	{
		std::string url;

		switch (id)
		{
			case Buttons::BtNew:
				url = Configuration::get().get_joinlink();
				break;
			case Buttons::BtHomePage:
				url = Configuration::get().get_website();
				break;
			case Buttons::BtPasswdLost:
				url = Configuration::get().get_findpass();
				break;
			case Buttons::BtEmailLost:
				url = Configuration::get().get_findid();
				break;
			default:
				return;
		}

		ShellExecuteA(NULL, "open", url.c_str(), NULL, NULL, SW_SHOWNORMAL);
	}

	Button::State UILogin::button_pressed(uint16_t id)
	{
		switch (id)
		{
			case Buttons::BtLogin:
			{
				login();

				return Button::State::NORMAL;
			}
			case Buttons::BtNew:
			case Buttons::BtHomePage:
			case Buttons::BtPasswdLost:
			case Buttons::BtEmailLost:
			{
				open_url(id);

				return Button::State::NORMAL;
			}
			case Buttons::BtEmailSave:
			{
				saveid = !saveid;

				Setting<SaveLogin>::get().save(saveid);

				return Button::State::MOUSEOVER;
			}
			case Buttons::BtQuit:
			{
				UI::get().quit();

				return Button::State::PRESSED;
			}
			case Buttons::BtMapleID:
			{
				nexon = false;

				if (buttons[Buttons::BtNexonID]) buttons[Buttons::BtNexonID]->set_state(Button::State::NORMAL);

				account.change_text("");
				password.change_text("");

				account.set_limit(TEXTFIELD_LIMIT);

				return Button::State::PRESSED;
			}
			case Buttons::BtNexonID:
			{
				nexon = true;

				if (buttons[Buttons::BtMapleID]) buttons[Buttons::BtMapleID]->set_state(Button::State::NORMAL);

				account.change_text("");
				password.change_text("");
				
				account.set_limit(72);

				return Button::State::PRESSED;
			}
			default:
			{
				return Button::State::DISABLED;
			}
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

	UIElement::Type UILogin::get_type() const
	{
		return TYPE;
	}
}