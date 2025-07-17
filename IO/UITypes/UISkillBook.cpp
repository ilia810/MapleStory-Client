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
#include "UISkillBook.h"

#include "../UI.h"

#include "../Components/MapleButton.h"

#include "../../Character/SkillId.h"
#include "../../Data/JobData.h"
#include "../../Data/SkillData.h"
#include "../../Gameplay/Stage.h"

#include "../../Net/Packets/PlayerPackets.h"

#include "../../Util/Misc.h"

#ifdef USE_NX
#include <nlnx/nx.hpp>
#endif

namespace ms
{
	// Helper function to safely convert string to integer
	int32_t safe_stoi(const std::string& str, int32_t default_value = 0) {
		try {
			if (str.empty()) return default_value;
			return std::stoi(str);
		} catch (const std::exception&) {
			return default_value;
		}
	}

	UISkillBook::SkillIcon::SkillIcon(int32_t id) : skill_id(id) {}

	void UISkillBook::SkillIcon::drop_on_bindings(Point<int16_t> cursorposition, bool remove) const
	{
		auto keyconfig = UI::get().get_element<UIKeyConfig>();
		Keyboard::Mapping mapping = Keyboard::Mapping(KeyType::SKILL, skill_id);

		if (remove)
			keyconfig->unstage_mapping(mapping);
		else
			keyconfig->stage_mapping(cursorposition, mapping);
	}

	Icon::IconType UISkillBook::SkillIcon::get_type()
	{
		return Icon::IconType::SKILL;
	}

	UISkillBook::SkillDisplayMeta::SkillDisplayMeta(int32_t i, int32_t l) : id(i), level(l)
	{
		const SkillData& data = SkillData::get(id);

		Texture ntx = data.get_icon(SkillData::Icon::NORMAL);
		Texture dtx = data.get_icon(SkillData::Icon::DISABLED);
		Texture motx = data.get_icon(SkillData::Icon::MOUSEOVER);
		icon = std::make_unique<StatefulIcon>(std::make_unique<SkillIcon>(id), ntx, dtx, motx);

		std::string namestr = data.get_name();
		std::string levelstr = std::to_string(level);

		name_text = Text(Text::Font::A11M, Text::Alignment::LEFT, Color::Name::EMPEROR, namestr);
		level_text = Text(Text::Font::A11M, Text::Alignment::LEFT, Color::Name::EMPEROR, levelstr);

		constexpr uint16_t MAX_NAME_WIDTH = 97;
		size_t overhang = 3;

		while (name_text.width() > MAX_NAME_WIDTH)
		{
			namestr.replace(namestr.end() - overhang, namestr.end(), "..");
			overhang += 1;

			name_text.change_text(namestr);
		}
	}

	void UISkillBook::SkillDisplayMeta::draw(const DrawArgument& args) const
	{
		icon->draw(args.getpos());
		name_text.draw(args + Point<int16_t>(38, -5));
		level_text.draw(args + Point<int16_t>(38, 13));
	}

	int32_t UISkillBook::SkillDisplayMeta::get_id() const
	{
		return id;
	}

	int32_t UISkillBook::SkillDisplayMeta::get_level() const
	{
		return level;
	}

	StatefulIcon* UISkillBook::SkillDisplayMeta::get_icon() const
	{
		return icon.get();
	}

