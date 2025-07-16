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
#include "MapInfo.h"

#include "../../Util/Misc.h"
#include "../../Constants.h"

namespace ms
{
	MapInfo::MapInfo(nl::node src, Range<int16_t> walls, Range<int16_t> borders)
	{
		nl::node info = src["info"];

		// Check for VR bounds in map info
		bool has_vr_bounds = (info["VRLeft"].data_type() == nl::node::type::integer) && 
		                     (info["VRRight"].data_type() == nl::node::type::integer) &&
		                     (info["VRTop"].data_type() == nl::node::type::integer) && 
		                     (info["VRBottom"].data_type() == nl::node::type::integer);
		
		// Also check if VR values are not zero (some maps have zero VR bounds)
		if (has_vr_bounds) {
			int16_t vr_left = info["VRLeft"];
			int16_t vr_right = info["VRRight"];
			int16_t vr_top = info["VRTop"];
			int16_t vr_bottom = info["VRBottom"];
			
			bool vr_values_valid = (vr_left != 0 || vr_right != 0 || vr_top != 0 || vr_bottom != 0) &&
			                      (vr_left < vr_right) && (vr_top < vr_bottom);
			
			if (vr_values_valid) {
				mapwalls = Range<int16_t>(vr_left, vr_right);
				mapborders = Range<int16_t>(vr_top, vr_bottom);
				LOG(LOG_DEBUG, "[MapInfo] Using map VR bounds - Walls: [" << mapwalls.smaller() << "," << mapwalls.greater() 
					<< "], Borders: [" << mapborders.smaller() << "," << mapborders.greater() << "]");
			} else {
				has_vr_bounds = false; // Treat as missing VR bounds
				LOG(LOG_DEBUG, "[MapInfo] VR bounds exist but are invalid (zeros or inverted), calculating from footholds");
			}
		}
		
		if (!has_vr_bounds) {
			// V83/V87 fallback: Use researcher's camera rect algorithm
			LOG(LOG_DEBUG, "[MapInfo] No valid VR bounds found, using researcher's camera rect algorithm for v83/v87");
			
			// Get current viewport size
			int16_t viewW = Constants::Constants::get().get_viewwidth();
			int16_t viewH = Constants::Constants::get().get_viewheight();
			
			// Use the new camera rect derivation
			LOG(LOG_DEBUG, "[MapInfo] Input physics bounds - Walls: [" << walls.smaller() << "," << walls.greater() 
				<< "], Borders: [" << borders.smaller() << "," << borders.greater() << "]");
			
			CameraRect camRect = derive_camera_rect(src, walls, borders, viewW, viewH);
			mapwalls = Range<int16_t>(camRect.left, camRect.right);
			mapborders = Range<int16_t>(camRect.top, camRect.bottom);
			
			LOG(LOG_DEBUG, "[MapInfo] Using researcher's camera rect - Walls: [" << mapwalls.smaller() << "," << mapwalls.greater() 
				<< "], Borders: [" << mapborders.smaller() << "," << mapborders.greater() << "]");
		}

		std::string bgmpath = info["bgm"];
		size_t split = bgmpath.find('/');
		bgm = bgmpath.substr(0, split) + ".img/" + bgmpath.substr(split + 1);

		cloud = info["cloud"].get_bool();
		fieldlimit = info["fieldLimit"];
		hideminimap = info["hideMinimap"].get_bool();
		mapmark = info["mapMark"];
		swim = info["swim"].get_bool();
		town = info["town"].get_bool();

		for (auto seat : src["seat"])
			seats.push_back(seat);

		for (auto ladder : src["ladderRope"])
			ladders.push_back(ladder);
	}

	MapInfo::MapInfo() {}

