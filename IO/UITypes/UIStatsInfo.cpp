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
#include "UIStatsInfo.h"

#include "UINotice.h"

#include "../UI.h"

#include "../Components/MapleButton.h"

#include "../../Gameplay/Stage.h"

#include "../../Net/Packets/PlayerPackets.h"

#ifdef USE_NX
#include <nlnx/nx.hpp>
#endif

namespace ms
{
	UIStatsInfo::UIStatsInfo(const CharStats& st) : UIDragElement<PosSTATS>(Point<int16_t>(212, 20)), stats(st)
	{
		// Use UIWindow.img/Stat structure
		nl::node Stat = nl::nx::UI["UIWindow.img"]["Stat"];
		
		if (!Stat) {
			// If no Stats window assets found, create minimal window
			dimension = Point<int16_t>(200, 300);
			dragarea = Point<int16_t>(200, 20);
			return;
		}
		
		// Load main background
		nl::node backgrnd = Stat["backgrnd"];
		if (backgrnd) sprites.emplace_back(backgrnd);
		
		// Load detail/extended stats background  
		nl::node backgrnd2 = Stat["backgrnd2"];
		if (backgrnd2) textures_detail.emplace_back(backgrnd2);
		
		// v92: Don't load backgrnd3 - it causes overlay issues
		// The main background (backgrnd) is sufficient
		
		// For ability-related nodes, try both direct and nested paths
		nl::node abilityTitle = Stat["abilityTitle"];
		if (!abilityTitle) {
			nl::node detail = Stat["detail"];
			if (detail) abilityTitle = detail["abilityTitle"];
		}
		
		nl::node metierLine = Stat["metierLine"];
		if (!metierLine) {
			nl::node detail = Stat["detail"];
			if (detail) metierLine = detail["metierLine"];
		}

		// Add abilities only if nodes exist
		if (abilityTitle["rare"] && abilityTitle["rare"]["0"]) abilities[Ability::RARE] = abilityTitle["rare"]["0"];
		if (abilityTitle["epic"] && abilityTitle["epic"]["0"]) abilities[Ability::EPIC] = abilityTitle["epic"]["0"];
		if (abilityTitle["unique"] && abilityTitle["unique"]["0"]) abilities[Ability::UNIQUE] = abilityTitle["unique"]["0"];
		if (abilityTitle["legendary"] && abilityTitle["legendary"]["0"]) abilities[Ability::LEGENDARY] = abilityTitle["legendary"]["0"];
		if (abilityTitle["normal"] && abilityTitle["normal"]["0"]) abilities[Ability::NONE] = abilityTitle["normal"]["0"];

		if (metierLine["activated"] && metierLine["activated"]["0"]) inner_ability[true] = metierLine["activated"]["0"];
		if (metierLine["disabled"] && metierLine["disabled"]["0"]) inner_ability[false] = metierLine["disabled"]["0"];

		// Close button - try window-specific first, then fallback
		nl::node close = Stat["BtClose"];
		if (!close) close = nl::nx::UI["Basic.img"]["BtClose3"];
		if (close) buttons[Buttons::BT_CLOSE] = std::make_unique<MapleButton>(close, Point<int16_t>(153, 5));
		
		// AP up buttons - positioned based on JSON layout
		// Button x=150, aligned with stat rows
		nl::node btApUp = Stat["BtApUp"];
		if (!btApUp) btApUp = Stat["BtHpUp"]; // Fallback for older versions

		if (btApUp) {
			buttons[Buttons::BT_HP] = std::make_unique<MapleButton>(btApUp, Point<int16_t>(150, 113));
			buttons[Buttons::BT_MP] = std::make_unique<MapleButton>(btApUp, Point<int16_t>(150, 131));
			buttons[Buttons::BT_STR] = std::make_unique<MapleButton>(btApUp, Point<int16_t>(150, 246));
			buttons[Buttons::BT_DEX] = std::make_unique<MapleButton>(btApUp, Point<int16_t>(150, 264));
			buttons[Buttons::BT_INT] = std::make_unique<MapleButton>(btApUp, Point<int16_t>(150, 282));
			buttons[Buttons::BT_LUK] = std::make_unique<MapleButton>(btApUp, Point<int16_t>(150, 300));
		}
		
		// Auto button - positioned based on JSON layout
		nl::node btAuto = Stat["BtAuto"];
		if (btAuto) buttons[Buttons::BT_AUTO] = std::make_unique<MapleButton>(btAuto, Point<int16_t>(94, 200));

		// Detail button - position at bottom of the window
		nl::node btDetail = Stat["BtDetail"];
		if (btDetail) {
			buttons[Buttons::BT_DETAILOPEN] = std::make_unique<MapleButton>(btDetail, Point<int16_t>(119, 324));
			buttons[Buttons::BT_DETAILCLOSE] = std::make_unique<MapleButton>(btDetail, Point<int16_t>(119, 324));
		}
		
		// Hyper stat buttons (may not exist in older versions)
		nl::node btHyperStatOpen = Stat["BtHyperStatOpen"];
		if (btHyperStatOpen) buttons[Buttons::BT_HYPERSTATOPEN] = std::make_unique<MapleButton>(btHyperStatOpen);
		
		nl::node btHyperStatClose = Stat["BtHyperStatClose"];
		if (btHyperStatClose) buttons[Buttons::BT_HYPERSTATCLOSE] = std::make_unique<MapleButton>(btHyperStatClose);
		
		// Ability button (may be in detail node)
		nl::node btAbility = Stat["BtAbility"];
		if (!btAbility) {
			nl::node detail = Stat["detail"];
			if (detail) btAbility = detail["BtAbility"];
		}
		// These buttons need to be positioned relative to the detail window
		// They will be repositioned dynamically in set_detail()
		if (btAbility) buttons[Buttons::BT_ABILITY] = std::make_unique<MapleButton>(btAbility);
		
		// Detail close button
		if (btDetail) buttons[Buttons::BT_DETAIL_DETAILCLOSE] = std::make_unique<MapleButton>(btDetail);

		// Set button states only if buttons exist
		if (buttons[Buttons::BT_HYPERSTATOPEN]) buttons[Buttons::BT_HYPERSTATOPEN]->set_active(false);
		if (buttons[Buttons::BT_DETAILCLOSE]) buttons[Buttons::BT_DETAILCLOSE]->set_active(false);
		if (buttons[Buttons::BT_ABILITY]) {
			buttons[Buttons::BT_ABILITY]->set_active(false);
			buttons[Buttons::BT_ABILITY]->set_state(Button::State::DISABLED);
		}
		if (buttons[Buttons::BT_DETAIL_DETAILCLOSE]) buttons[Buttons::BT_DETAIL_DETAILCLOSE]->set_active(false);

		jobId = stats.get_stat(MapleStat::Id::JOB);

		if (jobId == Job::Level::BEGINNER)
		{
			if (buttons[Buttons::BT_HP]) buttons[Buttons::BT_HP]->set_active(false);
			if (buttons[Buttons::BT_MP]) buttons[Buttons::BT_MP]->set_active(false);
			if (buttons[Buttons::BT_STR]) buttons[Buttons::BT_STR]->set_active(false);
			if (buttons[Buttons::BT_DEX]) buttons[Buttons::BT_DEX]->set_active(false);
			if (buttons[Buttons::BT_INT]) buttons[Buttons::BT_INT]->set_active(false);
			if (buttons[Buttons::BT_LUK]) buttons[Buttons::BT_LUK]->set_active(false);
			if (buttons[Buttons::BT_AUTO]) buttons[Buttons::BT_AUTO]->set_active(false);
		}

		update_ap();

		// Normal
		for (size_t i = StatLabel::NAME; i <= LUK; i++)
			statlabels[i] = Text(Text::Font::A11M, Text::Alignment::LEFT, Color::Name::EMPEROR);

		statlabels[StatLabel::AP] = Text(Text::Font::A11M, Text::Alignment::RIGHT, Color::Name::EMPEROR);

		// Text positions based on UI Screen Editor JSON layout
		statoffsets[StatLabel::NAME] = Point<int16_t>(58, 31);
		statoffsets[StatLabel::JOB] = Point<int16_t>(58, 48);
		statoffsets[StatLabel::FAME] = Point<int16_t>(59, 76);
		statoffsets[StatLabel::GUILD] = Point<int16_t>(59, 95);
		statoffsets[StatLabel::HP] = Point<int16_t>(58, 113);
		statoffsets[StatLabel::MP] = Point<int16_t>(58, 131);
		statoffsets[StatLabel::AP] = Point<int16_t>(59, 151);
		statoffsets[StatLabel::MIN_DAMAGE] = Point<int16_t>(58, 169);
		statoffsets[StatLabel::MAX_DAMAGE] = Point<int16_t>(100, 169);
		statoffsets[StatLabel::STR] = Point<int16_t>(59, 243);
		statoffsets[StatLabel::DEX] = Point<int16_t>(59, 262);
		statoffsets[StatLabel::INT] = Point<int16_t>(59, 280);
		statoffsets[StatLabel::LUK] = Point<int16_t>(59, 297);

		// Detailed stats labels
		statlabels[StatLabel::MIN_DAMAGE_DETAILED] = Text(Text::Font::A11M, Text::Alignment::RIGHT, Color::Name::EMPEROR);
		statlabels[StatLabel::MAX_DAMAGE_DETAILED] = Text(Text::Font::A11M, Text::Alignment::RIGHT, Color::Name::EMPEROR);
		statlabels[StatLabel::DAMAGE_BONUS] = Text(Text::Font::A11M, Text::Alignment::RIGHT, Color::Name::EMPEROR);
		statlabels[StatLabel::BOSS_DAMAGE] = Text(Text::Font::A11M, Text::Alignment::RIGHT, Color::Name::EMPEROR);
		statlabels[StatLabel::FINAL_DAMAGE] = Text(Text::Font::A11M, Text::Alignment::RIGHT, Color::Name::EMPEROR);
		statlabels[StatLabel::BUFF_DURATION] = Text(Text::Font::A11M, Text::Alignment::RIGHT, Color::Name::EMPEROR);
		statlabels[StatLabel::IGNORE_DEFENSE] = Text(Text::Font::A11M, Text::Alignment::RIGHT, Color::Name::EMPEROR);
		statlabels[StatLabel::ITEM_DROP_RATE] = Text(Text::Font::A11M, Text::Alignment::RIGHT, Color::Name::EMPEROR);
		statlabels[StatLabel::CRITICAL_RATE] = Text(Text::Font::A11M, Text::Alignment::RIGHT, Color::Name::EMPEROR);
		statlabels[StatLabel::MESOS_OBTAINED] = Text(Text::Font::A11M, Text::Alignment::RIGHT, Color::Name::EMPEROR);
		statlabels[StatLabel::CRITICAL_DAMAGE] = Text(Text::Font::A11M, Text::Alignment::RIGHT, Color::Name::EMPEROR);
		statlabels[StatLabel::STATUS_RESISTANCE] = Text(Text::Font::A11M, Text::Alignment::RIGHT, Color::Name::EMPEROR);
		statlabels[StatLabel::KNOCKBACK_RESISTANCE] = Text(Text::Font::A11M, Text::Alignment::RIGHT, Color::Name::EMPEROR);
		statlabels[StatLabel::DEFENSE] = Text(Text::Font::A11M, Text::Alignment::RIGHT, Color::Name::EMPEROR);
		statlabels[StatLabel::SPEED] = Text(Text::Font::A11M, Text::Alignment::RIGHT, Color::Name::EMPEROR);
		statlabels[StatLabel::JUMP] = Text(Text::Font::A11M, Text::Alignment::RIGHT, Color::Name::EMPEROR);
		statlabels[StatLabel::HONOR] = Text(Text::Font::A11M, Text::Alignment::LEFT, Color::Name::EMPEROR);

		// Detailed stat positions based on UI Screen Editor JSON layout
		// Detail panel at (172, 143), draw shifts by (172, 143)
		// statoffsets[i].x = JSON.x - 172, statoffsets[i].y = JSON.y - 143
		statoffsets[StatLabel::MIN_DAMAGE_DETAILED] = Point<int16_t>(79, 7);   // Damage: x=251-172=79, y=150-143=7
		statoffsets[StatLabel::MAX_DAMAGE_DETAILED] = Point<int16_t>(79, 25);  // Damage line 2

		// Row positions for detailed stats (relative to detail window)
		int16_t detail_col_left = 79;   // Left column x (251 - 172 = 79)
		int16_t detail_col_right = 130; // Right column x

		statoffsets[StatLabel::DAMAGE_BONUS] = Point<int16_t>(detail_col_left, 43);
		statoffsets[StatLabel::BOSS_DAMAGE] = Point<int16_t>(detail_col_left, 79);     // y=222-143=79

		statoffsets[StatLabel::FINAL_DAMAGE] = Point<int16_t>(detail_col_left, 61);
		statoffsets[StatLabel::BUFF_DURATION] = Point<int16_t>(detail_col_right, 61);

		statoffsets[StatLabel::IGNORE_DEFENSE] = Point<int16_t>(detail_col_left, 95);  // y=238-143=95
		statoffsets[StatLabel::ITEM_DROP_RATE] = Point<int16_t>(detail_col_right, 95);

		statoffsets[StatLabel::CRITICAL_RATE] = Point<int16_t>(detail_col_left, 133);  // y=276-143=133
		statoffsets[StatLabel::MESOS_OBTAINED] = Point<int16_t>(detail_col_right, 133);

		statoffsets[StatLabel::CRITICAL_DAMAGE] = Point<int16_t>(detail_col_left, 115);

		statoffsets[StatLabel::STATUS_RESISTANCE] = Point<int16_t>(detail_col_left, 151);  // y=294-143=151
		statoffsets[StatLabel::KNOCKBACK_RESISTANCE] = Point<int16_t>(detail_col_right, 151);

		statoffsets[StatLabel::DEFENSE] = Point<int16_t>(detail_col_left, 169);

		statoffsets[StatLabel::SPEED] = Point<int16_t>(detail_col_left, 187);
		statoffsets[StatLabel::JUMP] = Point<int16_t>(detail_col_left, 205);

		statoffsets[StatLabel::HONOR] = Point<int16_t>(79, 223);

		update_all_stats();
		update_stat(MapleStat::Id::JOB);
		update_stat(MapleStat::Id::FAME);

		dimension = Point<int16_t>(212, 318);
		showdetail = false;
	}

