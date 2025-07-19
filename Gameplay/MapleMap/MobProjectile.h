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

#include "../Physics/Physics.h"
#include "../../Graphics/Animation.h"
#include "../../Template/Point.h"
#include "../../Template/Interpolated.h"

namespace ms
{
	class MobProjectile
	{
	public:
		MobProjectile(const Animation& animation, Point<int16_t> origin, 
		              Point<int16_t> target, int16_t speed, int32_t mob_oid);

		// Update the projectile position
		// Returns true if the projectile should be removed
		bool update(const Physics& physics);
		
		// Draw the projectile
		void draw(double viewx, double viewy, float alpha) const;
		
		// Check if projectile hit the target area
		bool check_collision(Rectangle<int16_t> player_bounds) const;
		
		// Get the mob that created this projectile
		int32_t get_mob_oid() const { return mob_oid; }
		
		// Check if projectile is expired
		bool is_expired() const { return expired; }
		
		// Force expire the projectile
		void expire();  // Implementation in .cpp file

	private:
		Animation animation;
		Linear<float> x;
		Linear<float> y;
		Point<int16_t> target;
		float speed;
		int32_t mob_oid;
		bool expired;
		float angle;
		uint16_t lifetime;
		
		static constexpr uint16_t MAX_LIFETIME = 5000; // 5 seconds max
	};
}