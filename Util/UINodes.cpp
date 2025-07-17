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
#include "UINodes.h"

#ifdef USE_NX
#include <nlnx/nx.hpp>

namespace ms
{
	namespace UINodes
	{
		nl::node get_ui_node(const std::string& path)
		{
			// Try UI.nx first (base file)
			nl::node result = nl::nx::UI.resolve(path);
			if (result.data_type() != nl::node::type::none)
				return result;
			
			// Fallback to UI_000.nx (chunk file with actual data)
			result = nl::nx::UI_000.resolve(path);
			return result;
		}
		
		nl::node get_ui_img(const std::string& img_name)
		{
			std::string full_path = img_name;
			if (full_path.find(".img") == std::string::npos)
				full_path += ".img";
			
			return get_ui_node(full_path);
		}
		
		// Helper function for operator[] style access with fallback
		nl::node get_ui_child(const std::string& parent_path, const std::string& child_name)
		{
			// Try UI.nx first
			nl::node parent = nl::nx::UI.resolve(parent_path);
			if (parent.data_type() != nl::node::type::none)
			{
				nl::node child = parent[child_name];
				if (child.data_type() != nl::node::type::none)
					return child;
			}
			
			// Fallback to UI_000.nx
			parent = nl::nx::UI_000.resolve(parent_path);
			if (parent.data_type() != nl::node::type::none)
				return parent[child_name];
			
			return nl::node();
		}
	}
}
#endif