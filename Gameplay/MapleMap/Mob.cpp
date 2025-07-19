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
#include "Mob.h"

#include "../../Util/Misc.h"

#include "../../Net/Packets/GameplayPackets.h"
#include "../Stage.h"

#include <iostream>
#include <cmath>

#ifdef USE_NX
#include <nlnx/nx.hpp>
#endif

namespace ms
{
	Mob::Mob(int32_t oi, int32_t mid, int8_t mode, int8_t st, uint16_t fh, bool newspawn, int8_t tm, Point<int16_t> position) : MapObject(oi)
	{
		std::string strid = string_format::extend_id(mid, 7);
		
		nl::node src = nl::nx::Mob[strid + ".img"];
		if (!src) {
		}

		nl::node info = src["info"];

		level = info["level"];
		watk = info["PADamage"];
		matk = info["MADamage"];
		wdef = info["PDDamage"];
		mdef = info["MDDamage"];
		accuracy = info["acc"];
		avoid = info["eva"];
		knockback = info["pushed"];
		speed = info["speed"];
		flyspeed = info["flySpeed"];
		touchdamage = info["bodyAttack"].get_bool();
		undead = info["undead"].get_bool();
		noflip = info["noFlip"].get_bool();
		notattack = info["notAttack"].get_bool();
		canjump = src["jump"].size() > 0;
		canfly = src["fly"].size() > 0;
		canmove = src["move"].size() > 0 || canfly;
		
		std::string linkid = info["link"];
		nl::node link_src = nl::nx::Mob[linkid + ".img"];
		nl::node link = link_src ? link_src : src;
		nl::node fly = link["fly"];

		if (canfly)
		{
			animations[Stance::STAND] = fly;
			animations[Stance::MOVE] = fly;
		}
		else
		{
			animations[Stance::STAND] = link["stand"];
			animations[Stance::MOVE] = link["move"];
			
		}

		animations[Stance::JUMP] = link["jump"];
		animations[Stance::HIT] = link["hit1"];
		animations[Stance::DIE] = link["die1"];
		
		// Load attack animations if they exist
		if (link["attack1"])
			animations[Stance::ATTACK1] = link["attack1"];
		if (link["attack2"])
			animations[Stance::ATTACK2] = link["attack2"];
		if (link["attack3"])
			animations[Stance::ATTACK3] = link["attack3"];
		if (link["attack4"])
			animations[Stance::ATTACK4] = link["attack4"];
		if (link["skill1"])
			animations[Stance::SKILL1] = link["skill1"];
		if (link["skill2"])
			animations[Stance::SKILL2] = link["skill2"];

		name = nl::nx::String["Mob.img"][std::to_string(mid)]["name"];

		nl::node sndsrc = nl::nx::Sound["Mob.img"][strid];

		hitsound = sndsrc["Damage"];
		diesound = sndsrc["Die"];

		speed += 100;
		speed *= 0.001f;

		flyspeed += 100;
		flyspeed *= 0.0005f;

		if (canfly)
			phobj.type = PhysicsObject::Type::FLYING;

		id = mid;
		team = tm;
		
		
		set_position(position);
		set_control(mode);
		phobj.fhid = fh;
		phobj.set_flag(PhysicsObject::Flag::TURNATEDGES);
		

		hppercent = 0;
		dying = false;
		dead = false;
		fading = false;
		set_stance(st);
		flydirection = STRAIGHT;
		counter = 0;
		skill_anim_time = 0;
		pre_skill_stance = Stance::STAND;

		namelabel = Text(Text::Font::A13M, Text::Alignment::CENTER, Color::Name::WHITE, Text::Background::NAMETAG, name);

		if (newspawn)
		{
			fadein = true;
			opacity.set(0.0f);
		}
		else
		{
			fadein = false;
			opacity.set(1.0f);
		}

		if (control && stance == Stance::STAND)
			next_move();
	}

	void Mob::set_stance(uint8_t stancebyte)
	{
		// Handle invalid stance values (like -1/0xFF from server)
		if (stancebyte == 0xFF || stancebyte > Stance::DIE + 1)
		{
			// Default to STAND facing right
			stancebyte = Stance::STAND;
		}
		
		flip = (stancebyte % 2) == 0;

		if (!flip)
			stancebyte -= 1;

		if (stancebyte < Stance::MOVE)
			stancebyte = Stance::MOVE;

		set_stance(static_cast<Stance>(stancebyte));
	}

