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
#include <nlnx/node.hpp>

namespace ms
{
	namespace UINodes
	{
		// Unified access to UI nodes from split archives
		// This function checks UI.nx first, then UI_000.nx as fallback
		// This handles modern split UI archives transparently
		nl::node get_ui_node(const std::string& path);
		
		// Helper for common UI access patterns
		nl::node get_ui_img(const std::string& img_name);
		
		// Resolve UI nodes with fallback to UI_000
		template<typename... Args>
		nl::node resolve_ui(Args&&... args);
	}
}
#endif