	UISkillBook::UISkillBook(const CharStats& in_stats, const SkillBook& in_skillbook) : UIDragElement<PosSKILL>(), stats(in_stats), skillbook(in_skillbook), grabbing(false), tab(0), macro_enabled(false), sp_enabled(false)
	{
		LOG(LOG_DEBUG, "[UISkillBook] Constructor START");
		
		// Use UIWindow.img/Skill structure
		nl::node Skill = nl::nx::UI["UIWindow.img"]["Skill"];
		
		if (!Skill) {
			// If no Skills window assets found, create minimal window
			LOG(LOG_ERROR, "[UISkillBook] UIWindow.img/Skill node not found!");
			return;
		}
		
		LOG(LOG_DEBUG, "[UISkillBook] Found Skill node, proceeding with initialization");
		
		// Load main background
		LOG(LOG_DEBUG, "[UISkillBook] Loading main background");
		nl::node ui_backgrnd = Skill["backgrnd"];
		if (ui_backgrnd) {
			bg_dimensions = Texture(ui_backgrnd).get_dimensions();
			LOG(LOG_DEBUG, "[UISkillBook] Background loaded, dimensions: " << bg_dimensions.x() << "x" << bg_dimensions.y());
		} else {
			bg_dimensions = Point<int16_t>(400, 300); // Default size
			LOG(LOG_DEBUG, "[UISkillBook] No background found, using default size");
		}

		// Load skill display elements
		LOG(LOG_DEBUG, "[UISkillBook] Loading skill display elements");
		skilld = Skill["skill0"];
		skille = Skill["skill1"];
		
		nl::node skillBlankNode = Skill["skillBlank"];
		if (skillBlankNode) {
			skillb = skillBlankNode;
			LOG(LOG_DEBUG, "[UISkillBook] Found skillBlank node");
		} else {
			skillb = Skill["skill0"]; // Fallback if skillBlank doesn't exist
			LOG(LOG_DEBUG, "[UISkillBook] No skillBlank node, using skill0 as fallback");
		}
		
		// Validate the texture by checking if the node exists
		if (!skillBlankNode && !Skill["skill0"]) {
			LOG(LOG_ERROR, "[UISkillBook] No valid skill background texture found!");
		}
		
		line = Skill["line"];
		LOG(LOG_DEBUG, "[UISkillBook] Skill display elements loaded");

		// Create buttons - these are in the Skill node directly, not in a main subnode
		LOG(LOG_DEBUG, "[UISkillBook] Creating buttons");
		nl::node btHyper = Skill["BtHyper"];
		if (btHyper) {
			buttons[Buttons::BT_HYPER] = std::make_unique<MapleButton>(btHyper);
			buttons[Buttons::BT_HYPER]->set_state(Button::State::DISABLED);
			LOG(LOG_DEBUG, "[UISkillBook] Created BT_HYPER button");
		}
		
		nl::node btGuildSkill = Skill["BtGuildSkill"];
		if (btGuildSkill) {
			buttons[Buttons::BT_GUILDSKILL] = std::make_unique<MapleButton>(btGuildSkill);
			buttons[Buttons::BT_GUILDSKILL]->set_state(Button::State::DISABLED);
			LOG(LOG_DEBUG, "[UISkillBook] Created BT_GUILDSKILL button");
		}
		
		nl::node btRide = Skill["BtRide"];
		if (btRide) {
			buttons[Buttons::BT_RIDE] = std::make_unique<MapleButton>(btRide);
			buttons[Buttons::BT_RIDE]->set_state(Button::State::DISABLED);
			LOG(LOG_DEBUG, "[UISkillBook] Created BT_RIDE button");
		}
		
		nl::node btMacro = Skill["BtMacro"];
		if (btMacro) {
			buttons[Buttons::BT_MACRO] = std::make_unique<MapleButton>(btMacro);
			LOG(LOG_DEBUG, "[UISkillBook] Created BT_MACRO button");
		}

		// v92: Use UIWindow.img structure directly
		nl::node skillPoint = Skill["skillPoint"];

		if (skillPoint) {
			sp_backgrnd = skillPoint["backgrnd"];
			sp_backgrnd2 = skillPoint["backgrnd2"];
			sp_backgrnd3 = skillPoint["backgrnd3"];

			// Create skill point buttons only if nodes exist
			if (skillPoint["BtCancle"]) buttons[Buttons::BT_CANCLE] = std::make_unique<MapleButton>(skillPoint["BtCancle"], Point<int16_t>(bg_dimensions.x(), 0));
			if (skillPoint["BtOkay"]) buttons[Buttons::BT_OKAY] = std::make_unique<MapleButton>(skillPoint["BtOkay"], Point<int16_t>(bg_dimensions.x(), 0));
			if (skillPoint["BtSpDown"]) {
				buttons[Buttons::BT_SPDOWN] = std::make_unique<MapleButton>(skillPoint["BtSpDown"], Point<int16_t>(bg_dimensions.x(), 0));
				buttons[Buttons::BT_SPDOWN]->set_state(Button::State::DISABLED);
			}
			if (skillPoint["BtSpMax"]) buttons[Buttons::BT_SPMAX] = std::make_unique<MapleButton>(skillPoint["BtSpMax"], Point<int16_t>(bg_dimensions.x(), 0));
			if (skillPoint["BtSpUp"]) buttons[Buttons::BT_SPUP] = std::make_unique<MapleButton>(skillPoint["BtSpUp"], Point<int16_t>(bg_dimensions.x(), 0));

			if (skillPoint["num"]) {
				sp_before = Charset(skillPoint["num"], Charset::Alignment::RIGHT);
				sp_after = Charset(skillPoint["num"], Charset::Alignment::RIGHT);
			}
		}

		sp_used = Text(Text::Font::A12B, Text::Alignment::RIGHT, Color::Name::WHITE);
		sp_remaining = Text(Text::Font::A12B, Text::Alignment::LEFT, Color::Name::SUPERNOVA);
		sp_name = Text(Text::Font::A12B, Text::Alignment::CENTER, Color::Name::WHITE);

		// Add background sprite
		if (ui_backgrnd) sprites.emplace_back(ui_backgrnd, Point<int16_t>(1, 0));

		nl::node macro = Skill["macro"];

		macro_backgrnd = macro["backgrnd"];
		macro_backgrnd2 = macro["backgrnd2"];
		macro_backgrnd3 = macro["backgrnd3"];

		if (macro["BtOK"]) {
			buttons[Buttons::BT_MACRO_OK] = std::make_unique<MapleButton>(macro["BtOK"], Point<int16_t>(bg_dimensions.x(), 0));
			buttons[Buttons::BT_MACRO_OK]->set_state(Button::State::DISABLED);
		}

		nl::node close = nl::nx::UI["Basic.img"]["BtClose3"];

		if (close) {
			buttons[Buttons::BT_CLOSE] = std::make_unique<MapleButton>(close, Point<int16_t>(bg_dimensions.x() - 23, 6));
		}

		LOG(LOG_DEBUG, "[UISkillBook] Creating tab buttons");
		nl::node Tab = Skill["Tab"];
		if (Tab) {
			nl::node enabled = Tab["enabled"];
			nl::node disabled = Tab["disabled"];

			for (uint16_t i = Buttons::BT_TAB0; i <= Buttons::BT_TAB4; ++i)
			{
				uint16_t tabid = i - Buttons::BT_TAB0;
				LOG(LOG_DEBUG, "[UISkillBook] Creating tab button " << tabid);
				if (disabled[tabid] && enabled[tabid]) {
					buttons[i] = std::make_unique<TwoSpriteButton>(disabled[tabid], enabled[tabid]);
					LOG(LOG_DEBUG, "[UISkillBook] Created tab button " << tabid);
				}
			}
		} else {
			LOG(LOG_WARN, "[UISkillBook] No Tab structure found, creating null placeholders");
			// If no Tab structure exists, create invisible placeholder buttons
			// This prevents crashes when change_tab is called
			for (uint16_t i = Buttons::BT_TAB0; i <= Buttons::BT_TAB4; ++i)
			{
				// Create buttons that do nothing but prevent null pointer access
				buttons[i] = nullptr;
			}
		}

		uint16_t y_adj = 0;

		// Check if we should use single column layout based on background width
		bool use_single_column = (bg_dimensions.x() < 250);

		for (uint16_t i = Buttons::BT_SPUP0; i <= Buttons::BT_SPUP11; ++i)
		{
			uint16_t x_adj = 0;
			uint16_t spupid = i - Buttons::BT_SPUP0;

			// Only use second column if window is wide enough
			if (!use_single_column && spupid % 2)
				x_adj = ROW_WIDTH;

			Point<int16_t> spup_position = SKILL_OFFSET + Point<int16_t>(124 + x_adj, 20 + y_adj);
			nl::node btSpUp = Skill["BtSpUp"];
			if (btSpUp) {
				buttons[i] = std::make_unique<MapleButton>(btSpUp, spup_position);
			}

			// Update y position
			if (use_single_column) {
				// Single column: always move down
				y_adj += ROW_HEIGHT;
			} else {
				// Two columns: move down every other button
				if (spupid % 2)
					y_adj += ROW_HEIGHT;
			}
		}

		booktext = Text(Text::Font::A11M, Text::Alignment::CENTER, Color::Name::WHITE, "", 150);
		splabel = Text(Text::Font::A12M, Text::Alignment::RIGHT, Color::Name::BLACK);

		LOG(LOG_DEBUG, "[UISkillBook] Creating slider");
		// Initialize with a simple type first to see if it works
		try {
			slider = Slider(
				Slider::Type::LINE_CYAN, Range<int16_t>(93, 317), 295, ROWS, 1,
				[&](bool upwards)
				{
					int16_t shift = upwards ? -1 : 1;
					bool above = offset + shift >= 0;
					bool below = offset + 4 + shift <= skillcount;

					if (above && below)
						change_offset(offset + shift);
				}
			);
			LOG(LOG_DEBUG, "[UISkillBook] Slider created successfully");
		} catch (...) {
			LOG(LOG_ERROR, "[UISkillBook] Failed to create slider");
		}

		LOG(LOG_DEBUG, "[UISkillBook] About to call change_job()");
		uint16_t job_id = stats.get_stat(MapleStat::Id::JOB);
		LOG(LOG_DEBUG, "[UISkillBook] Job ID: " << job_id);
		change_job(job_id);
		LOG(LOG_DEBUG, "[UISkillBook] change_job() completed");

		set_macro(false);
		set_skillpoint(false);

		dimension = bg_dimensions;
		dragarea = Point<int16_t>(dimension.x(), 20);
		
		LOG(LOG_DEBUG, "[UISkillBook] Constructor END");
	}

