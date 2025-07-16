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

#include "../MapleStory.h"

#ifdef USE_NX
// NoLifeNx is now compiled from source, no need for pragma comment

#include "../Error.h"

#include <array>

namespace ms
{
	namespace NxFiles
	{
		// Number of needed files (v83 consolidated files only)
		constexpr uint8_t NUM_FILES = 18;

		// Names of the needed game files (v83 consolidated versions)
		constexpr std::array<const char*, NUM_FILES> filenames =
		{
			"Base.nx",
			"Character.nx",
			"Effect.nx",
			"Etc.nx",
			"Item.nx",
			"Map.nx",
			//"Map001.nx",    // v83: consolidated in Map.nx
			//"Map002.nx",    // v83: consolidated in Map.nx  
			"Map2.nx",
			"Mob.nx",
			//"Mob001.nx",    // v83: consolidated in Mob.nx
			//"Mob002.nx",    // v83: consolidated in Mob.nx
			//"Mob2.nx",      // v83: consolidated in Mob.nx
			"Morph.nx",
			"Npc.nx",
			"Quest.nx",
			"Reactor.nx",
			"Skill.nx",
			//"Skill001.nx",  // v83: consolidated in Skill.nx
			//"Skill002.nx",  // v83: consolidated in Skill.nx
			//"Skill003.nx",  // v83: consolidated in Skill.nx
			"Sound.nx",
			//"Sound001.nx",  // v83: consolidated in Sound.nx
			//"Sound002.nx",  // v83: consolidated in Sound.nx or Sound2.nx
			"Sound2.nx",
			"String.nx",
			"TamingMob.nx",
			"UI.nx"
			//"UI_000.nx"     // v83: not present
		};

		// Initialize NX
		// When successful test if the UI file is the correct version
		Error init();
	};
}
#endif