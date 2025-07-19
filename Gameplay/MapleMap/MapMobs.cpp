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
#include "MapMobs.h"
#include "Mob.h"

#include "../../Util/Misc.h"
#include "../Stage.h"
#include "../../Template/Range.h"
#include "../../Template/Rectangle.h"

#include <algorithm>
#include <iostream>
#include <map>

#ifdef USE_NX
#include <nlnx/nx.hpp>
#endif

namespace ms
{
	void MapMobs::draw(Layer::Id layer, double viewx, double viewy, float alpha) const
	{
		static int draw_frame = 0;
		mobs.draw(layer, viewx, viewy, alpha);
		
		// Draw projectiles on layer 6 (effects layer)
		if (layer == Layer::SIX)
		{
			draw_projectiles(layer, viewx, viewy, alpha);
		}
	}

	void MapMobs::update(const Physics& physics)
	{
		// Update projectiles
		update_projectiles(physics);
		
		// Check projectile collisions with player
		auto& player = Stage::get().get_player();
		const PhysicsObject& phobj = player.get_phobj();
		
		// Get player position for collision
		Point<int16_t> player_pos = phobj.get_position();
		
		// Create player bounds as a box around the player position
		Rectangle<int16_t> player_bounds = {
			player_pos.x() - 30,    // Left
			player_pos.x() + 30,    // Right
			player_pos.y() - 50,    // Top (above player)
			player_pos.y()          // Bottom (at player feet)
		};
		
		// Debug player position occasionally
		static int player_debug_count = 0;
		if (player_debug_count++ % 300 == 0)
		{
			std::cout << "[DEBUG] Player position: (" << player_pos.x() << ", " << player_pos.y() 
			          << ") bounds: [" << player_bounds.left() << "-" << player_bounds.right() 
			          << ", " << player_bounds.top() << "-" << player_bounds.bottom() << "]" << std::endl;
		}
		
		check_projectile_collisions(player_bounds);
		
		for (; !spawns.empty(); spawns.pop())
		{
			const MobSpawn& spawn = spawns.front();
			

			if (Optional<Mob> mob = mobs.get(spawn.get_oid()))
			{
				int8_t mode = spawn.get_mode();
				

				if (mode > 0)
					mob->set_control(mode);

				mob->makeactive();
			}
			else
			{
				mobs.add(spawn.instantiate(physics));
			}
		}

		mobs.update(physics);
	}

	void MapMobs::spawn(MobSpawn&& spawn)
	{
		spawns.emplace(std::move(spawn));
	}

	void MapMobs::remove(int32_t oid, int8_t animation)
	{
		if (Optional<Mob> mob = mobs.get(oid))
			mob->kill(animation);
	}

	void MapMobs::clear()
	{
		mobs.clear();
	}

	void MapMobs::set_control(int32_t oid, bool control)
	{
		int8_t mode = control ? 1 : 0;

		if (Optional<Mob> mob = mobs.get(oid))
			mob->set_control(mode);
	}

	void MapMobs::send_mobhp(int32_t oid, int8_t percent, uint16_t playerlevel)
	{
		if (Optional<Mob> mob = mobs.get(oid))
			mob->show_hp(percent, playerlevel);
	}

	void MapMobs::send_movement(int32_t oid, Point<int16_t> start, std::vector<Movement>&& movements)
	{
		if (Optional<Mob> mob = mobs.get(oid))
		{
			mob->send_movement(start, std::move(movements));
			
			// Check if this is an attack movement that might have projectiles
			for (const auto& movement : movements)
			{
				// Check if this is an attack stance
				uint8_t stance = movement.newstate;
				// Attack stances are typically 12, 14, 16, 18 (ATTACK1-4)
				if (stance >= 12 && stance <= 18 && (stance % 2 == 0))
				{
					std::cout << "[DEBUG] Mob " << oid << " entered attack stance " << (int)stance << std::endl;
					// Check if this mob has projectile data for attacks
					check_attack_projectile(oid, mob->get_id());
				}
			}
		}
	}