	void UISkillBook::draw(float alpha) const
	{
		static int draw_count = 0;
		draw_count++;
		LOG(LOG_DEBUG, "[UISkillBook::draw] START (call #" << draw_count << ")");
		
		UIElement::draw_sprites(alpha);
		LOG(LOG_DEBUG, "[UISkillBook::draw] Sprites drawn");

		bookicon.draw(position + Point<int16_t>(11, 85));
		LOG(LOG_DEBUG, "[UISkillBook::draw] Book icon drawn");
		
		// Adjust text positions for narrow window
		if (bg_dimensions.x() < 250) {
			// v92 narrow window positioning
			booktext.draw(position + Point<int16_t>(bg_dimensions.x() / 2, 59));
			splabel.draw(position + Point<int16_t>(bg_dimensions.x() - 20, 23));
		} else {
			// Original wider window positioning
			booktext.draw(position + Point<int16_t>(173, 59));
			splabel.draw(position + Point<int16_t>(304, 23));
		}
		LOG(LOG_DEBUG, "[UISkillBook::draw] Text drawn");

		Point<int16_t> skill_position_l = position + SKILL_OFFSET + Point<int16_t>(-1, 0);
		Point<int16_t> skill_position_r = position + SKILL_OFFSET + Point<int16_t>(-1 + ROW_WIDTH, 0);

		// Draw skills in the list
		
		// For v92, use single column layout since window is only 175px wide
		bool use_single_column = (bg_dimensions.x() < 250);
		
		// Calculate how many rows can fit in the window
		int16_t visible_rows = ROWS;
		if (use_single_column) {
			// Estimate based on window height minus header space
			visible_rows = (bg_dimensions.y() - 100) / ROW_HEIGHT;
			if (visible_rows > ROWS) visible_rows = ROWS;
			if (visible_rows < 1) visible_rows = 1;
			LOG(LOG_DEBUG, "[UISkillBook::draw] Limiting to " << visible_rows << " visible rows");
		}
		
		for (size_t i = 0; i < visible_rows; i++)
		{
			Point<int16_t> pos = skill_position_l;

			// Only use right column if window is wide enough
			if (!use_single_column && i % 2)
				pos = skill_position_r;

			if (i < skills.size())
			{
				if (check_required(skills[i].get_id()))
				{
					skille.draw(pos);
				}
				else
				{
					skilld.draw(pos);
					skills[i].get_icon()->set_state(StatefulIcon::State::DISABLED);
				}

				skills[i].draw(pos + SKILL_META_OFFSET);
			}
			else
			{
				// Draw blank skill slot
				skillb.draw(pos);
			}

			if (i < visible_rows - 2)
				line.draw(pos + LINE_OFFSET);

			// Update position for next row
			if (use_single_column) {
				// Single column: always shift down
				skill_position_l.shift_y(ROW_HEIGHT);
			} else {
				// Two columns: shift down every other skill
				if (i % 2)
				{
					skill_position_l.shift_y(ROW_HEIGHT);
					skill_position_r.shift_y(ROW_HEIGHT);
				}
			}
		}

		LOG(LOG_DEBUG, "[UISkillBook::draw] About to draw slider");
		// Only draw slider if there are more skills than can fit on screen
		if (skillcount > ROWS) {
			LOG(LOG_DEBUG, "[UISkillBook::draw] Need slider (skillcount=" << skillcount << " > ROWS=" << ROWS << ")");
			// For now, skip drawing the slider to avoid crashes
			// TODO: Fix slider drawing for v92 client
			LOG(LOG_DEBUG, "[UISkillBook::draw] Slider drawing disabled for v92 compatibility");
		} else {
			LOG(LOG_DEBUG, "[UISkillBook::draw] No slider needed (skillcount=" << skillcount << " <= ROWS=" << ROWS << ")");
		}

		if (macro_enabled)
		{
			Point<int16_t> macro_pos = position + Point<int16_t>(bg_dimensions.x(), 0);

			macro_backgrnd.draw(macro_pos + Point<int16_t>(1, 0));
			macro_backgrnd2.draw(macro_pos);
			macro_backgrnd3.draw(macro_pos);
		}

		if (sp_enabled)
		{
			Point<int16_t> sp_pos = position + Point<int16_t>(bg_dimensions.x(), 0);

			sp_backgrnd.draw(sp_pos);
			sp_backgrnd2.draw(sp_pos);
			sp_backgrnd3.draw(sp_pos);

			Point<int16_t> sp_level_pos = sp_pos + Point<int16_t>(78, 149);

			sp_before.draw(sp_before_text, 12, sp_level_pos);
			sp_after.draw(sp_after_text, 11, sp_level_pos + Point<int16_t>(78, 0));
			sp_used.draw(sp_pos + Point<int16_t>(82, 87));
			sp_remaining.draw(sp_pos + Point<int16_t>(76, 65));
			sp_name.draw(sp_pos + Point<int16_t>(97, 35));
			sp_skill.draw(sp_pos + Point<int16_t>(13, 31));
		}

		LOG(LOG_DEBUG, "[UISkillBook::draw] About to draw buttons");
		UIElement::draw_buttons(alpha);
		
		LOG(LOG_DEBUG, "[UISkillBook::draw] END - draw method complete");
	}