	Range<int16_t> MapInfo::calculate_vr_from_footholds(nl::node src, Range<int16_t> fallback_walls) const
	{
		nl::node foothold_src = src["foothold"];
		if (foothold_src.name().empty()) {
			LOG(LOG_DEBUG, "[MapInfo] No foothold data found, using fallback walls");
			return fallback_walls;
		}

		int16_t leftmost = 30000;
		int16_t rightmost = -30000;
		bool found_footholds = false;

		// Iterate through all footholds to find envelope
		for (auto layer : foothold_src) {
			for (auto group : layer) {
				for (auto fh : group) {
					found_footholds = true;
					
					int16_t x1 = fh["x1"];
					int16_t x2 = fh["x2"];
					
					leftmost = std::min(leftmost, std::min(x1, x2));
					rightmost = std::max(rightmost, std::max(x1, x2));
				}
			}
		}

		if (!found_footholds) {
			LOG(LOG_DEBUG, "[MapInfo] No footholds found in data, using fallback walls");
			return fallback_walls;
		}

		// Add margin as per v83 private server practice
		// Need very large margins because players can move far beyond footholds
		// Based on edge cases, player at 5782 needs camera at -4623
		int16_t margin = 5000;  // Very large margin to handle extreme player positions
		leftmost -= margin;
		rightmost += margin;

		LOG(LOG_DEBUG, "[MapInfo] Calculated VR walls from footholds: [" << leftmost << "," << rightmost << "]");
		return Range<int16_t>(leftmost, rightmost);
	}

	Range<int16_t> MapInfo::calculate_vr_borders_from_footholds(nl::node src, Range<int16_t> fallback_borders) const
	{
		nl::node foothold_src = src["foothold"];
		if (foothold_src.name().empty()) {
			LOG(LOG_DEBUG, "[MapInfo] No foothold data found, using fallback borders");
			return fallback_borders;
		}

		int16_t topmost = 30000;
		int16_t bottommost = -30000;
		bool found_footholds = false;

		// Iterate through all footholds to find envelope
		for (auto layer : foothold_src) {
			for (auto group : layer) {
				for (auto fh : group) {
					found_footholds = true;
					
					int16_t y1 = fh["y1"];
					int16_t y2 = fh["y2"];
					
					topmost = std::min(topmost, std::min(y1, y2));
					bottommost = std::max(bottommost, std::max(y1, y2));
				}
			}
		}

		if (!found_footholds) {
			LOG(LOG_DEBUG, "[MapInfo] No footholds found in data, using fallback borders");
			return fallback_borders;
		}

		// Add margin as per v83 private server practice
		// We need to be careful with margins to avoid showing void while allowing camera movement
		// The camera center needs to be able to reach areas where the player can stand
		
		// Get current viewport once – this is always available because Constants
		// is initialised before Stage loads the map.
		int16_t viewport_height = Constants::Constants::get().get_viewheight();
		int16_t half_viewport = viewport_height / 2;
		
		// For top: When player is at ground level, camera needs to go high (negative Y)
		// Add generous margin for when player is at bottom of map
		int16_t top_margin = 1500;
		topmost -= top_margin;
		
		// For bottom: We need to balance showing minimal void while allowing camera movement
		// The key insight: we want the VIEWPORT bottom (camera_y + viewport/2) to stay near footholds
		// So the maximum camera Y should be: bottommost_foothold + margin - viewport/2
		// This way viewport bottom = bottommost_foothold + margin
		const int16_t TILE = 64;          // visual safety margin
		int16_t bottom_margin = half_viewport - TILE;   // viewport bottom ≤ foothold + TILE
		bottommost += bottom_margin;

		int16_t viewport_bottom_at_max_camera = bottommost - half_viewport;
		int16_t void_pixels = viewport_bottom_at_max_camera - (bottommost - bottom_margin);
		
		LOG(LOG_DEBUG, "[MapInfo] Foothold Y range: [" << (topmost + top_margin) << "," << (bottommost - bottom_margin) 
			<< "], margins: top=" << top_margin << ", bottom=" << bottom_margin 
			<< ", final VR borders: [" << topmost << "," << bottommost << "]"
			<< ", void below map: ~" << void_pixels << " pixels");
		return Range<int16_t>(topmost, bottommost);
	}