	void MapMobs::send_skill(int32_t oid, int16_t skill_id, int16_t skill_level)
	{
		if (Optional<Mob> mob = mobs.get(oid))
		{
			mob->show_skill(skill_id, skill_level);
			
			std::cout << "[DEBUG] Mob " << oid << " (ID: " << mob->get_id() 
			          << ") using skill " << skill_id << " level " << skill_level << std::endl;
			
			// Check if this skill should spawn a projectile
			// Skills that typically have projectiles include various magic attacks
			// For now, let's check specific skill IDs known to have projectiles
			bool should_spawn_projectile = false;
			
			// Common projectile skills:
			// Pixie skills, magic attacks, etc.
			// Let's be more inclusive with skill ranges
			// Most projectile skills are in these ranges:
			// Skill ID 1 is typically a buff, not a projectile attack
			if ((skill_id >= 100 && skill_id <= 150) ||  // Basic magic/projectile attacks
			    (skill_id >= 170 && skill_id <= 190) ||  // Special projectiles
			    (skill_id >= 200 && skill_id <= 230) ||  // Boss/advanced projectiles
			    (skill_id >= 140 && skill_id <= 141) ||  // Specific known projectile skills
			    (skill_id >= 126 && skill_id <= 129))    // More magic attacks
			{
				should_spawn_projectile = true;
			}
			
			if (should_spawn_projectile)
			{
				std::cout << "[DEBUG] Spawning projectile for skill " << skill_id << std::endl;
				
				// Get player position as target
				Point<int16_t> mob_pos = mob->get_position();
				Point<int16_t> target_pos = Stage::get().get_player().get_position();
				// Adjust target position slightly above player for better visual
				target_pos.shift_y(-50);
				
				spawn_projectile(oid, skill_id, target_pos);
			}
			else
			{
				std::cout << "[DEBUG] Skill " << skill_id << " is not in projectile range" << std::endl;
			}
		}
	}

	void MapMobs::send_attack(AttackResult& result, const Attack& attack, const std::vector<int32_t>& targets, uint8_t mobcount)
	{
		for (auto& target : targets)
		{
			if (Optional<Mob> mob = mobs.get(target))
			{
				result.damagelines[target] = mob->calculate_damage(attack);
				result.mobcount++;

				if (result.mobcount == 1)
					result.first_oid = target;

				if (result.mobcount == mobcount)
					result.last_oid = target;
			}
		}
	}

	void MapMobs::apply_damage(int32_t oid, int32_t damage, bool toleft, const AttackUser& user, const SpecialMove& move)
	{
		if (Optional<Mob> mob = mobs.get(oid))
		{
			mob->apply_damage(damage, toleft);

			// TODO: Maybe move this into the method above too?
			move.apply_hiteffects(user, *mob);
		}
	}

	bool MapMobs::contains(int32_t oid) const
	{
		return mobs.contains(oid);
	}

	int32_t MapMobs::find_colliding(const MovingObject& moveobj) const
	{
		Range<int16_t> horizontal = Range<int16_t>(moveobj.get_last_x(), moveobj.get_x());
		Range<int16_t> vertical = Range<int16_t>(moveobj.get_last_y(), moveobj.get_y());

		Rectangle<int16_t> player_rect = {
			horizontal.smaller(),
			horizontal.greater(),
			vertical.smaller() - 50,
			vertical.greater()
		};

		auto iter = std::find_if(
			mobs.begin(),
			mobs.end(),
			[&player_rect](auto& mmo)
			{
				Optional<Mob> mob = mmo.second.get();
				return mob && mob->is_alive() && mob->is_in_range(player_rect);
			}
		);

		if (iter == mobs.end())
			return 0;

		return iter->second->get_oid();
	}

	MobAttack MapMobs::create_attack(int32_t oid) const
	{
		if (Optional<const Mob> mob = mobs.get(oid))
			return mob->create_touch_attack();
		else
			return {};
	}

	Point<int16_t> MapMobs::get_mob_position(int32_t oid) const
	{
		if (auto mob = mobs.get(oid))
			return mob->get_position();
		else
			return Point<int16_t>(0, 0);
	}

	Point<int16_t> MapMobs::get_mob_head_position(int32_t oid) const
	{
		if (Optional<const Mob> mob = mobs.get(oid))
			return mob->get_head_position();
		else
			return Point<int16_t>(0, 0);
	}

	MapObjects* MapMobs::get_mobs()
	{
		return &mobs;
	}
	