	void Mob::set_stance(Stance newstance)
	{
		if (stance != newstance)
		{
			stance = newstance;

			animations.at(stance).reset();
		}
	}

	int8_t Mob::update(const Physics& physics)
	{
		if (!active)
			return phobj.fhlayer;
			

		bool aniend = animations.at(stance).update();

		if (aniend && stance == Stance::DIE)
			dead = true;

		if (fading)
		{
			opacity -= 0.025f;

			if (opacity.last() < 0.025f)
			{
				opacity.set(0.0f);
				fading = false;
				dead = true;
			}
		}
		else if (fadein)
		{
			opacity += 0.025f;

			if (opacity.last() > 0.975f)
			{
				opacity.set(1.0f);
				fadein = false;
			}
		}

		if (dead)
		{
			deactivate();

			return -1;
		}

		effects.update();
		showhp.update();

		// Update skill animation timer
		if (skill_anim_time > 0)
		{
			skill_anim_time -= Constants::TIMESTEP;
			if (skill_anim_time <= 0)
			{
				// Return to previous stance after skill animation
				skill_anim_time = 0;
				set_stance(pre_skill_stance);
			}
		}

		if (!dying)
		{
			if (!canfly)
			{
				if (phobj.is_flag_not_set(PhysicsObject::Flag::TURNATEDGES))
				{
					flip = !flip;
					phobj.set_flag(PhysicsObject::Flag::TURNATEDGES);

					if (stance == Stance::HIT)
						set_stance(Stance::STAND);
				}
			}

			switch (stance)
			{
			case Stance::MOVE:
				if (canfly)
				{
					phobj.hforce = flip ? flyspeed : -flyspeed;

					switch (flydirection)
					{
					case FlyDirection::UPWARDS:
						phobj.vforce = -flyspeed;
						break;
					case FlyDirection::DOWNWARDS:
						phobj.vforce = flyspeed;
						break;
					}
				}
				else
				{
					phobj.hforce = flip ? speed : -speed;
				}

				break;
			case Stance::HIT:
				if (canmove)
				{
					double KBFORCE = phobj.onground ? 0.2 : 0.1;
					phobj.hforce = flip ? -KBFORCE : KBFORCE;
				}

				break;
			case Stance::JUMP:
				phobj.vforce = -5.0;
				break;
			}

			physics.move_object(phobj);

			if (control)
			{
				counter++;

				bool next;

				switch (stance)
				{
				case Stance::HIT:
					next = counter > 200;
					break;
				case Stance::JUMP:
					next = phobj.onground;
					break;
				default:
					next = aniend && counter > 200;
					break;
				}

				if (next)
				{
					next_move();
					update_movement();
					counter = 0;
				}
			}
		}
		else
		{
			Point<int16_t> pos_before = phobj.get_position();
			phobj.normalize();
			physics.get_fht().update_fh(phobj);
			Point<int16_t> pos_after = phobj.get_position();
			
		}

		return phobj.fhlayer;
	}