	void UIStatsInfo::draw(float alpha) const
	{
		UIElement::draw_sprites(alpha);

		if (showdetail && !textures_detail.empty())
		{
			// Calculate detail window position based on JSON layout
			// backgrnd2 is at (172, 143) relative to main window
			Point<int16_t> detail_pos(position + Point<int16_t>(172, 143));

			// Draw the extended stats background (backgrnd2)
			textures_detail[0].draw(detail_pos);

			abilities[Ability::NONE].draw(DrawArgument(detail_pos));

			inner_ability[false].draw(detail_pos);
			inner_ability[false].draw(detail_pos + Point<int16_t>(0, 19));
			inner_ability[false].draw(detail_pos + Point<int16_t>(0, 38));
		}

		size_t last = showdetail ? StatLabel::NUM_LABELS : StatLabel::NUM_NORMAL;

		for (size_t i = 0; i < last; i++)
		{
			Point<int16_t> labelpos = position + statoffsets[i];

			// Detail stats are offset by the detail panel position (172, 143)
			if (i >= StatLabel::NUM_NORMAL) {
				labelpos.shift_x(172);
				labelpos.shift_y(143);
			}

			if (jobId == Job::Level::BEGINNER)
			{
				if (i < AP || i > NUM_NORMAL)
					statlabels[i].draw(labelpos);
			}
			else
			{
				statlabels[i].draw(labelpos);
			}
		}

		UIElement::draw_buttons(alpha);
	}