	void MapMobs::spawn_projectile(int32_t mob_oid, int16_t skill_id, Point<int16_t> target)
	{
		if (Optional<const Mob> mob = mobs.get(mob_oid))
		{
			Point<int16_t> origin = mob->get_position();
			
			// Move origin up to mob's mid-height
			origin.shift_y(-30); // Adjust to middle of mob sprite
			
			// Adjust origin based on mob's facing direction
			bool mob_facing_left = target.x() < origin.x();
			if (mob_facing_left)
			{
				origin.shift_x(-20); // Spawn from front when facing left
			}
			else
			{
				origin.shift_x(20);  // Spawn from front when facing right
			}
			
			// Load the projectile animation from WZ files
			std::string skill_str = std::to_string(skill_id);
			nl::node skill_node = nl::nx::Skill["MobSkill.img"][skill_str]["level"]["1"]["ball"];
			
			// Some skills use "ball", others use "effect"
			if (!skill_node)
				skill_node = nl::nx::Skill["MobSkill.img"][skill_str]["level"]["1"]["effect"];
				
			// For Pixie, let's check specific mob projectiles
			if (!skill_node)
			{
				// Pixie star projectile
				int32_t mob_id = mob->get_id();
				if (mob_id == 5120000 || mob_id == 4230106 || mob_id == 3230200)
				{
					// Try to load pixie star from mob data
					std::string mob_id_str = string_format::extend_id(mob_id, 7);
					skill_node = nl::nx::Mob[mob_id_str + ".img"]["attack1"]["info"]["ball"];
				}
			}
			
			if (skill_node)
			{
				Animation projectile_anim(skill_node);
				
				// Get projectile speed from skill data or use default
				int16_t speed = 140; // Default speed
				nl::node speed_node = nl::nx::Skill["MobSkill.img"][skill_str]["level"]["1"]["info"]["bulletSpeed"];
				if (speed_node)
					speed = speed_node;
				
				projectiles.push_back(std::make_unique<MobProjectile>(
					projectile_anim, origin, target, speed, mob_oid
				));
				
				std::cout << "[DEBUG] Projectile spawned! Total projectiles: " << projectiles.size() << std::endl;
			}
			else
			{
				std::cout << "[DEBUG] No projectile animation found for skill " << skill_id 
				          << " (tried ball, effect, and mob-specific)" << std::endl;
			}
		}
	}
	
	void MapMobs::update_projectiles(const Physics& physics)
	{
		// Update delayed projectiles
		auto delayed_it = delayed_projectiles.begin();
		while (delayed_it != delayed_projectiles.end())
		{
			delayed_it->delay_remaining -= Constants::TIMESTEP;
			
			if (delayed_it->delay_remaining <= 0)
			{
				// Spawn the projectile
				projectiles.push_back(std::make_unique<MobProjectile>(
					delayed_it->animation, 
					delayed_it->origin, 
					delayed_it->target, 
					delayed_it->speed, 
					delayed_it->mob_oid
				));
				
				std::cout << "[DEBUG] Delayed projectile spawned after delay!" << std::endl;
				
				// Remove from delayed list
				delayed_it = delayed_projectiles.erase(delayed_it);
			}
			else
			{
				++delayed_it;
			}
		}
		
		// Remove expired projectiles
		projectiles.erase(
			std::remove_if(projectiles.begin(), projectiles.end(),
				[&physics](const std::unique_ptr<MobProjectile>& proj) {
					return proj->update(physics);
				}
			),
			projectiles.end()
		);
	}
	
	void MapMobs::draw_projectiles(Layer::Id layer, double viewx, double viewy, float alpha) const
	{
		for (const auto& projectile : projectiles)
		{
			projectile->draw(viewx, viewy, alpha);
		}
	}
	