	void Mob::next_move()
	{
		std::cout << "[DEBUG] Mob " << oid << " next_move called - control=" << control 
		          << ", aggro=" << aggro << ", stance=" << (int)stance 
		          << ", notattack=" << notattack << ", canmove=" << canmove << std::endl;
		
		if (canmove)
		{
			switch (stance)
			{
			case Stance::HIT:
			case Stance::STAND:
			case Stance::ATTACK1:
			case Stance::ATTACK2:
			case Stance::ATTACK3:
			case Stance::ATTACK4:
				// Check if we should attack (50% chance when aggro)
				if (aggro && notattack == false && randomizer.below(0.5f))
				{
					// Try to use attack1 if available
					if (animations.find(Stance::ATTACK1) != animations.end())
					{
						set_stance(Stance::ATTACK1);
						std::cout << "[DEBUG] Mob " << oid << " deciding to attack!" << std::endl;
						// Spawn projectile for local attacks
						Stage::get().get_mobs().check_attack_projectile(oid, id);
					}
					else
					{
						std::cout << "[DEBUG] Mob " << oid << " has no ATTACK1 animation!" << std::endl;
						set_stance(Stance::MOVE);
						flip = randomizer.next_bool();
					}
				}
				else
				{
					std::cout << "[DEBUG] Mob " << oid << " not attacking (aggro=" << aggro 
				          << ", notattack=" << notattack << ", random failed)" << std::endl;
					set_stance(Stance::MOVE);
					flip = randomizer.next_bool();
				}
				break;
			case Stance::MOVE:
			case Stance::JUMP:
				if (canjump && phobj.onground && randomizer.below(0.25f))
				{
					set_stance(Stance::JUMP);
				}
				else
				{
					switch (randomizer.next_int(3))
					{
					case 0:
						set_stance(Stance::STAND);
						break;
					case 1:
						set_stance(Stance::MOVE);
						flip = false;
						break;
					case 2:
						set_stance(Stance::MOVE);
						flip = true;
						break;
					}
				}

				break;
			}

			if (stance == Stance::MOVE && canfly)
				flydirection = randomizer.next_enum(FlyDirection::NUM_DIRECTIONS);
		}
		else
		{
			// For stationary mobs, check if they should attack
			if (stance == Stance::STAND || stance == Stance::ATTACK1)
			{
				// Check if player is in range for attack (simplified range check)
				auto& player = Stage::get().get_player();
				Point<int16_t> player_pos = player.get_phobj().get_position();
				Point<int16_t> mob_pos = phobj.get_position();
				
				int16_t distance = std::abs(player_pos.x() - mob_pos.x());
				bool in_range = distance < 150; // Attack range for stationary mobs
				
				if (in_range && notattack == false && randomizer.below(0.3f)) // 30% chance
				{
					if (animations.find(Stance::ATTACK1) != animations.end())
					{
						// Turn to face the player before attacking
						bool player_is_left = player_pos.x() < mob_pos.x();
						// Invert the logic - if player is left, mob should NOT flip (face left)
						bool should_flip = !player_is_left; // flip=false means facing left, flip=true means facing right
						
						std::cout << "[DEBUG] Player is " << (player_is_left ? "left" : "right") 
						          << " of mob. Current flip=" << flip 
						          << ", should_flip=" << should_flip << std::endl;
						
						if (flip != should_flip)
						{
							flip = should_flip;
							std::cout << "[DEBUG] Stationary mob turning to face " 
							          << (!flip ? "left" : "right") << std::endl;
						}
						
						set_stance(Stance::ATTACK1);
						std::cout << "[DEBUG] Stationary mob " << oid << " attacking! Distance: " << distance << std::endl;
						// Spawn projectile for local attacks
						Stage::get().get_mobs().check_attack_projectile(oid, id);
					}
					else
					{
						set_stance(Stance::STAND);
					}
				}
				else
				{
					set_stance(Stance::STAND);
				}
			}
			else
			{
				set_stance(Stance::STAND);
			}
		}
	}

	void Mob::update_movement()
	{
		MoveMobPacket(
			oid, 1, 0, 0, 0, 0, 0, 0,
			get_position(),
			Movement(phobj, value_of(stance, flip))
		).dispatch();
	}

	void Mob::draw(double viewx, double viewy, float alpha) const
	{
		static int draw_count = 0;
		
		Point<int16_t> absp = phobj.get_absolute(viewx, viewy, alpha);
		Point<int16_t> headpos = get_head_position(absp);

		effects.drawbelow(absp, alpha);

		if (!dead)
		{
			float interopc = opacity.get(alpha);

			animations.at(stance).draw(DrawArgument(absp, flip && !noflip, interopc), alpha);

			if (showhp)
			{
				namelabel.draw(absp);

				if (!dying && hppercent > 0)
					hpbar.draw(headpos, hppercent);
			}
		}

		effects.drawabove(absp, alpha);
	}

	void Mob::set_control(int8_t mode)
	{
		control = mode > 0;
		aggro = mode == 2;
		// TEMPORARY: For testing, make all controlled mobs aggressive
		if (control && id == 4230106) // Lunar Pixie
		{
			aggro = true;
			std::cout << "[DEBUG] Forcing Lunar Pixie to be aggressive for testing" << std::endl;
		}
		std::cout << "[DEBUG] Mob " << oid << " set_control: mode=" << (int)mode 
		          << ", control=" << control << ", aggro=" << aggro << std::endl;
	}

	void Mob::send_movement(Point<int16_t> start, std::vector<Movement>&& in_movements)
	{
		if (control)
			return;

		set_position(start);

		movements = std::forward<decltype(in_movements)>(in_movements);

		if (movements.empty())
			return;

		const Movement& lastmove = movements.front();

		uint8_t laststance = lastmove.newstate;
		set_stance(laststance);

		phobj.fhid = lastmove.fh;
	}