	void UIStatsInfo::send_key(int32_t keycode, bool pressed, bool escape)
	{
		if (pressed && escape)
			deactivate();
	}

	bool UIStatsInfo::is_in_range(Point<int16_t> cursorpos) const
	{
		Point<int16_t> pos_adj;

		if (showdetail)
			// Detail panel adds 177 width at x=172, so total extends to 172+177=349
			pos_adj = Point<int16_t>(172, 0);
		else
			pos_adj = Point<int16_t>(0, 0);

		auto bounds = Rectangle<int16_t>(position, position + dimension + pos_adj);
		return bounds.contains(cursorpos);
	}

	UIElement::Type UIStatsInfo::get_type() const
	{
		return TYPE;
	}

	void UIStatsInfo::update_all_stats()
	{
		update_simple(AP, MapleStat::Id::AP);

		if (hasap ^ (stats.get_stat(MapleStat::Id::AP) > 0))
			update_ap();

		statlabels[StatLabel::NAME].change_text(stats.get_name());
		statlabels[StatLabel::GUILD].change_text("-");
		statlabels[StatLabel::HP].change_text(std::to_string(stats.get_stat(MapleStat::Id::HP)) + " / " + std::to_string(stats.get_total(EquipStat::Id::HP)));
		statlabels[StatLabel::MP].change_text(std::to_string(stats.get_stat(MapleStat::Id::MP)) + " / " + std::to_string(stats.get_total(EquipStat::Id::MP)));

		update_basevstotal(StatLabel::STR, MapleStat::Id::STR, EquipStat::Id::STR);
		update_basevstotal(StatLabel::DEX, MapleStat::Id::DEX, EquipStat::Id::DEX);
		update_basevstotal(StatLabel::INT, MapleStat::Id::INT, EquipStat::Id::INT);
		update_basevstotal(StatLabel::LUK, MapleStat::Id::LUK, EquipStat::Id::LUK);

		statlabels[StatLabel::MIN_DAMAGE].change_text(std::to_string(stats.get_mindamage()));
		statlabels[StatLabel::MAX_DAMAGE].change_text(" ~ " + std::to_string(stats.get_maxdamage()));

		if (stats.is_damage_buffed())
		{
			statlabels[StatLabel::MIN_DAMAGE].change_color(Color::Name::RED);
			statlabels[StatLabel::MIN_DAMAGE].change_color(Color::Name::RED);
		}
		else
		{
			statlabels[StatLabel::MAX_DAMAGE].change_color(Color::Name::EMPEROR);
			statlabels[StatLabel::MAX_DAMAGE].change_color(Color::Name::EMPEROR);
		}

		statlabels[StatLabel::MIN_DAMAGE_DETAILED].change_text(std::to_string(stats.get_mindamage()));
		statlabels[StatLabel::MAX_DAMAGE_DETAILED].change_text(" ~ " + std::to_string(stats.get_maxdamage()));
		statlabels[StatLabel::DAMAGE_BONUS].change_text("0%");
		statlabels[StatLabel::BOSS_DAMAGE].change_text(std::to_string(static_cast<int32_t>(stats.get_bossdmg() * 100)) + "%");
		statlabels[StatLabel::FINAL_DAMAGE].change_text("0.00%");
		statlabels[StatLabel::BUFF_DURATION].change_text("0%");
		statlabels[StatLabel::IGNORE_DEFENSE].change_text(std::to_string(static_cast<int32_t>(stats.get_ignoredef())) + "%");
		statlabels[StatLabel::ITEM_DROP_RATE].change_text("0%");
		statlabels[StatLabel::CRITICAL_RATE].change_text(std::to_string(static_cast<int32_t>(stats.get_critical() * 100)) + "%");
		statlabels[StatLabel::MESOS_OBTAINED].change_text("0%");
		statlabels[StatLabel::CRITICAL_DAMAGE].change_text("0.00%");
		statlabels[StatLabel::STATUS_RESISTANCE].change_text(std::to_string(static_cast<int32_t>(stats.get_resistance())));
		statlabels[StatLabel::KNOCKBACK_RESISTANCE].change_text("0%");

		update_buffed(StatLabel::DEFENSE, EquipStat::Id::WDEF);

		statlabels[StatLabel::SPEED].change_text(std::to_string(stats.get_total(EquipStat::Id::SPEED)) + "%");
		statlabels[StatLabel::JUMP].change_text(std::to_string(stats.get_total(EquipStat::Id::JUMP)) + "%");
		statlabels[StatLabel::HONOR].change_text(std::to_string(stats.get_honor()));
	}

