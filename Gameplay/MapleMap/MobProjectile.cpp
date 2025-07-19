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
#include "MobProjectile.h"

#include "../../Constants.h"

#include <iostream>
#define _USE_MATH_DEFINES
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace ms
{
	MobProjectile::MobProjectile(const Animation& anim, Point<int16_t> origin, 
	                             Point<int16_t> tgt, int16_t spd, int32_t oid) :
		animation(anim), target(tgt), speed(spd * 0.001f), mob_oid(oid), 
		expired(false), lifetime(0)
	{
		x.set(static_cast<float>(origin.x()));
		y.set(static_cast<float>(origin.y()));
		
		// Calculate angle to target
		float dx = tgt.x() - origin.x();
		float dy = tgt.y() - origin.y();
		angle = std::atan2(dy, dx);
		
		// Debug: Log projectile creation details
		std::cout << "[DEBUG] Projectile created - Origin: (" << origin.x() << ", " << origin.y() 
		          << ") Target: (" << tgt.x() << ", " << tgt.y() 
		          << ") Speed: " << speed << " Angle: " << angle << std::endl;
	}

	bool MobProjectile::update(const Physics& physics)
	{
		if (expired)
		{
			std::cout << "[DEBUG] Projectile already expired" << std::endl;
			return true;
		}
			
		// Update animation
		animation.update();
		
		// Move projectile towards target
		float dx = std::cos(angle) * speed * Constants::TIMESTEP;
		float dy = std::sin(angle) * speed * Constants::TIMESTEP;
		
		x += dx;
		y += dy;
		
		static int update_count = 0;
		if (update_count++ % 30 == 0) // Log every 30 updates
		{
			std::cout << "[DEBUG] Projectile update - pos: (" << x.get() << ", " << y.get() 
			          << ") velocity: (" << dx << ", " << dy << ") angle: " << angle << std::endl;
		}
		
		// Check if reached target or exceeded lifetime
		Point<int16_t> current_pos(static_cast<int16_t>(x.get()), 
		                           static_cast<int16_t>(y.get()));
		
		float dist_to_target = std::sqrt(
			std::pow(target.x() - current_pos.x(), 2) + 
			std::pow(target.y() - current_pos.y(), 2)
		);
		
		lifetime += Constants::TIMESTEP;
		
		if (dist_to_target < 20.0f)
		{
			std::cout << "[DEBUG] Projectile reached target at distance: " << dist_to_target << std::endl;
			expired = true;
			return true;
		}
		
		if (lifetime > MAX_LIFETIME)
		{
			std::cout << "[DEBUG] Projectile exceeded lifetime: " << lifetime << " > " << MAX_LIFETIME << std::endl;
			expired = true;
			return true;
		}
		
		return false;
	}
	
	void MobProjectile::draw(double viewx, double viewy, float alpha) const
	{
		if (expired)
			return;
			
		// Get the interpolated world position
		float world_x = x.get(alpha);
		float world_y = y.get(alpha);
		
		// Convert world coordinates to screen coordinates
		// The view position is negative, so we add it to get screen position
		Point<int16_t> absp(
			static_cast<int16_t>(std::round(world_x + viewx)),
			static_cast<int16_t>(std::round(world_y + viewy))
		);
		
		// Check if projectile is on screen
		const int SCREEN_WIDTH = 800;
		const int SCREEN_HEIGHT = 600;
		const int MARGIN = 100; // Extra margin for projectiles partially on screen
		
		if (absp.x() < -MARGIN || absp.x() > SCREEN_WIDTH + MARGIN ||
		    absp.y() < -MARGIN || absp.y() > SCREEN_HEIGHT + MARGIN)
		{
			// Projectile is off-screen, don't draw
			static int offscreen_count = 0;
			if (offscreen_count++ % 60 == 0)
			{
				std::cout << "[DEBUG] Projectile off-screen at (" << absp.x() << ", " << absp.y() << ")" << std::endl;
			}
			// Don't return - still draw it for debugging
			// return;
		}
		
		static int draw_count = 0;
		if (draw_count++ % 60 == 0) // Log every 60 frames
		{
			std::cout << "[DEBUG] Projectile world pos: (" << world_x << ", " << world_y << ")" << std::endl;
			std::cout << "[DEBUG] Camera pos: (" << viewx << ", " << viewy << ")" << std::endl;
			std::cout << "[DEBUG] Drawing projectile at screen pos: (" 
			      << absp.x() << ", " << absp.y() << ")" << std::endl;
		}
		
		// Calculate if projectile should be flipped based on direction
		// Flip it the opposite way to fix the visual
		bool flip = !(angle > M_PI/2 || angle < -M_PI/2);
		
		// Draw the projectile animation at its position
		// The animation system should handle the sprite origin automatically
		DrawArgument args(absp, flip);
		animation.draw(args, alpha);
		
		// Debug: Log when projectile is visible on screen
		static int marker_count = 0;
		if (marker_count++ % 30 == 0) // Every 30 frames
		{
			if (absp.x() >= 0 && absp.x() <= SCREEN_WIDTH && 
			    absp.y() >= 0 && absp.y() <= SCREEN_HEIGHT)
			{
				std::cout << "[DEBUG] Projectile VISIBLE at screen pos (" << absp.x() << ", " << absp.y() 
				          << ") world pos (" << world_x << ", " << world_y << ")" << std::endl;
			}
		}
	}
	
	bool MobProjectile::check_collision(Rectangle<int16_t> player_bounds) const
	{
		if (expired)
			return false;
			
		Point<int16_t> pos(static_cast<int16_t>(x.get()), 
		                   static_cast<int16_t>(y.get()));
		
		// Debug collision check
		static int collision_check_count = 0;
		if (collision_check_count++ % 120 == 0) // Less frequent logging
		{
			std::cout << "[DEBUG] Checking collision - Projectile at (" << pos.x() << ", " << pos.y() 
			          << ") vs Player bounds [L:" << player_bounds.left() << ", R:" << player_bounds.right() 
			          << ", T:" << player_bounds.top() << ", B:" << player_bounds.bottom() << "]" << std::endl;
		}
		
		// Simple point-in-rectangle collision
		bool hit = player_bounds.contains(pos);
		if (hit)
		{
			std::cout << "[DEBUG] COLLISION DETECTED at (" << pos.x() << ", " << pos.y() 
			          << ") within bounds [" << player_bounds.left() << "-" << player_bounds.right() 
			          << ", " << player_bounds.top() << "-" << player_bounds.bottom() << "]" << std::endl;
		}
		return hit;
	}
	
	void MobProjectile::expire()
	{
		expired = true;
	}
}