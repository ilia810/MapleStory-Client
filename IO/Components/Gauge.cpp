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
#include "Gauge.h"

namespace ms
{
	Gauge::Gauge(Type type, Texture front, int16_t maximum, float percentage) : Gauge(type, front, {}, maximum, percentage) {}
	Gauge::Gauge(Type type, Texture front, Texture middle, int16_t maximum, float percentage) : Gauge(type, front, {}, {}, maximum, percentage) {}
	Gauge::Gauge(Type type, Texture front, Texture middle, Texture end, int16_t maximum, float percentage) : type(type), barfront(front), barmid(middle), barend(end), maximum(maximum), percentage(percentage), target(percentage) {}

	void Gauge::draw(const DrawArgument& args) const
	{
		int16_t length = static_cast<int16_t>(percentage * maximum);

		if (length > 0)
		{
			if (type == Type::DEFAULT)
			{
				barfront.draw(args + DrawArgument(Point<int16_t>(0, 0), Point<int16_t>(length, 0)));
				barmid.draw(args);
				barend.draw(args + Point<int16_t>(length + 8, 20));
			}
			else if (type == Type::CASHSHOP)
			{
				Point<int16_t> pos_adj = Point<int16_t>(45, 1);

				barfront.draw(args - pos_adj);
				barmid.draw(args + DrawArgument(Point<int16_t>(0, 0), Point<int16_t>(length, 0)));
				barend.draw(args - pos_adj + Point<int16_t>(length + barfront.width(), 0));
			}
			else if (type == Type::WORLDSELECT)
			{
				barfront.draw(args, {}, Range<int16_t>(0, barfront.width() - length));
			}
			else if (type == Type::V87_FILL)
			{
				// V87: Draw texture clipped to the fill length, no stretching
				// Use Range to clip the texture to the desired width
				if (length <= barfront.width())
				{
					// Normal case: clip texture to show only the filled portion
					// Ensure full opacity by adding 1.0f to the draw arguments
					barfront.draw(args + 1.0f, {}, Range<int16_t>(0, length));
				}
				else
				{
					// Edge case: if length exceeds texture width, tile the texture
					int16_t remaining = length;
					int16_t offset = 0;
					while (remaining > 0)
					{
						int16_t tile_width = (remaining >= barfront.width()) ? barfront.width() : remaining;
						barfront.draw(args + Point<int16_t>(offset, 0) + 1.0f, {}, Range<int16_t>(0, tile_width));
						offset += tile_width;
						remaining -= tile_width;
					}
				}
			}
			else if (type == Type::V87_FILL_REVERSE)
			{
				// V87_FILL_REVERSE: Draw texture horizontally flipped
				// The flipped texture fills normally from left to right, but appears to fill right to left
				if (length <= barfront.width())
				{
					// Create a flipped draw argument by using the bool constructor
					// DrawArgument(position, flip) where flip = true for horizontal flip
					Point<int16_t> pos = args.getpos();
					DrawArgument flipped_args(pos, true);
					
					// Now draw with clipping to show only the filled portion
					barfront.draw(flipped_args, {}, Range<int16_t>(0, length));
				}
				else
				{
					// Edge case: if length exceeds texture width, just draw full texture flipped
					Point<int16_t> pos = args.getpos();
					DrawArgument flipped_args(pos, true);
					barfront.draw(flipped_args);
				}
			}
		}
		else
		{
			if (type == Type::WORLDSELECT)
				barfront.draw(args, {}, Range<int16_t>(0, barfront.width() - 1));
			// V87_FILL: Draw nothing when length is 0 (empty gauge)
		}
	}

	void Gauge::update(float t)
	{
		if (target != t)
		{
			target = t;
			step = (target - percentage) / 24;
		}

		if (percentage != target)
		{
			percentage += step;

			if (step < 0.0f)
			{
				if (target - percentage >= step)
					percentage = target;
			}
			else if (step > 0.0f)
			{
				if (target - percentage <= step)
					percentage = target;
			}
		}
	}

	bool Gauge::is_valid() const
	{
		// A gauge is valid if it has a valid front texture and a positive maximum
		return barfront.is_valid() && maximum > 0;
	}
}