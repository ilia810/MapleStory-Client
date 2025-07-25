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
#include "Physics.h"
#include <iostream>
#include <unordered_map>

namespace ms
{
	const double GRAVFORCE = 0.14;
	const double SWIMGRAVFORCE = 0.03;
	const double FRICTION = 0.5;
	const double SLOPEFACTOR = 0.1;
	const double GROUNDSLIP = 3.0;
	const double FLYFRICTION = 0.05;
	const double SWIMFRICTION = 0.08;

	Physics::Physics(nl::node src)
	{
		fht = src;
	}

	Physics::Physics() {}

	void Physics::move_object(PhysicsObject& phobj) const
	{
		// Store previous position for oscillation detection
		static std::unordered_map<void*, double> lastYPositions;
		static std::unordered_map<void*, int> oscillationCounters;
		
		double prevY = lastYPositions[&phobj];
		double currentY = phobj.crnt_y();
		
		// Determine which platform the object is currently on
		fht.update_fh(phobj);

		// Use the appropriate physics for the terrain the object is on
		switch (phobj.type)
		{
		case PhysicsObject::Type::NORMAL:
			move_normal(phobj);
			fht.limit_movement(phobj);
			break;
		case PhysicsObject::Type::FLYING:
			move_flying(phobj);
			fht.limit_movement(phobj);
			break;
		case PhysicsObject::Type::SWIMMING:
			move_swimming(phobj);
			fht.limit_movement(phobj);
			break;
		case PhysicsObject::Type::FIXATED:
		default:
			break;
		}

		// Clamp extreme positions as safety net
		const double MAX_SAFE_Y = 5000.0;
		const double MIN_SAFE_Y = -5000.0;
		
		double next_y = phobj.next_y();
		if (next_y > MAX_SAFE_Y) {
			phobj.limity(MAX_SAFE_Y);
			phobj.vspeed = 0.0;  // Stop vertical movement
		} else if (next_y < MIN_SAFE_Y) {
			phobj.limity(MIN_SAFE_Y);
			phobj.vspeed = 0.0;  // Stop vertical movement
		}

		// Move the object forward
		phobj.move();
		
		// Oscillation detection after movement
		double newY = phobj.crnt_y();
		double yDelta = std::abs(newY - prevY);
		
		// Detect large position changes (potential oscillation)
		if (yDelta > 1000.0) {
			oscillationCounters[&phobj]++;
			
			if (oscillationCounters[&phobj] >= 2) {
				// Emergency stabilization
				double midY = (prevY + newY) / 2.0;
				if (std::abs(midY) > 2000.0) {
					midY = 0.0;  // Reset to origin if midpoint is also extreme
				}
				
				phobj.limity(midY);
				phobj.vspeed = 0.0;
				phobj.hspeed = 0.0;
				phobj.onground = true;  // Force ground state to stop gravity
				
				// Reset counter after fixing
				oscillationCounters[&phobj] = 0;
			}
		} else {
			// Reset counter if movement is normal
			oscillationCounters[&phobj] = 0;
		}
		
		// Update position history
		lastYPositions[&phobj] = newY;
	}

	void Physics::move_normal(PhysicsObject& phobj) const
	{
		phobj.vacc = 0.0;
		phobj.hacc = 0.0;

		if (phobj.onground)
		{
			phobj.vacc += phobj.vforce;
			phobj.hacc += phobj.hforce;

			if (phobj.hacc == 0.0 && phobj.hspeed < 0.1 && phobj.hspeed > -0.1)
			{
				phobj.hspeed = 0.0;
			}
			else
			{
				double inertia = phobj.hspeed / GROUNDSLIP;
				double slopef = phobj.fhslope;

				if (slopef > 0.5)
					slopef = 0.5;
				else if (slopef < -0.5)
					slopef = -0.5;

				phobj.hacc -= (FRICTION + SLOPEFACTOR * (1.0 + slopef * -inertia)) * inertia;
			}
		}
		else if (phobj.is_flag_not_set(PhysicsObject::Flag::NOGRAVITY))
		{
			phobj.vacc += GRAVFORCE;
		}

		phobj.hforce = 0.0;
		phobj.vforce = 0.0;

		phobj.hspeed += phobj.hacc;
		phobj.vspeed += phobj.vacc;
	}

	void Physics::move_flying(PhysicsObject& phobj) const
	{
		phobj.hacc = phobj.hforce;
		phobj.vacc = phobj.vforce;
		phobj.hforce = 0.0;
		phobj.vforce = 0.0;

		phobj.hacc -= FLYFRICTION * phobj.hspeed;
		phobj.vacc -= FLYFRICTION * phobj.vspeed;

		phobj.hspeed += phobj.hacc;
		phobj.vspeed += phobj.vacc;

		if (phobj.hacc == 0.0 && phobj.hspeed < 0.1 && phobj.hspeed > -0.1)
			phobj.hspeed = 0.0;

		if (phobj.vacc == 0.0 && phobj.vspeed < 0.1 && phobj.vspeed > -0.1)
			phobj.vspeed = 0.0;
	}

	void Physics::move_swimming(PhysicsObject& phobj) const
	{
		phobj.hacc = phobj.hforce;
		phobj.vacc = phobj.vforce;
		phobj.hforce = 0.0;
		phobj.vforce = 0.0;

		phobj.hacc -= SWIMFRICTION * phobj.hspeed;
		phobj.vacc -= SWIMFRICTION * phobj.vspeed;

		if (phobj.is_flag_not_set(PhysicsObject::Flag::NOGRAVITY))
			phobj.vacc += SWIMGRAVFORCE;

		phobj.hspeed += phobj.hacc;
		phobj.vspeed += phobj.vacc;

		if (phobj.hacc == 0.0 && phobj.hspeed < 0.1 && phobj.hspeed > -0.1)
			phobj.hspeed = 0.0;

		if (phobj.vacc == 0.0 && phobj.vspeed < 0.1 && phobj.vspeed > -0.1)
			phobj.vspeed = 0.0f;
	}

	Point<int16_t> Physics::get_y_below(Point<int16_t> position) const
	{
		int16_t ground = fht.get_y_below(position);
		
		
		// Validate ground result - if extreme, use fallback
		const int16_t MIN_VALID_Y = -2000;  // Reasonable minimum Y position
		const int16_t MAX_VALID_Y = 3000;   // Reasonable maximum Y position
		
		if (ground < MIN_VALID_Y || ground > MAX_VALID_Y) {
			ground = position.y();  // Use input Y as fallback
		}

		return Point<int16_t>(position.x(), ground - 1);
	}

	const FootholdTree& Physics::get_fht() const
	{
		return fht;
	}
}