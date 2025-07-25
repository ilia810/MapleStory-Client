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
#pragma once

#include "../../Graphics/Texture.h"

namespace ms
{
	class Gauge
	{
	public:
		enum Type : uint8_t
		{
			DEFAULT,
			CASHSHOP,
			WORLDSELECT,
			V87_FILL,  // New type for v87 single-texture horizontal fill
			V87_FILL_REVERSE  // Fill from right to left
		};

		Gauge() {}
		Gauge(Type type, Texture front, int16_t maximum, float percent);
		Gauge(Type type, Texture front, Texture middle, int16_t maximum, float percent);
		Gauge(Type type, Texture front, Texture middle, Texture end, int16_t maximum, float percentage);

		void draw(const DrawArgument& args) const;
		void update(float target);
		bool is_valid() const;

	private:
		Texture barfront;
		Texture barmid;
		Texture barend;
		int16_t maximum;

		float percentage;
		float target;
		float step;

		Type type;
	};
}