	void MapMobs::check_projectile_collisions(Rectangle<int16_t> player_bounds)
	{
		for (auto& projectile : projectiles)
		{
			if (projectile->check_collision(player_bounds))
			{
				std::cout << "[DEBUG] Projectile hit player!" << std::endl;
				
				// Create a mob attack for the projectile
				int32_t mob_oid = projectile->get_mob_oid();
				if (Optional<const Mob> mob = mobs.get(mob_oid))
				{
					// Create a projectile attack
					MobAttack attack;
					attack.watk = 150; // Base projectile damage
					attack.mobid = mob->get_id();
					attack.oid = mob_oid;
					attack.valid = true;
					
					// Apply damage to player
					Stage::get().get_player().damage(attack);
					
					// Show impact effect on player (spit dissipating)
					// For spit attacks, we should show the hit animation
					int32_t mob_id = mob->get_id();
					std::string mob_id_str = string_format::extend_id(mob_id, 7);
					nl::node attack_info = nl::nx::Mob[mob_id_str + ".img"]["attack1"]["info"];
					nl::node hit_node = attack_info["hit"];
					
					if (hit_node)
					{
						// Create hit effect animation at player position
						Point<int16_t> impact_pos = Stage::get().get_player().get_position();
						impact_pos.shift_y(-30); // Show effect at player's mid-body
						
						std::cout << "[DEBUG] Showing spit impact effect at player position" << std::endl;
						
						// TODO: Add effect rendering system for impact animations
						// For now, the damage is applied but visual effect needs to be implemented
					}
				}
				
				// Mark projectile as expired
				projectile->expire();
			}
		}
	}
	
