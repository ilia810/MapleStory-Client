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
#include "Configuration.h"

#include "Util/Misc.h"

#include <fstream>

namespace ms
{
	Configuration::Configuration()
	{
		settings.emplace<ServerIP>();
		settings.emplace<ServerPort>();
		settings.emplace<Fullscreen>();
		settings.emplace<Width>();
		settings.emplace<Height>();
		settings.emplace<VSync>();
		settings.emplace<Monitor>();
		settings.emplace<FontPathNormal>();
		settings.emplace<FontPathBold>();
		settings.emplace<BGMVolume>();
		settings.emplace<SFXVolume>();
		settings.emplace<SaveLogin>();
		settings.emplace<DefaultAccount>();
		settings.emplace<DefaultWorld>();
		settings.emplace<DefaultChannel>();
		settings.emplace<DefaultRegion>();
		settings.emplace<DefaultCharacter>();
		settings.emplace<AutoLogin>();
		settings.emplace<AutoAccount>();
		settings.emplace<AutoPassword>();
		settings.emplace<AutoWorld>();
		settings.emplace<AutoChannel>();
		settings.emplace<AutoCharacter>();
		settings.emplace<AutoPIC>();
		settings.emplace<ChatViewMax>();
		settings.emplace<ChatViewX>();
		settings.emplace<ChatViewY>();
		settings.emplace<PosSTATS>();
		settings.emplace<PosEQINV>();
		settings.emplace<PosINV>();
		settings.emplace<PosSKILL>();
		settings.emplace<PosQUEST>();
		settings.emplace<PosMAP>();
		settings.emplace<PosUSERLIST>();
		settings.emplace<PosCHAT>();
		settings.emplace<PosMINIMAP>();
		settings.emplace<PosSHOP>();
		settings.emplace<PosNOTICE>();
		settings.emplace<PosMAPLECHAT>();
		settings.emplace<PosCHANNEL>();
		settings.emplace<PosJOYPAD>();
		settings.emplace<PosEVENT>();
		settings.emplace<PosKEYCONFIG>();
		settings.emplace<PosOPTIONMENU>();
		settings.emplace<PosCHARINFO>();
		settings.emplace<MiniMapType>();
		settings.emplace<MiniMapSimpleMode>();
		settings.emplace<MiniMapDefaultHelpers>();

		load();
	}

	Configuration::~Configuration()
	{
#ifndef _DEBUG
		save();
#endif
	}

	void Configuration::load()
	{
		std::unordered_map<std::string, std::string> rawsettings;
		std::ifstream file(FILENAME);

		LOG(LOG_DEBUG, "[Configuration] Loading settings from: " + std::string(FILENAME));

		if (file.is_open())
		{
			// Go through the file line by line
			std::string line;

			while (getline(file, line))
			{
				// If the setting is not empty, load the value.
				size_t split = line.find('=');

				if (split != std::string::npos && split + 1 < line.size())
				{
					std::string key = line.substr(0, split);
					std::string value = line.substr(split + 1);
					
					// Trim whitespace from key and value
					key.erase(key.find_last_not_of(" \t\r\n") + 1);
					key.erase(0, key.find_first_not_of(" \t\r\n"));
					value.erase(value.find_last_not_of(" \t\r\n") + 1);
					value.erase(0, value.find_first_not_of(" \t\r\n"));
					
					rawsettings.emplace(key, value);
					
					// Log auto-login related settings
					if (key == "AutoLogin" || key == "AutoAccount" || key == "AutoPassword" || 
					    key == "AutoWorld" || key == "AutoChannel" || key == "AutoCharacter")
					{
						LOG(LOG_DEBUG, "[Configuration] Loaded setting: " + key + " = " + value);
					}
				}
			}
		}
		else
		{
			LOG(LOG_ERROR, "[Configuration] Failed to open settings file: " + std::string(FILENAME));
		}

		// Replace default values with loaded values
		for (auto& setting : settings)
		{
			auto rsiter = rawsettings.find(setting.second->name);

			if (rsiter != rawsettings.end())
			{
				setting.second->value = rsiter->second;
			}
		}
		
		// Verify auto-login settings were loaded
		LOG(LOG_DEBUG, "[Configuration] Auto-login settings after load:");
		LOG(LOG_DEBUG, "  AutoLogin = " + Setting<AutoLogin>::get().value);
		LOG(LOG_DEBUG, "  AutoAccount = " + Setting<AutoAccount>::get().value);
		LOG(LOG_DEBUG, "  AutoWorld = " + Setting<AutoWorld>::get().value);
		LOG(LOG_DEBUG, "  AutoChannel = " + Setting<AutoChannel>::get().value);
		LOG(LOG_DEBUG, "  AutoCharacter = " + Setting<AutoCharacter>::get().value);
	}

	void Configuration::save() const
	{
		// Open the settings file
		std::ofstream config(FILENAME);

		if (config.is_open())
		{
			// Save settings line by line
			for (auto& setting : settings)
				config << setting.second->to_string() << std::endl;
		}
	}

	void Configuration::BoolEntry::save(bool b)
	{
		value = b ? "true" : "false";
	}

	bool Configuration::BoolEntry::load() const
	{
		return value == "true";
	}

	void Configuration::StringEntry::save(std::string str)
	{
		value = str;
	}

	std::string Configuration::StringEntry::load() const
	{
		return value;
	}