	Button::State UISkillBook::button_pressed(uint16_t id)
	{
		int16_t cur_sp = safe_stoi(splabel.get_text());

		switch (id)
		{
		case Buttons::BT_CLOSE:
			close();
			break;
		case Buttons::BT_MACRO:
			set_macro(!macro_enabled);
			break;
		case Buttons::BT_CANCLE:
			set_skillpoint(false);
			break;
		case Buttons::BT_OKAY:
		{
			int32_t used = safe_stoi(sp_used.get_text());

			while (used > 0)
			{
				spend_sp(sp_id);
				used--;
			}

			change_sp();
			set_skillpoint(false);
		}
		break;
		case Buttons::BT_SPDOWN:
		{
			int32_t used = safe_stoi(sp_used.get_text());
			int32_t sp_after = safe_stoi(sp_after_text);
			int32_t sp_before = safe_stoi(sp_before_text);
			used--;
			sp_after--;

			sp_after_text = std::to_string(sp_after);
			sp_used.change_text(std::to_string(used));
			sp_remaining.change_text(std::to_string(cur_sp - used));

			if (buttons[Buttons::BT_SPUP]) buttons[Buttons::BT_SPUP]->set_state(Button::State::NORMAL);
			if (buttons[Buttons::BT_SPMAX]) buttons[Buttons::BT_SPMAX]->set_state(Button::State::NORMAL);

			if (sp_after - 1 == sp_before)
				return Button::State::DISABLED;

			return Button::State::NORMAL;
		}
		break;
		case Buttons::BT_SPMAX:
		{
			int32_t used = safe_stoi(sp_used.get_text());
			int32_t sp_before = safe_stoi(sp_before_text);
			int32_t sp_touse = sp_masterlevel - sp_before - used;

			used += sp_touse;

			sp_after_text = std::to_string(sp_masterlevel);
			sp_used.change_text(std::to_string(used));
			sp_remaining.change_text(std::to_string(cur_sp - used));

			if (buttons[Buttons::BT_SPUP]) buttons[Buttons::BT_SPUP]->set_state(Button::State::DISABLED);
			if (buttons[Buttons::BT_SPDOWN]) buttons[Buttons::BT_SPDOWN]->set_state(Button::State::NORMAL);

			return Button::State::DISABLED;
		}
		break;
		case Buttons::BT_SPUP:
		{
			int32_t used = safe_stoi(sp_used.get_text());
			int32_t sp_after = safe_stoi(sp_after_text);
			used++;
			sp_after++;

			sp_after_text = std::to_string(sp_after);
			sp_used.change_text(std::to_string(used));
			sp_remaining.change_text(std::to_string(cur_sp - used));

			if (buttons[Buttons::BT_SPDOWN]) buttons[Buttons::BT_SPDOWN]->set_state(Button::State::NORMAL);

			if (sp_after == sp_masterlevel)
			{
				if (buttons[Buttons::BT_SPMAX]) buttons[Buttons::BT_SPMAX]->set_state(Button::State::DISABLED);

				return Button::State::DISABLED;
			}

			return Button::State::NORMAL;
		}
		break;
		case Buttons::BT_TAB0:
		case Buttons::BT_TAB1:
		case Buttons::BT_TAB2:
		case Buttons::BT_TAB3:
		case Buttons::BT_TAB4:
			change_tab(id - Buttons::BT_TAB0);

			return Button::State::PRESSED;
		case Buttons::BT_SPUP0:
		case Buttons::BT_SPUP1:
		case Buttons::BT_SPUP2:
		case Buttons::BT_SPUP3:
		case Buttons::BT_SPUP4:
		case Buttons::BT_SPUP5:
		case Buttons::BT_SPUP6:
		case Buttons::BT_SPUP7:
		case Buttons::BT_SPUP8:
		case Buttons::BT_SPUP9:
		case Buttons::BT_SPUP10:
		case Buttons::BT_SPUP11:
			send_spup(id - Buttons::BT_SPUP0 + offset);
			break;
		case Buttons::BT_HYPER:
		case Buttons::BT_GUILDSKILL:
		case Buttons::BT_RIDE:
		case Buttons::BT_MACRO_OK:
		default:
			break;
		}

		return Button::State::NORMAL;
	}