	void UIStatsInfo::update_stat(MapleStat::Id stat)
	{
		switch (stat)
		{
			case MapleStat::Id::JOB:
			{
				jobId = stats.get_stat(MapleStat::Id::JOB);

				statlabels[StatLabel::JOB].change_text(stats.get_jobname());

				if (buttons[Buttons::BT_HP]) buttons[Buttons::BT_HP]->set_active(jobId != Job::Level::BEGINNER);
				if (buttons[Buttons::BT_MP]) buttons[Buttons::BT_MP]->set_active(jobId != Job::Level::BEGINNER);
				if (buttons[Buttons::BT_STR]) buttons[Buttons::BT_STR]->set_active(jobId != Job::Level::BEGINNER);
				if (buttons[Buttons::BT_DEX]) buttons[Buttons::BT_DEX]->set_active(jobId != Job::Level::BEGINNER);
				if (buttons[Buttons::BT_INT]) buttons[Buttons::BT_INT]->set_active(jobId != Job::Level::BEGINNER);
				if (buttons[Buttons::BT_LUK]) buttons[Buttons::BT_LUK]->set_active(jobId != Job::Level::BEGINNER);
				if (buttons[Buttons::BT_AUTO]) buttons[Buttons::BT_AUTO]->set_active(jobId != Job::Level::BEGINNER);

				break;
			}
			case MapleStat::Id::FAME:
			{
				update_simple(StatLabel::FAME, MapleStat::Id::FAME);
				break;
			}
		}
	}