	void MapMobs::check_attack_projectile(int32_t oid, int32_t mob_id)
	{
		if (Optional<const Mob> mob = mobs.get(oid))
		{
			// Get mob's position for projectile origin
			Point<int16_t> origin = mob->get_position();
			
			// Move origin up to mob's mid-height (mobs are typically 40-60 pixels tall)
			origin.shift_y(-30); // Adjust to middle of mob sprite
			
			// Get player position as target  
			Point<int16_t> target = Stage::get().get_player().get_position();
			
			// Adjust origin based on mob's facing direction
			bool mob_facing_left = target.x() < origin.x();
			if (mob_facing_left)
			{
				origin.shift_x(-20); // Spawn from front when facing left
			}
			else
			{
				origin.shift_x(20);  // Spawn from front when facing right
			}
			
			// Adjust target to hit player center
			target.shift_y(-25); // Aim at player mid-body
			
			// Try to load attack data from mob
			std::string mob_id_str = string_format::extend_id(mob_id, 7);
			nl::node attack_info = nl::nx::Mob[mob_id_str + ".img"]["attack1"]["info"];
			
			// Debug: Print all children of attack_info
			std::cout << "[DEBUG] Attack info for mob " << mob_id << ":" << std::endl;
			for (auto child : attack_info)
			{
				std::cout << "[DEBUG] attack_info child: " << child.name();
				// Print values for timing-related fields
				if (child.name() == "attackAfter" || child.name() == "effectAfter")
				{
					int delay = child.get_integer();
					std::cout << " = " << delay << "ms";
				}
				std::cout << std::endl;
			}
			
			// Check for projectile attack (ball node)
			nl::node ball_node = attack_info["ball"];
			if (ball_node)
			{
				std::cout << "[DEBUG] Found attack projectile for mob " << mob_id << std::endl;
				std::cout << "[DEBUG] Mob raw position: (" << mob->get_position().x() << ", " << mob->get_position().y() << ")" << std::endl;
				std::cout << "[DEBUG] Projectile origin: (" << origin.x() << ", " << origin.y() << ")" << std::endl;
				std::cout << "[DEBUG] Target position: (" << target.x() << ", " << target.y() << ")" << std::endl;
				
				// The ball node has frames 0, 1, 2 as children
				Animation projectile_anim(ball_node);
				int16_t speed = 100; // Double speed for faster projectiles
				
				// Debug: Check if animation has frames
				std::cout << "[DEBUG] Projectile animation created with ball node" << std::endl;
				
				// Check the projectile sprite dimensions
				if (ball_node["0"])
				{
					nl::node frame0 = ball_node["0"];
					nl::node origin = frame0["origin"];
					if (origin)
					{
						Point<int16_t> sprite_origin = origin;
						std::cout << "[DEBUG] Projectile sprite origin: (" << sprite_origin.x() 
						          << ", " << sprite_origin.y() << ")" << std::endl;
					}
				}
				
				projectiles.push_back(std::make_unique<MobProjectile>(
					projectile_anim, origin, target, speed, oid
				));
				
				std::cout << "[DEBUG] Attack projectile spawned! Total projectiles: " << projectiles.size() << std::endl;
			}
			// Check for spit/ranged attack with hit animation (hit node)
			else if (nl::node hit_node = attack_info["hit"])
			{
				std::cout << "[DEBUG] Found hit-based ranged attack (spit) for mob " << mob_id << std::endl;
				
				// Check for attack timing delays
				int16_t effect_delay = 0;
				if (attack_info["effectAfter"])
				{
					effect_delay = attack_info["effectAfter"].get_integer();
					std::cout << "[DEBUG] Found effectAfter delay: " << effect_delay << "ms" << std::endl;
				}
				else if (attack_info["attackAfter"])
				{
					effect_delay = attack_info["attackAfter"].get_integer();
					std::cout << "[DEBUG] Found attackAfter delay: " << effect_delay << "ms" << std::endl;
				}
				
				// For spit attacks, check if there's an effect node for the projectile
				nl::node effect_node = attack_info["effect"];
				Animation spit_projectile;
				bool has_projectile = false;
				
				if (effect_node)
				{
					std::cout << "[DEBUG] Found effect node for spit projectile" << std::endl;
					spit_projectile = Animation(effect_node);
					has_projectile = true;
				}
				else
				{
					// Use the first few frames of hit animation as projectile
					std::cout << "[DEBUG] Using hit frames for spit projectile" << std::endl;
					// Create a simple projectile from hit frames
					// We'll just use a basic animation for now
					has_projectile = true;
				}
				
				// Get attack range - spit attacks typically have good range
				int16_t range = 400; // Default range for spit attacks
				
				if (attack_info["range"])
				{
					int range_value = attack_info["range"].get_integer();
					if (range_value > 0)
						range = range_value;
				}
				
				// Check if player is in attack range
				Point<int16_t> mob_pos = mob->get_position();
				Point<int16_t> player_pos = Stage::get().get_player().get_position();
				
				int16_t distance = std::abs(player_pos.x() - mob_pos.x());
				int16_t y_diff = std::abs(player_pos.y() - mob_pos.y());
				
				std::cout << "[DEBUG] Spit attack - Distance: " << distance << ", Range: " << range 
				          << ", Y-diff: " << y_diff << std::endl;
				
				// Check if player is within spit range
				if (distance <= range && y_diff < 100) // Spit can reach higher/lower
				{
					std::cout << "[DEBUG] Player in spit range! Creating spit projectile." << std::endl;
					
					if (has_projectile)
					{
						// Create projectile with slower speed for spit
						int16_t spit_speed = 80; // Slower than normal projectiles
						
						// If we don't have an effect animation, create a simple one
						if (!effect_node)
						{
							// For now, let's try to create a projectile from the attack animation itself
							// Look for the main attack animation
							nl::node attack_anim = nl::nx::Mob[mob_id_str + ".img"]["attack1"];
							if (attack_anim)
							{
								spit_projectile = Animation(attack_anim);
								std::cout << "[DEBUG] Using attack1 animation as projectile" << std::endl;
							}
							else
							{
								// Fall back to using hit animation
								spit_projectile = Animation(hit_node);
								std::cout << "[DEBUG] Using hit animation as projectile fallback" << std::endl;
							}
						}
						
						if (effect_delay > 0)
						{
							// Add to delayed projectiles
							DelayedProjectile delayed;
							delayed.animation = spit_projectile;
							delayed.origin = origin;
							delayed.target = target;
							delayed.speed = spit_speed;
							delayed.mob_oid = oid;
							delayed.delay_remaining = effect_delay;
							
							delayed_projectiles.push_back(delayed);
							std::cout << "[DEBUG] Spit projectile delayed by " << effect_delay << "ms" << std::endl;
						}
						else
						{
							// Spawn immediately
							projectiles.push_back(std::make_unique<MobProjectile>(
								spit_projectile, origin, target, spit_speed, oid
							));
							
							std::cout << "[DEBUG] Spit projectile created immediately! Total projectiles: " << projectiles.size() << std::endl;
						}
					}
				}
				else
				{
					std::cout << "[DEBUG] Player out of spit range." << std::endl;
				}
			}
			else
			{
				std::cout << "[DEBUG] No attack data found for mob " << mob_id << " (no ball or hit node)" << std::endl;
			}
		}
	}
}