	void UISkillBook::toggle_active()
	{
		LOG(LOG_DEBUG, "[UISkillBook::toggle_active] Called, is_skillpoint_enabled()=" << is_skillpoint_enabled());
		
		if (!is_skillpoint_enabled())
		{
			LOG(LOG_DEBUG, "[UISkillBook::toggle_active] Calling UIElement::toggle_active()");
			UIElement::toggle_active();
			LOG(LOG_DEBUG, "[UISkillBook::toggle_active] UIElement::toggle_active() completed");

			clear_tooltip();
		}
		
		LOG(LOG_DEBUG, "[UISkillBook::toggle_active] END");
	}

	void UISkillBook::doubleclick(Point<int16_t> cursorpos)
	{
		const SkillDisplayMeta* skill = skill_by_position(cursorpos - position);

		if (skill)
		{
			int32_t skill_id = skill->get_id();
			int32_t skill_level = skillbook.get_level(skill_id);

			if (skill_level > 0)
				Stage::get().get_combat().use_move(skill_id);
		}
	}

	void UISkillBook::remove_cursor()
	{
		UIDragElement::remove_cursor();

		slider.remove_cursor();
	}

	Cursor::State UISkillBook::send_cursor(bool clicked, Point<int16_t> cursorpos)
	{
		static int call_count = 0;
		if (++call_count % 100 == 0) {
			LOG(LOG_DEBUG, "[UISkillBook::send_cursor] Called " << call_count << " times");
		}
		
		Cursor::State dstate = UIDragElement::send_cursor(clicked, cursorpos);

		if (dragged)
			return dstate;

		Point<int16_t> cursor_relative = cursorpos - position;

		if (slider.isenabled())
		{
			if (Cursor::State new_state = slider.send_cursor(cursor_relative, clicked))
			{
				clear_tooltip();

				return new_state;
			}
		}

		Point<int16_t> skill_position_l = position + SKILL_OFFSET + Point<int16_t>(-1, 0);
		Point<int16_t> skill_position_r = position + SKILL_OFFSET + Point<int16_t>(-1 + ROW_WIDTH, 0);

		// Check if we should use single column layout
		bool use_single_column = (bg_dimensions.x() < 250);

		if (!grabbing)
		{
			for (size_t i = 0; i < skills.size(); i++)
			{
				Point<int16_t> skill_position = skill_position_l;

				// Only use right column if window is wide enough
				if (!use_single_column && i % 2)
					skill_position = skill_position_r;

				constexpr Rectangle<int16_t> bounds = Rectangle<int16_t>(0, 32, 0, 32);
				bool inrange = bounds.contains(cursorpos - skill_position);

				if (inrange)
				{
					if (clicked)
					{
						clear_tooltip();
						grabbing = true;

						int32_t skill_id = skills[i].get_id();
						int32_t skill_level = skillbook.get_level(skill_id);

						if (skill_level > 0 && !SkillData::get(skill_id).is_passive())
						{
							skills[i].get_icon()->start_drag(cursorpos - skill_position);
							UI::get().drag_icon(skills[i].get_icon());

							return Cursor::State::GRABBING;
						}
						else
						{
							return Cursor::State::IDLE;
						}
					}
					else
					{
						skills[i].get_icon()->set_state(StatefulIcon::State::MOUSEOVER);
						show_skill(skills[i].get_id());

						return Cursor::State::IDLE;
					}
				}

				// Update position for next row
				if (use_single_column) {
					// Single column: always shift down
					skill_position_l.shift_y(ROW_HEIGHT);
				} else {
					// Two columns: shift down every other skill
					if (i % 2)
					{
						skill_position_l.shift_y(ROW_HEIGHT);
						skill_position_r.shift_y(ROW_HEIGHT);
					}
				}
			}

			for (size_t i = 0; i < skills.size(); i++)
			{
				skills[i].get_icon()->set_state(StatefulIcon::State::NORMAL);
			}
			clear_tooltip();
		}
		else
		{
			grabbing = false;
		}

		return UIElement::send_cursor(clicked, cursorpos);
	}