	Button::State UIStatsInfo::button_pressed(uint16_t id)
	{
		const Player& player = Stage::get().get_player();

		switch (id)
		{
			case Buttons::BT_CLOSE:
			{
				deactivate();
				break;
			}
			case Buttons::BT_HP:
			{
				send_apup(MapleStat::Id::HP);
				break;
			}
			case Buttons::BT_MP:
			{
				send_apup(MapleStat::Id::MP);
				break;
			}
			case Buttons::BT_STR:
			{
				send_apup(MapleStat::Id::STR);
				break;
			}
			case Buttons::BT_DEX:
			{
				send_apup(MapleStat::Id::DEX);
				break;
			}
			case Buttons::BT_INT:
			{
				send_apup(MapleStat::Id::INT);
				break;
			}
			case Buttons::BT_LUK:
			{
				send_apup(MapleStat::Id::LUK);
				break;
			}
			case Buttons::BT_AUTO:
			{
				uint16_t autostr = 0;
				uint16_t autodex = 0;
				uint16_t autoint = 0;
				uint16_t autoluk = 0;
				uint16_t nowap = stats.get_stat(MapleStat::Id::AP);
				EquipStat::Id id = player.get_stats().get_job().get_primary(player.get_weapontype());

				switch (id)
				{
					case EquipStat::Id::STR:
						autostr = nowap;
						break;
					case EquipStat::Id::DEX:
						autodex = nowap;
						break;
					case EquipStat::Id::INT:
						autoint = nowap;
						break;
					case EquipStat::Id::LUK:
						autoluk = nowap;
						break;
				}

				std::string message =
					"Your AP will be distributed as follows:\\r"
					"\\nSTR : +" + std::to_string(autostr) +
					"\\nDEX : +" + std::to_string(autodex) +
					"\\nINT : +" + std::to_string(autoint) +
					"\\nLUK : +" + std::to_string(autoluk) +
					"\\r\\n";

				std::function<void(bool)> yesnohandler = [&, autostr, autodex, autoint, autoluk](bool yes)
				{
					if (yes)
					{
						if (autostr > 0)
							for (size_t i = 0; i < autostr; i++)
								send_apup(MapleStat::Id::STR);

						if (autodex > 0)
							for (size_t i = 0; i < autodex; i++)
								send_apup(MapleStat::Id::DEX);

						if (autoint > 0)
							for (size_t i = 0; i < autoint; i++)
								send_apup(MapleStat::Id::INT);

						if (autoluk > 0)
							for (size_t i = 0; i < autoluk; i++)
								send_apup(MapleStat::Id::LUK);
					}
				};

				UI::get().emplace<UIYesNo>(message, yesnohandler, Text::Alignment::LEFT);
				break;
			}
			case Buttons::BT_HYPERSTATOPEN:
			{
				break;
			}
			case Buttons::BT_HYPERSTATCLOSE:
			{
				if (player.get_level() < 140)
					UI::get().emplace<UIOk>("You can use the Hyper Stat at Lv. 140 and above.", [](bool) {});

				break;
			}
			case Buttons::BT_DETAILOPEN:
			{
				set_detail(true);
				break;
			}
			case Buttons::BT_DETAILCLOSE:
			case Buttons::BT_DETAIL_DETAILCLOSE:
			{
				set_detail(false);
				break;
			}
			case Buttons::BT_ABILITY:
			default:
			{
				break;
			}
		}

		return Button::State::NORMAL;
	}

