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
#include "Camera.h"

#include "../Constants.h"
#include "Stage.h"
#include <iostream>
#include <chrono>
#include <algorithm>

namespace ms
{
	Camera::Camera()
	{
		x.set(0.0);
		y.set(0.0);
		leftmost_platform = 0;
		rightmost_platform = 0;
		lowest_platform = 0;

		VWIDTH = Constants::Constants::get().get_viewwidth();
		VHEIGHT = Constants::Constants::get().get_viewheight();
	}

	void Camera::update(Point<int16_t> position)
	{
		int32_t new_width = Constants::Constants::get().get_viewwidth();
		int32_t new_height = Constants::Constants::get().get_viewheight();

		if (VWIDTH != new_width || VHEIGHT != new_height)
		{
			VWIDTH = new_width;
			VHEIGHT = new_height;
		}

		// Calculate desired camera target
		double raw_target_x = VWIDTH / 2 - position.x();
		double raw_target_y = VHEIGHT / 2 - position.y();

		double target_x = raw_target_x;
		double target_y = raw_target_y;

		// Push camera down to show more ground, less sky (player appears higher on screen)
		double void_reduction_offset = 130.0;
		target_y = target_y - void_reduction_offset;

		// Clamp bottom of view to not go below lowest platform + 200px
		// view_bottom (in map coords) = -target_y + VHEIGHT
		// We want: view_bottom <= lowest_platform + 200
		// So: -target_y + VHEIGHT <= lowest_platform + 200
		// => target_y >= VHEIGHT - lowest_platform - 200
		if (lowest_platform > -30000)
		{
			double max_view_bottom = lowest_platform + 200;
			double min_target_y = VHEIGHT - max_view_bottom;

			if (target_y < min_target_y)
				target_y = min_target_y;
		}

		// Clamp horizontal view to platform bounds
		// view_left (in map coords) = -target_x
		// view_right (in map coords) = -target_x + VWIDTH
		// We want: view_left >= leftmost_platform => target_x <= -leftmost_platform
		// We want: view_right <= rightmost_platform => target_x >= VWIDTH - rightmost_platform
		if (leftmost_platform < 30000 && rightmost_platform > -30000)
		{
			double max_target_x = -static_cast<double>(leftmost_platform);
			double min_target_x = VWIDTH - rightmost_platform;

			if (target_x > max_target_x)
				target_x = max_target_x;
			if (target_x < min_target_x)
				target_x = min_target_x;
		}

		// Now smooth toward target
		double next_x = x.get();
		double hdelta = target_x - next_x;
		next_x += hdelta * (12.0 / VWIDTH);

		double next_y = y.get();
		double vdelta = target_y - next_y;
		next_y += vdelta * (12.0 / VHEIGHT);

		x = next_x;
		y = next_y;
	}

	void Camera::set_position(Point<int16_t> position)
	{
		int32_t new_width = Constants::Constants::get().get_viewwidth();
		int32_t new_height = Constants::Constants::get().get_viewheight();

		if (VWIDTH != new_width || VHEIGHT != new_height)
		{
			VWIDTH = new_width;
			VHEIGHT = new_height;
		}

		double new_x = VWIDTH / 2 - position.x();
		double new_y = VHEIGHT / 2 - position.y();
		

		// Debug: Track direct position sets
		if (new_x == 0 && position.x() != VWIDTH / 2) {
			LOG(LOG_DEBUG, "[Camera] WARNING: set_position resulted in X=0! Player pos: " << position.x() << ", calculated X: " << new_x);
		}
		
		x.set(new_x);
		y.set(new_y);
	}

	void Camera::set_view(Range<int16_t> mapwalls, Range<int16_t> mapborders,
	                      int16_t platform_left, int16_t platform_right, int16_t platform_bottom)
	{
		// Fix: Don't negate the ranges - use them directly
		// The negation was causing inverted bounds
		hbounds = mapwalls;
		vbounds = mapborders;
		leftmost_platform = platform_left;
		rightmost_platform = platform_right;
		lowest_platform = platform_bottom;
	}

	Point<int16_t> Camera::position() const
	{
		auto shortx = static_cast<int16_t>(std::round(x.get()));
		auto shorty = static_cast<int16_t>(std::round(y.get()));
		
		// Debug: Check if conversion is causing issues
		if (std::abs(x.get() - shortx) > 1.0) {
			LOG(LOG_DEBUG, "[Camera] Large rounding difference in position() - double: " << x.get() << ", int16: " << shortx);
		}

		return { shortx, shorty };
	}

	Point<int16_t> Camera::position(float alpha) const
	{
		auto interx = static_cast<int16_t>(std::round(x.get(alpha)));
		auto intery = static_cast<int16_t>(std::round(y.get(alpha)));

		return { interx, intery };
	}

	Point<double> Camera::realposition(float alpha) const
	{
		return { x.get(alpha), y.get(alpha) };
	}
}