	void Configuration::PointEntry::save(Point<int16_t> vec)
	{
		value = vec.to_string();
	}

	Point<int16_t> Configuration::PointEntry::load() const
	{
		std::string xstr = value.substr(1, value.find(",") - 1);
		std::string ystr = value.substr(value.find(",") + 1, value.find(")") - value.find(",") - 1);

		auto x = string_conversion::or_zero<int16_t>(xstr);
		auto y = string_conversion::or_zero<int16_t>(ystr);

		return { x, y };
	}

	bool Configuration::get_show_fps() const
	{
		return SHOW_FPS;
	}

	bool Configuration::get_show_packets() const
	{
		return SHOW_PACKETS;
	}

	bool Configuration::get_auto_login() const
	{
		bool auto_login = Setting<AutoLogin>::get().load();
		LOG(LOG_DEBUG, "[Configuration] get_auto_login() returning: " + std::string(auto_login ? "true" : "false"));
		LOG(LOG_DEBUG, "[Configuration] AutoLogin raw value: " + Setting<AutoLogin>::get().value);
		return auto_login;
	}

	uint8_t Configuration::get_auto_world()
	{
		return Setting<AutoWorld>::get().load();
	}

	uint8_t Configuration::get_auto_channel()
	{
		return Setting<AutoChannel>::get().load();
	}

	std::string Configuration::get_auto_acc()
	{
		return Setting<AutoAccount>::get().load();
	}

	std::string Configuration::get_auto_pass()
	{
		return Setting<AutoPassword>::get().load();
	}

	std::string Configuration::get_auto_pic()
	{
		return Setting<AutoPIC>::get().load();
	}

	int32_t Configuration::get_auto_cid()
	{
		return Setting<AutoCharacter>::get().load();
	}

	std::string Configuration::get_title() const
	{
		return TITLE;
	}

	std::string Configuration::get_version() const
	{
		return VERSION;
	}

	std::string Configuration::get_login_music() const
	{
		return LoginMusic;
	}

	std::string Configuration::get_login_music_sea() const
	{
		return LoginMusicSEA;
	}

	std::string Configuration::get_login_music_newtro() const
	{
		return LoginMusicNewtro;
	}

	std::string Configuration::get_joinlink() const
	{
		return JOINLINK;
	}

	std::string Configuration::get_website() const
	{
		return WEBSITE;
	}

	std::string Configuration::get_findid() const
	{
		return FINDID;
	}

	std::string Configuration::get_findpass() const
	{
		return FINDPASS;
	}

	std::string Configuration::get_resetpic() const
	{
		return RESETPIC;
	}

	std::string Configuration::get_chargenx() const
	{
		return CHARGENX;
	}

	void Configuration::set_macs(char* macs)
	{
		MACS = macs;
	}

	void Configuration::set_hwid(char* hwid, char* volumeSerialNumber)
	{
		VolumeSerialNumber = volumeSerialNumber;

		std::string newHWID;

		newHWID.append(hwid);
		newHWID.append("_");

		std::string part1 = VolumeSerialNumber.substr(0, 2);
		std::string part2 = VolumeSerialNumber.substr(2, 2);
		std::string part3 = VolumeSerialNumber.substr(4, 2);
		std::string part4 = VolumeSerialNumber.substr(6, 2);

		// TODO: VolumeSerialNumber was not eight characters long, added fix for last part.
		// TODO: Need to look into why the VolumeSerialNumber was seven characters long
		part4 = string_format::pad_string(part4, 2);

		newHWID.append(part4);
		newHWID.append(part3);
		newHWID.append(part2);
		newHWID.append(part1);

		HWID = newHWID;
	}

	void Configuration::set_max_width(int16_t max_width)
	{
		MAXWIDTH = max_width;
	}

	void Configuration::set_max_height(int16_t max_height)
	{
		MAXHEIGHT = max_height;
	}

	std::string Configuration::get_macs()
	{
		return MACS;
	}

	std::string Configuration::get_hwid()
	{
		return HWID;
	}

	std::string Configuration::get_vol_serial_num()
	{
		return VolumeSerialNumber;
	}

	int16_t Configuration::get_max_width()
	{
		return MAXWIDTH;
	}

	int16_t Configuration::get_max_height()
	{
		return MAXHEIGHT;
	}

	bool Configuration::get_rightclicksell()
	{
		return rightclicksell;
	}

	void Configuration::set_rightclicksell(bool value)
	{
		rightclicksell = value;
	}

	bool Configuration::get_show_weekly()
	{
		return show_weekly;
	}

	void Configuration::set_show_weekly(bool value)
	{
		show_weekly = value;
	}

	bool Configuration::get_start_shown()
	{
		return start_shown;
	}

	void Configuration::set_start_shown(bool value)
	{
		start_shown = value;
	}

	uint8_t Configuration::get_worldid()
	{
		return worldid;
	}

	void Configuration::set_worldid(uint8_t id)
	{
		worldid = id;
	}

	uint8_t Configuration::get_channelid()
	{
		return channelid;
	}

	void Configuration::set_channelid(uint8_t id)
	{
		channelid = id;
	}

	bool Configuration::get_admin()
	{
		return admin;
	}

	void Configuration::set_admin(bool value)
	{
		admin = value;
	}

	bool Configuration::get_caps_lock_enabled()
	{
		return caps_lock_enabled;
	}

	void Configuration::set_caps_lock_enabled(int value)
	{
		caps_lock_enabled = value;
	}
}