	void UIStatsInfo::send_apup(MapleStat::Id stat) const
	{
		SpendApPacket(stat).dispatch();
		UI::get().disable();
	}

	void UIStatsInfo::set_detail(bool enabled)
	{
		showdetail = enabled;

		if (buttons[Buttons::BT_DETAILOPEN]) buttons[Buttons::BT_DETAILOPEN]->set_active(!enabled);
		if (buttons[Buttons::BT_DETAILCLOSE]) buttons[Buttons::BT_DETAILCLOSE]->set_active(enabled);
		
		// Update positions of detail window buttons based on JSON layout
		// Detail panel at (172, 143)
		if (enabled && !textures_detail.empty()) {
			if (buttons[Buttons::BT_ABILITY]) {
				buttons[Buttons::BT_ABILITY]->set_position(Point<int16_t>(172, 143));
				buttons[Buttons::BT_ABILITY]->set_active(enabled);
			}
			if (buttons[Buttons::BT_DETAIL_DETAILCLOSE]) {
				// Position close button at bottom of detail panel
				buttons[Buttons::BT_DETAIL_DETAILCLOSE]->set_position(Point<int16_t>(172 + 119, 143 + 181));
				buttons[Buttons::BT_DETAIL_DETAILCLOSE]->set_active(enabled);
			}
		} else {
			if (buttons[Buttons::BT_ABILITY]) buttons[Buttons::BT_ABILITY]->set_active(false);
			if (buttons[Buttons::BT_DETAIL_DETAILCLOSE]) buttons[Buttons::BT_DETAIL_DETAILCLOSE]->set_active(false);
		}
	}