	MapInfo::CameraRect MapInfo::derive_camera_rect(nl::node src, Range<int16_t> physWalls, Range<int16_t> physBorders, int16_t viewW, int16_t viewH) const
	{
		LOG(LOG_DEBUG, "[MapInfo] SIMPLIFIED - Only calculating ground level for void elimination");
		
		// We only care about ground level for void elimination now
		// The camera will ignore all bounds except the bottom void clamp
		CameraRect cam;
		
		// Store ground level from physics bounds - this is what matters for void elimination
		int16_t ground_level = physBorders.greater(); // Lowest foothold Y (ground level)
		
		// Set bottom bound for ZERO void elimination - this is the only bound that will be used
		// Camera bottom bound = ground - viewport_half
		// This ensures viewport bottom = ground when camera is at this bound (no void at all)
		cam.bottom = ground_level - (viewH/2);
		
		// Set other bounds very wide so they're effectively disabled
		cam.left = -30000;
		cam.right = 30000;
		cam.top = -30000;
		// cam.bottom already set above for void elimination
		
		LOG(LOG_DEBUG, "[MapInfo] SIMPLIFIED bounds - Only bottom matters:");
		LOG(LOG_DEBUG, "  Ground level: " << ground_level);
		LOG(LOG_DEBUG, "  Bottom bound (ZERO void): " << cam.bottom);
		LOG(LOG_DEBUG, "  When camera at bottom bound, viewport bottom = " << (cam.bottom + viewH/2) << " (should equal ground)");
		LOG(LOG_DEBUG, "  All other bounds set to ±30000 (effectively unlimited)");
		
		return cam;
	}

	bool MapInfo::is_underwater() const
	{
		return swim;
	}

	std::string MapInfo::get_bgm() const
	{
		return bgm;
	}

	Range<int16_t> MapInfo::get_walls() const
	{
		return mapwalls;
	}

	Range<int16_t> MapInfo::get_borders() const
	{
		return mapborders;
	}

	Optional<const Seat> MapInfo::findseat(Point<int16_t> position) const
	{
		for (auto& seat : seats)
			if (seat.inrange(position))
				return seat;

		return nullptr;
	}

	Optional<const Ladder> MapInfo::findladder(Point<int16_t> position, bool upwards) const
	{
		for (auto& ladder : ladders)
			if (ladder.inrange(position, upwards))
				return ladder;

		return nullptr;
	}

	Seat::Seat(nl::node src)
	{
		pos = src;
	}

	bool Seat::inrange(Point<int16_t> position) const
	{
		auto hor = Range<int16_t>::symmetric(position.x(), 10);
		auto ver = Range<int16_t>::symmetric(position.y(), 10);

		return hor.contains(pos.x()) && ver.contains(pos.y());
	}

	Point<int16_t> Seat::getpos() const
	{
		return pos;
	}

	Ladder::Ladder(nl::node src)
	{
		x = src["x"];
		y1 = src["y1"];
		y2 = src["y2"];
		ladder = src["l"].get_bool();
	}

	bool Ladder::is_ladder() const
	{
		return ladder;
	}

	bool Ladder::inrange(Point<int16_t> position, bool upwards) const
	{
		auto hor = Range<int16_t>::symmetric(position.x(), 10);
		auto ver = Range<int16_t>(y1, y2);

		int16_t y = upwards ?
			position.y() - 5 :
			position.y() + 5;

		return hor.contains(x) && ver.contains(y);
	}

	bool Ladder::felloff(int16_t y, bool downwards) const
	{
		int16_t dy = downwards ? y + 5 : y - 5;

		return dy > y2 || y + 5 < y1;
	}

	int16_t Ladder::get_x() const
	{
		return x;
	}
}