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
#include "MapPortals.h"

#include "../../Constants.h"

#include "../../Util/Misc.h"

#ifdef USE_NX
#include <nlnx/nx.hpp>
#endif

namespace ms
{
	MapPortals::MapPortals(nl::node src, int32_t mapid)
	{
		LOG(LOG_DEBUG, "[MapPortals] Loading portals for map " << mapid);
		
		for (auto sub : src)
		{
			int8_t portal_id = string_conversion::or_default<int8_t>(sub.name(), -1);

			if (portal_id < 0) {
				LOG(LOG_DEBUG, "[MapPortals] Skipping portal with invalid ID: " << sub.name());
				continue;
			}

			Portal::Type type = Portal::typebyid(sub["pt"]);
			std::string name = sub["pn"];
			std::string target_name = sub["tn"];
			int32_t target_id = sub["tm"];
			Point<int16_t> position = { sub["x"], sub["y"] };

			LOG(LOG_DEBUG, "[MapPortals] Loading portal " << (int)portal_id << ": name='" << name 
				<< "', type=" << (int)type << ", position=(" << position.x() << "," << position.y() 
				<< "), target_map=" << target_id << ", target_name='" << target_name << "'");

			const Animation* animation = &animations[type];
			bool intramap = target_id == mapid;

			portals_by_id.emplace(
				std::piecewise_construct,
				std::forward_as_tuple(portal_id),
				std::forward_as_tuple(animation, type, name, intramap, position, target_id, target_name)
			);

			portal_ids_by_name.emplace(name, portal_id);
		}

		LOG(LOG_DEBUG, "[MapPortals] Loaded " << portals_by_id.size() << " portals total");
		cooldown = WARPCD;
	}

	MapPortals::MapPortals()
	{
		cooldown = WARPCD;
	}

	void MapPortals::update(Point<int16_t> playerpos)
	{
		animations[Portal::REGULAR].update(Constants::TIMESTEP);
		animations[Portal::HIDDEN].update(Constants::TIMESTEP);

		for (auto& iter : portals_by_id)
		{
			Portal& portal = iter.second;
			switch (portal.get_type())
			{
			case Portal::HIDDEN:
			case Portal::TOUCH:
				portal.update(playerpos);
				break;
			}
		}

		if (cooldown > 0)
			cooldown--;
	}

	void MapPortals::draw(Point<int16_t> viewpos, float inter) const
	{
		for (auto& ptit : portals_by_id)
			ptit.second.draw(viewpos, inter);
	}

	Point<int16_t> MapPortals::get_portal_by_id(uint8_t portal_id) const
	{
		auto iter = portals_by_id.find(portal_id);

		if (iter != portals_by_id.end())
		{
			constexpr Point<int16_t> ABOVE(0, 30);
			Point<int16_t> result = iter->second.get_position() - ABOVE;
			LOG(LOG_DEBUG, "[MapPortals] Found portal ID " << (int)portal_id 
				<< ": name='" << iter->second.get_name() << "', position=(" 
				<< iter->second.get_position().x() << "," << iter->second.get_position().y() 
				<< "), adjusted spawn=(" << result.x() << "," << result.y() << ")");
			return result;
		}
		else
		{
			LOG(LOG_DEBUG, "[MapPortals] ERROR: Portal ID " << (int)portal_id << " not found! Available portals:");
			for (const auto& portal : portals_by_id) {
				LOG(LOG_DEBUG, "  Portal ID " << (int)portal.first << ": name='" << portal.second.get_name() << "'");
			}
			return {};
		}
	}

	Point<int16_t> MapPortals::get_portal_by_name(const std::string& portal_name) const
	{
		auto iter = portal_ids_by_name.find(portal_name);

		if (iter != portal_ids_by_name.end()) {
			LOG(LOG_DEBUG, "[MapPortals] Found portal by name '" << portal_name << "' -> ID " << (int)iter->second);
			return get_portal_by_id(iter->second);
		} else {
			LOG(LOG_DEBUG, "[MapPortals] ERROR: Portal name '" << portal_name << "' not found! Available portal names:");
			for (const auto& name_pair : portal_ids_by_name) {
				LOG(LOG_DEBUG, "  Portal name '" << name_pair.first << "' -> ID " << (int)name_pair.second);
			}
			return {};
		}
	}

	Portal::WarpInfo MapPortals::find_warp_at(Point<int16_t> playerpos)
	{
		if (cooldown == 0)
		{
			cooldown = WARPCD;

			for (auto& iter : portals_by_id)
			{
				const Portal& portal = iter.second;

				if (portal.bounds().contains(playerpos))
				{
					Portal::WarpInfo info = portal.getwarpinfo();
					LOG(LOG_DEBUG, "[MapPortals] Portal found at position - name: " << info.name 
						<< ", type: " << (int)portal.get_type() 
						<< ", to map: " << info.mapid 
						<< ", valid: " << info.valid);
					return info;
				}
			}
			
			LOG(LOG_DEBUG, "[MapPortals] No portal found at position " << playerpos.x() << "," << playerpos.y() 
				<< " (checked " << portals_by_id.size() << " portals)");
		}
		else
		{
			// LOG(LOG_DEBUG, "[MapPortals] Portal check on cooldown: " << cooldown);
		}

		return {};
	}

	void MapPortals::init()
	{
		nl::node src = nl::nx::Map["MapHelper.img"]["portal"]["game"];
		
		LOG(LOG_DEBUG, "[MapPortals] Initializing portal animations from MapHelper.img");

		// Check if the portal nodes exist
		if (src.name().empty()) {
			LOG(LOG_DEBUG, "[MapPortals] WARNING: MapHelper.img/portal/game node not found, portals may not display");
			return;
		}
		
		// Try to load animations with v83/v87 fallbacks
		nl::node hidden_anim = src["ph"]["default"]["portalContinue"];
		nl::node regular_anim = src["pv"]["default"];
		
		// For v83/v87 compatibility, try alternative paths if main ones fail
		if (hidden_anim.name().empty()) {
			hidden_anim = src["ph"]["default"]["portal"];
		}
		if (regular_anim.name().empty()) {
			regular_anim = src["pv"]["default"]["portal"];
		}
		
		if (!hidden_anim.name().empty()) {
			animations[Portal::HIDDEN] = hidden_anim;
			LOG(LOG_DEBUG, "[MapPortals] Loaded HIDDEN portal animation");
		} else {
			LOG(LOG_DEBUG, "[MapPortals] WARNING: HIDDEN portal animation not found - portals may be invisible");
		}
		
		if (!regular_anim.name().empty()) {
			animations[Portal::REGULAR] = regular_anim;
			LOG(LOG_DEBUG, "[MapPortals] Loaded REGULAR portal animation");
		} else {
			LOG(LOG_DEBUG, "[MapPortals] WARNING: REGULAR portal animation not found - portals may be invisible");
		}
		
		// Also load INVISIBLE portal animation (same as HIDDEN for most cases)
		animations[Portal::INVISIBLE] = animations[Portal::HIDDEN];
		
		LOG(LOG_DEBUG, "[MapPortals] Portal animation initialization complete");
	}

	std::unordered_map<Portal::Type, Animation> MapPortals::animations;
}