	void UISkillBook::send_key(int32_t keycode, bool pressed, bool escape)
	{
		if (pressed)
		{
			if (escape)
			{
				if (sp_enabled)
					set_skillpoint(false);
				else
					close();
			}
			else if (keycode == KeyAction::Id::TAB)
			{
				clear_tooltip();

				Job::Level level = job.get_level();
				uint16_t id = tab + 1;
				uint16_t new_tab = tab + Buttons::BT_TAB0;

				if (new_tab < Buttons::BT_TAB4 && id <= level)
					new_tab++;
				else
					new_tab = Buttons::BT_TAB0;

				change_tab(new_tab - Buttons::BT_TAB0);
			}
		}
	}

	UIElement::Type UISkillBook::get_type() const
	{
		return TYPE;
	}

	void UISkillBook::update_stat(MapleStat::Id stat, int16_t value)
	{
		switch (stat)
		{
		case MapleStat::Id::JOB:
			change_job(value);
			break;
		case MapleStat::Id::SP:
			change_sp();
			break;
		}
	}

	void UISkillBook::update_skills(int32_t skill_id)
	{
		change_tab(tab);
	}

	void UISkillBook::change_job(uint16_t id)
	{
		LOG(LOG_DEBUG, "[UISkillBook::change_job] START with job id: " << id);
		
		job.change_job(id);
		LOG(LOG_DEBUG, "[UISkillBook::change_job] job.change_job() completed");

		Job::Level level = job.get_level();
		LOG(LOG_DEBUG, "[UISkillBook::change_job] Job level: " << static_cast<int>(level));

		for (uint16_t i = 0; i <= Job::Level::FOURTH; i++) {
			if (buttons[Buttons::BT_TAB0 + i]) {
				buttons[Buttons::BT_TAB0 + i]->set_active(i <= level);
				LOG(LOG_DEBUG, "[UISkillBook::change_job] Set tab " << i << " active state");
			}
		}

		LOG(LOG_DEBUG, "[UISkillBook::change_job] About to call change_tab()");
		change_tab(level - Job::Level::BEGINNER);
		LOG(LOG_DEBUG, "[UISkillBook::change_job] END");
	}

	void UISkillBook::change_sp()
	{
		Job::Level joblevel = joblevel_by_tab(tab);
		uint16_t level = stats.get_stat(MapleStat::Id::LEVEL);

		if (joblevel == Job::Level::BEGINNER)
		{
			int16_t remaining_beginner_sp = 0;

			if (level >= 7)
				remaining_beginner_sp = 6;
			else
				remaining_beginner_sp = level - 1;

			for (size_t i = 0; i < skills.size(); i++)
			{
				int32_t skillid = skills[i].get_id();

				if (skillid == SkillId::Id::THREE_SNAILS || skillid == SkillId::Id::HEAL || skillid == SkillId::Id::FEATHER)
					remaining_beginner_sp -= skills[i].get_level();
			}

			beginner_sp = remaining_beginner_sp;
			splabel.change_text(std::to_string(beginner_sp));
		}
		else
		{
			sp = stats.get_stat(MapleStat::Id::SP);
			splabel.change_text(std::to_string(sp));
		}

		change_offset(offset);
		set_skillpoint(false);
	}