	Point<int16_t> Mob::get_head_position(Point<int16_t> position) const
	{
		Point<int16_t> head = animations.at(stance).get_head();

		position.shift_x((flip && !noflip) ? -head.x() : head.x());
		position.shift_y(head.y());

		return position;
	}

	void Mob::kill(int8_t animation)
	{
		switch (animation)
		{
		case 0:
			deactivate();
			break;
		case 1:
			dying = true;

			apply_death();
			break;
		case 2:
			fading = true;
			dying = true;
			break;
		}
	}

	void Mob::show_hp(int8_t percent, uint16_t playerlevel)
	{
		if (hppercent == 0)
		{
			int16_t delta = playerlevel - level;

			if (delta > 9)
				namelabel.change_color(Color::Name::YELLOW);
			else if (delta < -9)
				namelabel.change_color(Color::Name::RED);
		}

		if (percent > 100)
			percent = 100;
		else if (percent < 0)
			percent = 0;

		hppercent = percent;
		showhp.set_for(2000);
	}

	void Mob::show_effect(const Animation& animation, int8_t pos, int8_t z, bool f)
	{
		if (!active)
			return;

		Point<int16_t> shift;

		switch (pos)
		{
		case 0:
			shift = get_head_position(Point<int16_t>());
			break;
		case 1:
			break;
		case 2:
			break;
		case 3:
			break;
		case 4:
			break;
		}

		effects.add(animation, DrawArgument(shift, f), z);
	}

	void Mob::show_skill(int16_t skill_id, int16_t skill_level)
	{
		if (!active || dying || skill_anim_time > 0)
			return;

		// Save current stance to return to after skill
		pre_skill_stance = stance;

		// Determine which attack animation to use based on skill_id
		// For now, we'll cycle through available attack animations
		Stance skill_stance = Stance::STAND;
		
		// Try to find an available attack animation
		if (animations.find(Stance::ATTACK1) != animations.end())
			skill_stance = Stance::ATTACK1;
		else if (animations.find(Stance::ATTACK2) != animations.end())
			skill_stance = Stance::ATTACK2;
		else if (animations.find(Stance::SKILL1) != animations.end())
			skill_stance = Stance::SKILL1;
		else if (animations.find(Stance::SKILL2) != animations.end())
			skill_stance = Stance::SKILL2;
		else if (animations.find(Stance::ATTACK3) != animations.end())
			skill_stance = Stance::ATTACK3;
		else if (animations.find(Stance::ATTACK4) != animations.end())
			skill_stance = Stance::ATTACK4;
		else
		{
			// No attack animation available, try using hit animation as fallback
			if (animations.find(Stance::HIT) != animations.end())
				skill_stance = Stance::HIT;
			else
				return; // No suitable animation found
		}

		// Play the attack animation
		set_stance(skill_stance);
		
		// Get the actual animation duration
		// We'll use a reasonable default and let the animation loop if needed
		const Animation& skill_anim = animations.at(skill_stance);
		
		// Get total delay for one complete animation cycle
		// Most mob skill animations are 2-4 seconds
		uint16_t base_duration = 3000; // 3 second default
		
		// Try to get the actual delay from the first frame
		uint16_t frame_delay = skill_anim.get_delay(0);
		if (frame_delay > 0)
		{
			// Estimate total duration based on typical frame counts
			// Skills usually have more frames than regular attacks
			base_duration = frame_delay * 20; // Assume ~20 frames for skills
			
			// Ensure minimum duration for skill animations
			if (base_duration < 2000)
				base_duration = 2000;
		}
		
		// Set skill animation timer
		skill_anim_time = base_duration;
		
		// Skill animation started with estimated duration
		
		
		// Add skill-specific visual effects based on skill_id
		// Load skill effect from WZ files if available
		std::string skill_str = std::to_string(skill_id);
		nl::node skill_node = nl::nx::Skill["MobSkill.img"][skill_str]["level"]["1"]["mob"];
		
		if (skill_node)
		{
			Animation skill_effect(skill_node);
			// Show the effect at the mob's position
			show_effect(skill_effect, 0, 1, false);
			// Skill effect shown
		}
		
		// For projectile skills, we need to handle them separately
		// These would need to be spawned as separate objects that move toward the player
	}

