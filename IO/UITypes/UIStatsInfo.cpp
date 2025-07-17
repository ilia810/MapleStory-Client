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
		nl::node close = nl::nx::UI["Basic.img"]["BtClose3"];
		
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

		// Create buttons
		if (close) buttons[Buttons::BT_CLOSE] = std::make_unique<MapleButton>(close, Point<int16_t>(190, 6));
		
		// AP up buttons - try different names
		nl::node btApUp = Stat["BtApUp"];
		if (!btApUp) btApUp = Stat["BtHpUp"]; // Fallback for older versions
		
		if (btApUp) {
			buttons[Buttons::BT_HP] = std::make_unique<MapleButton>(btApUp);
			buttons[Buttons::BT_MP] = std::make_unique<MapleButton>(btApUp);
			buttons[Buttons::BT_STR] = std::make_unique<MapleButton>(btApUp);
			buttons[Buttons::BT_DEX] = std::make_unique<MapleButton>(btApUp);
			buttons[Buttons::BT_INT] = std::make_unique<MapleButton>(btApUp);
			buttons[Buttons::BT_LUK] = std::make_unique<MapleButton>(btApUp);
		}
		
		// Auto button
		nl::node btAuto = Stat["BtAuto"];
		if (btAuto) buttons[Buttons::BT_AUTO] = std::make_unique<MapleButton>(btAuto);
		
		// Detail button - position at bottom right
		nl::node btDetail = Stat["BtDetail"];
		if (btDetail) {
			// Position the detail button at bottom right of the window
			buttons[Buttons::BT_DETAILOPEN] = std::make_unique<MapleButton>(btDetail, Point<int16_t>(165, 290));
			buttons[Buttons::BT_DETAILCLOSE] = std::make_unique<MapleButton>(btDetail, Point<int16_t>(165, 290));
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

		// Adjust text positions for better alignment
		Point<int16_t> statoffset = Point<int16_t>(73, 27);
		int16_t statoffset_y = 18;

		statoffsets[StatLabel::NAME] = statoffset;
		statoffsets[StatLabel::JOB] = statoffset + Point<int16_t>(1, statoffset_y * 1);
		statoffsets[StatLabel::GUILD] = statoffset + Point<int16_t>(1, statoffset_y * 2);
		statoffsets[StatLabel::FAME] = statoffset + Point<int16_t>(1, statoffset_y * 3);
		statoffsets[StatLabel::MIN_DAMAGE] = statoffset + Point<int16_t>(1, statoffset_y * 4);
		statoffsets[StatLabel::MAX_DAMAGE] = statoffset + Point<int16_t>(80, statoffset_y * 4);
		statoffsets[StatLabel::HP] = statoffset + Point<int16_t>(1, statoffset_y * 6);
		statoffsets[StatLabel::MP] = statoffset + Point<int16_t>(1, statoffset_y * 7);
		statoffsets[StatLabel::AP] = statoffset + Point<int16_t>(19, 167);
		statoffsets[StatLabel::STR] = statoffset + Point<int16_t>(1, 196);
		statoffsets[StatLabel::DEX] = statoffset + Point<int16_t>(1, 214);
		statoffsets[StatLabel::INT] = statoffset + Point<int16_t>(1, 232);
		statoffsets[StatLabel::LUK] = statoffset + Point<int16_t>(1, 250);

		// Detailed
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

		Point<int16_t> statoffset_detailed = Point<int16_t>(0, 77);
		int16_t statoffset_detailed_lx = 116;
		int16_t statoffset_detailed_rx = 227;

		statoffsets[StatLabel::MIN_DAMAGE_DETAILED] = Point<int16_t>(94, 41);
		statoffsets[StatLabel::MAX_DAMAGE_DETAILED] = Point<int16_t>(105, 59);

		statoffsets[StatLabel::DAMAGE_BONUS] = statoffset_detailed + Point<int16_t>(statoffset_detailed_lx, statoffset_y * 0);
		statoffsets[StatLabel::BOSS_DAMAGE] = statoffset_detailed + Point<int16_t>(statoffset_detailed_rx, statoffset_y * 0);

		statoffsets[StatLabel::FINAL_DAMAGE] = statoffset_detailed + Point<int16_t>(statoffset_detailed_lx, statoffset_y * 1);
		statoffsets[StatLabel::BUFF_DURATION] = statoffset_detailed + Point<int16_t>(statoffset_detailed_rx, statoffset_y * 1);

		statoffsets[StatLabel::IGNORE_DEFENSE] = statoffset_detailed + Point<int16_t>(statoffset_detailed_lx, statoffset_y * 2);
		statoffsets[StatLabel::ITEM_DROP_RATE] = statoffset_detailed + Point<int16_t>(statoffset_detailed_rx, statoffset_y * 2);

		statoffsets[StatLabel::CRITICAL_RATE] = statoffset_detailed + Point<int16_t>(statoffset_detailed_lx, statoffset_y * 3);
		statoffsets[StatLabel::MESOS_OBTAINED] = statoffset_detailed + Point<int16_t>(statoffset_detailed_rx, statoffset_y * 3);

		statoffsets[StatLabel::CRITICAL_DAMAGE] = statoffset_detailed + Point<int16_t>(statoffset_detailed_lx + 3, statoffset_y * 4);

		statoffsets[StatLabel::STATUS_RESISTANCE] = statoffset_detailed + Point<int16_t>(statoffset_detailed_lx, statoffset_y * 5);
		statoffsets[StatLabel::KNOCKBACK_RESISTANCE] = statoffset_detailed + Point<int16_t>(statoffset_detailed_rx, statoffset_y * 5);

		statoffsets[StatLabel::DEFENSE] = statoffset_detailed + Point<int16_t>(statoffset_detailed_lx, statoffset_y * 6);

		statoffsets[StatLabel::SPEED] = statoffset_detailed + Point<int16_t>(statoffset_detailed_lx, statoffset_y * 7);
		statoffsets[StatLabel::JUMP] = statoffset_detailed + Point<int16_t>(statoffset_detailed_lx, statoffset_y * 8);

		statoffsets[StatLabel::HONOR] = statoffset_detailed + Point<int16_t>(73, 263);

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
			// Calculate detail window position to align bottom with main window
			int16_t detail_height = textures_detail[0].get_dimensions().y();
			int16_t main_height = dimension.y();
			int16_t y_offset = main_height - detail_height;
			
			Point<int16_t> detail_pos(position + Point<int16_t>(212, y_offset));

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

			if (i >= StatLabel::NUM_NORMAL) {
				labelpos.shift_x(213);
				// Adjust for detail window vertical offset
				if (!textures_detail.empty()) {
					int16_t detail_height = textures_detail[0].get_dimensions().y();
					int16_t main_height = dimension.y();
					int16_t y_offset = main_height - detail_height;
					labelpos.shift_y(y_offset);
				}
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
			pos_adj = Point<int16_t>(211, 25);
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
		
		// Update positions of detail window buttons based on detail window position
		if (enabled && !textures_detail.empty()) {
			int16_t detail_height = textures_detail[0].get_dimensions().y();
			int16_t main_height = dimension.y();
			int16_t y_offset = main_height - detail_height;
			
			if (buttons[Buttons::BT_ABILITY]) {
				buttons[Buttons::BT_ABILITY]->set_position(Point<int16_t>(212, y_offset));
				buttons[Buttons::BT_ABILITY]->set_active(enabled);
			}
			if (buttons[Buttons::BT_DETAIL_DETAILCLOSE]) {
				buttons[Buttons::BT_DETAIL_DETAILCLOSE]->set_position(Point<int16_t>(212 + 165, y_offset + 290));
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