	void UISkillBook::change_tab(uint16_t new_tab)
	{
		LOG(LOG_DEBUG, "[UISkillBook::change_tab] START with new_tab: " << new_tab << ", current tab: " << tab);
		
		// Only change button states if the buttons exist
		if (buttons[Buttons::BT_TAB0 + tab]) {
			buttons[Buttons::BT_TAB0 + tab]->set_state(Button::NORMAL);
			LOG(LOG_DEBUG, "[UISkillBook::change_tab] Set old tab " << tab << " to NORMAL");
		}
		if (buttons[Buttons::BT_TAB0 + new_tab]) {
			buttons[Buttons::BT_TAB0 + new_tab]->set_state(Button::PRESSED);
			LOG(LOG_DEBUG, "[UISkillBook::change_tab] Set new tab " << new_tab << " to PRESSED");
		}
		tab = new_tab;

		skills.clear();
		skillcount = 0;

		Job::Level joblevel = joblevel_by_tab(tab);
		LOG(LOG_DEBUG, "[UISkillBook::change_tab] Job level from tab: " << static_cast<int>(joblevel));
		
		LOG(LOG_DEBUG, "[UISkillBook::change_tab] About to call job.get_subjob()");
		uint16_t subid = 0;
		try {
			subid = job.get_subjob(joblevel);
			LOG(LOG_DEBUG, "[UISkillBook::change_tab] job.get_subjob() returned");
		} catch (...) {
			LOG(LOG_ERROR, "[UISkillBook::change_tab] Exception in job.get_subjob()");
			subid = 0;
		}
		LOG(LOG_DEBUG, "[UISkillBook::change_tab] Got subjob");
		LOG(LOG_DEBUG, "[UISkillBook::change_tab] subid = " << static_cast<int>(subid));

		try {
			const JobData& data = JobData::get(subid);
			LOG(LOG_DEBUG, "[UISkillBook::change_tab] Got JobData for subid " << subid);

		bookicon = data.get_icon();
		booktext.change_text(data.get_name());
		LOG(LOG_DEBUG, "[UISkillBook::change_tab] Set book icon and text");

		LOG(LOG_DEBUG, "[UISkillBook::change_tab] Starting to load skills");
		const auto& skill_ids = data.get_skills();
		LOG(LOG_DEBUG, "[UISkillBook::change_tab] Job has " << skill_ids.size() << " skills");
		
		for (int32_t skill_id : skill_ids)
		{
			LOG(LOG_DEBUG, "[UISkillBook::change_tab] Processing skill_id: " << skill_id);
			int32_t level = skillbook.get_level(skill_id);
			int32_t masterlevel = skillbook.get_masterlevel(skill_id);

			bool invisible = SkillData::get(skill_id).is_invisible();

			if (invisible && masterlevel == 0)
				continue;

			skills.emplace_back(skill_id, level);
			skillcount++;
		}
		LOG(LOG_DEBUG, "[UISkillBook::change_tab] Loaded " << skillcount << " skills");

		slider.setrows(ROWS, skillcount);
		LOG(LOG_DEBUG, "[UISkillBook::change_tab] Set slider rows");
		
		change_offset(0);
		LOG(LOG_DEBUG, "[UISkillBook::change_tab] Called change_offset");
		
		change_sp();
		LOG(LOG_DEBUG, "[UISkillBook::change_tab] END");
		} catch (const std::exception& e) {
			LOG(LOG_ERROR, "[UISkillBook::change_tab] Exception caught: " << e.what());
			// Set some defaults to prevent crash
			booktext.change_text("Unknown");
			skillcount = 0;
			slider.setrows(ROWS, 0);
		}
	}

	void UISkillBook::change_offset(uint16_t new_offset)
	{
		offset = new_offset;

		for (int16_t i = 0; i < ROWS; i++)
		{
			uint16_t index = Buttons::BT_SPUP0 + i;
			uint16_t row = offset + i;
			buttons[index]->set_active(row < skillcount);

			if (row < skills.size())
			{
				int32_t skill_id = skills[row].get_id();
				bool canraise = can_raise(skill_id);
				buttons[index]->set_state(canraise ? Button::State::NORMAL : Button::State::DISABLED);
			}
		}
	}

	void UISkillBook::show_skill(int32_t id)
	{
		int32_t skill_id = id;
		int32_t level = skillbook.get_level(id);
		int32_t masterlevel = skillbook.get_masterlevel(id);
		int64_t expiration = skillbook.get_expiration(id);

		UI::get().show_skill(Tooltip::Parent::SKILLBOOK, skill_id, level, masterlevel, expiration);
	}

	void UISkillBook::clear_tooltip()
	{
		UI::get().clear_tooltip(Tooltip::Parent::SKILLBOOK);
	}

	bool UISkillBook::can_raise(int32_t skill_id) const
	{
		Job::Level joblevel = joblevel_by_tab(tab);

		if (joblevel == Job::Level::BEGINNER && beginner_sp <= 0)
			return false;

		if (tab + Buttons::BT_TAB0 != Buttons::BT_TAB0 && sp <= 0)
			return false;

		int32_t level = skillbook.get_level(skill_id);
		int32_t masterlevel = skillbook.get_masterlevel(skill_id);

		if (masterlevel == 0)
			masterlevel = SkillData::get(skill_id).get_masterlevel();

		if (level >= masterlevel)
			return false;

		switch (skill_id)
		{
		case SkillId::Id::ANGEL_BLESSING:
			return false;
		default:
			return check_required(skill_id);
		}
	}