	float Mob::calculate_hitchance(int16_t leveldelta, int32_t player_accuracy) const
	{
		float faccuracy = static_cast<float>(player_accuracy);
		float hitchance = faccuracy / (((1.84f + 0.07f * leveldelta) * avoid) + 1.0f);

		if (hitchance < 0.01f)
			hitchance = 0.01f;

		return hitchance;
	}

	double Mob::calculate_mindamage(int16_t leveldelta, double damage, bool magic) const
	{
		double mindamage =
			magic ?
			damage - (1 + 0.01 * leveldelta) * mdef * 0.6 :
			damage * (1 - 0.01 * leveldelta) - wdef * 0.6;

		return mindamage < 1.0 ? 1.0 : mindamage;
	}

	double Mob::calculate_maxdamage(int16_t leveldelta, double damage, bool magic) const
	{
		double maxdamage =
			magic ?
			damage - (1 + 0.01 * leveldelta) * mdef * 0.5 :
			damage * (1 - 0.01 * leveldelta) - wdef * 0.5;

		return maxdamage < 1.0 ? 1.0 : maxdamage;
	}

	std::vector<std::pair<int32_t, bool>> Mob::calculate_damage(const Attack& attack)
	{
		double mindamage;
		double maxdamage;
		float hitchance;
		float critical;
		int16_t leveldelta = level - attack.playerlevel;

		if (leveldelta < 0)
			leveldelta = 0;

		Attack::DamageType damagetype = attack.damagetype;

		switch (damagetype)
		{
		case Attack::DamageType::DMG_WEAPON:
		case Attack::DamageType::DMG_MAGIC:
			mindamage = calculate_mindamage(leveldelta, attack.mindamage, damagetype == Attack::DamageType::DMG_MAGIC);
			maxdamage = calculate_maxdamage(leveldelta, attack.maxdamage, damagetype == Attack::DamageType::DMG_MAGIC);
			hitchance = calculate_hitchance(leveldelta, attack.accuracy);
			critical = attack.critical;
			break;
		case Attack::DamageType::DMG_FIXED:
			mindamage = attack.fixdamage;
			maxdamage = attack.fixdamage;
			hitchance = 1.0f;
			critical = 0.0f;
			break;
		}

		std::vector<std::pair<int32_t, bool>> result(attack.hitcount);

		std::generate(
			result.begin(), result.end(),
			[&]()
			{
				return next_damage(mindamage, maxdamage, hitchance, critical);
			}
		);

		update_movement();

		return result;
	}

	std::pair<int32_t, bool> Mob::next_damage(double mindamage, double maxdamage, float hitchance, float critical) const
	{
		bool hit = randomizer.below(hitchance);

		if (!hit)
			return std::pair<int32_t, bool>(0, false);

		constexpr double DAMAGECAP = 999999.0;

		double damage = randomizer.next_real(mindamage, maxdamage);
		bool iscritical = randomizer.below(critical);

		if (iscritical)
			damage *= 1.5;

		if (damage < 1)
			damage = 1;
		else if (damage > DAMAGECAP)
			damage = DAMAGECAP;

		auto intdamage = static_cast<int32_t>(damage);

		return std::pair<int32_t, bool>(intdamage, iscritical);
	}

	void Mob::apply_damage(int32_t damage, bool toleft)
	{
		hitsound.play();

		if (dying && stance != Stance::DIE)
		{
			apply_death();
		}
		else if (control && is_alive() && damage >= knockback)
		{
			flip = toleft;
			counter = 170;
			set_stance(Stance::HIT);

			update_movement();
		}
	}

	MobAttack Mob::create_touch_attack() const
	{
		if (!touchdamage)
			return MobAttack();

		int32_t minattack = static_cast<int32_t>(watk * 0.8f);
		int32_t maxattack = watk;
		int32_t attack = randomizer.next_int(minattack, maxattack);

		return MobAttack(attack, get_position(), id, oid);
	}

	void Mob::apply_death()
	{
		set_stance(Stance::DIE);
		diesound.play();
		dying = true;
	}

	bool Mob::is_alive() const
	{
		return active && !dying;
	}

	bool Mob::is_in_range(const Rectangle<int16_t>& range) const
	{
		if (!active)
			return false;

		Rectangle<int16_t> bounds = animations.at(stance).get_bounds();
		bounds.shift(get_position());

		return range.overlaps(bounds);
	}

	Point<int16_t> Mob::get_head_position() const
	{
		Point<int16_t> position = get_position();

		return get_head_position(position);
	}
}