	void UIStatsInfo::update_ap()
	{
		bool nowap = stats.get_stat(MapleStat::Id::AP) > 0;
		Button::State newstate = nowap ? Button::State::NORMAL : Button::State::DISABLED;

		for (int i = Buttons::BT_HP; i <= Buttons::BT_AUTO; i++) {
			if (buttons[i]) buttons[i]->set_state(newstate);
		}

		hasap = nowap;
	}

	void UIStatsInfo::update_simple(StatLabel label, MapleStat::Id stat)
	{
		statlabels[label].change_text(std::to_string(stats.get_stat(stat)));
	}

	void UIStatsInfo::update_basevstotal(StatLabel label, MapleStat::Id bstat, EquipStat::Id tstat)
	{
		int32_t base = stats.get_stat(bstat);
		int32_t total = stats.get_total(tstat);
		int32_t delta = total - base;

		std::string stattext = std::to_string(total);

		if (delta)
		{
			stattext += " (" + std::to_string(base);

			if (delta > 0)
				stattext += "+" + std::to_string(delta);
			else if (delta < 0)
				stattext += "-" + std::to_string(-delta);

			stattext += ")";
		}

		statlabels[label].change_text(stattext);
	}

	void UIStatsInfo::update_buffed(StatLabel label, EquipStat::Id stat)
	{
		int32_t total = stats.get_total(stat);
		int32_t delta = stats.get_buffdelta(stat);

		std::string stattext = std::to_string(total);

		if (delta)
		{
			stattext += " (" + std::to_string(total - delta);

			if (delta > 0)
			{
				stattext += "+" + std::to_string(delta);

				statlabels[label].change_color(Color::Name::RED);
			}
			else if (delta < 0)
			{
				stattext += "-" + std::to_string(-delta);

				statlabels[label].change_color(Color::Name::BLUE);
			}

			stattext += ")";
		}

		statlabels[label].change_text(stattext);
	}
}