	void UISkillBook::send_spup(uint16_t row)
	{
		if (row >= skills.size())
			return;

		int32_t id = skills[row].get_id();

		// Direct skill level up without opening the allocation window
		spend_sp(id);
		
		// Don't open the skill point allocation window
		/*
		if (sp_enabled && id == sp_id)
		{
			set_skillpoint(false);
			return;
		}

		int32_t level = skills[row].get_level();
		int32_t used = 1;

		const SkillData& skillData = SkillData::get(id);
		std::string name = skillData.get_name();
		int16_t cur_sp = safe_stoi(splabel.get_text());

		sp_before_text = std::to_string(level);
		sp_after_text = std::to_string(level + used);
		sp_used.change_text(std::to_string(used));
		sp_remaining.change_text(std::to_string(cur_sp - used));
		sp_name.change_text(name);
		sp_skill = skills[row].get_icon()->get_texture();
		sp_id = id;
		sp_masterlevel = skillData.get_masterlevel();

		if (sp_masterlevel == 1)
		{
			if (buttons[Buttons::BT_SPDOWN]) buttons[Buttons::BT_SPDOWN]->set_state(Button::State::DISABLED);
			if (buttons[Buttons::BT_SPMAX]) buttons[Buttons::BT_SPMAX]->set_state(Button::State::DISABLED);
			if (buttons[Buttons::BT_SPUP]) buttons[Buttons::BT_SPUP]->set_state(Button::State::DISABLED);
		}
		else
		{
			if (buttons[Buttons::BT_SPDOWN]) buttons[Buttons::BT_SPDOWN]->set_state(Button::State::DISABLED);
			if (buttons[Buttons::BT_SPMAX]) buttons[Buttons::BT_SPMAX]->set_state(Button::State::NORMAL);
			if (buttons[Buttons::BT_SPUP]) buttons[Buttons::BT_SPUP]->set_state(Button::State::NORMAL);
		}

		if (!sp_enabled)
			set_skillpoint(true);
		*/
	}

	void UISkillBook::spend_sp(int32_t skill_id)
	{
		SpendSpPacket(skill_id).dispatch();

		UI::get().disable();
	}

	Job::Level UISkillBook::joblevel_by_tab(uint16_t t) const
	{
		switch (t)
		{
		case 1:
			return Job::Level::FIRST;
		case 2:
			return Job::Level::SECOND;
		case 3:
			return Job::Level::THIRD;
		case 4:
			return Job::Level::FOURTH;
		default:
			return Job::Level::BEGINNER;
		}
	}

	const UISkillBook::SkillDisplayMeta* UISkillBook::skill_by_position(Point<int16_t> cursorpos) const
	{
		int16_t x = cursorpos.x();

		if (x < SKILL_OFFSET.x() || x > SKILL_OFFSET.x() + 2 * ROW_WIDTH)
			return nullptr;

		int16_t y = cursorpos.y();

		if (y < SKILL_OFFSET.y())
			return nullptr;

		uint16_t row = (y - SKILL_OFFSET.y()) / ROW_HEIGHT;

		if (row < 0 || row >= ROWS)
			return nullptr;

		uint16_t offset_row = offset + row;

		if (offset_row >= ROWS)
			return nullptr;

		uint16_t col = (x - SKILL_OFFSET.x()) / ROW_WIDTH;

		uint16_t skill_idx = 2 * offset_row + col;

		if (skill_idx >= skills.size())
			return nullptr;

		auto iter = skills.data() + skill_idx;

		return iter;
	}

	void UISkillBook::close()
	{
		clear_tooltip();
		deactivate();
	}

	bool UISkillBook::check_required(int32_t id) const
	{
		std::unordered_map<int32_t, int32_t> required = skillbook.collect_required(id);

		if (required.size() <= 0)
			required = SkillData::get(id).get_reqskills();

		for (auto reqskill : required)
		{
			int32_t reqskill_level = skillbook.get_level(reqskill.first);
			int32_t req_level = reqskill.second;

			if (reqskill_level < req_level)
				return false;
		}

		return true;
	}

	void UISkillBook::set_macro(bool enabled)
	{
		macro_enabled = enabled;

		if (macro_enabled)
			dimension = bg_dimensions + Point<int16_t>(macro_backgrnd.get_dimensions().x(), 0);
		else if (!sp_enabled)
			dimension = bg_dimensions;

		if (buttons[Buttons::BT_MACRO_OK]) {
			buttons[Buttons::BT_MACRO_OK]->set_active(macro_enabled);
		}

		if (macro_enabled && sp_enabled)
			set_skillpoint(false);
	}

	void UISkillBook::set_skillpoint(bool enabled)
	{
		sp_enabled = enabled;

		if (sp_enabled)
			dimension = bg_dimensions + Point<int16_t>(sp_backgrnd.get_dimensions().x(), 0);
		else if (!macro_enabled)
			dimension = bg_dimensions;

		if (buttons[Buttons::BT_CANCLE]) buttons[Buttons::BT_CANCLE]->set_active(sp_enabled);
		if (buttons[Buttons::BT_OKAY]) buttons[Buttons::BT_OKAY]->set_active(sp_enabled);
		if (buttons[Buttons::BT_SPDOWN]) buttons[Buttons::BT_SPDOWN]->set_active(sp_enabled);
		if (buttons[Buttons::BT_SPMAX]) buttons[Buttons::BT_SPMAX]->set_active(sp_enabled);
		if (buttons[Buttons::BT_SPUP]) buttons[Buttons::BT_SPUP]->set_active(sp_enabled);

		if (sp_enabled && macro_enabled)
			set_macro(false);
	}

	bool UISkillBook::is_skillpoint_enabled()
	{
		return sp_enabled;
	}
}