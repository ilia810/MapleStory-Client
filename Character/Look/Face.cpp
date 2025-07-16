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
#include "Face.h"

#include <iostream>

#ifdef USE_NX
#include <nlnx/nx.hpp>
#endif

namespace ms
{
	Expression::Id Expression::byaction(size_t action)
	{
		action -= 98;

		if (action < Expression::Id::LENGTH)
			return static_cast<Id>(action);

		LOG(LOG_DEBUG, "Unknown Expression::Id action: [" << action << "]");

		return Expression::Id::DEFAULT;
	}

	const EnumMap<Expression::Id, std::string> Expression::names =
	{
		"default",
		"blink",
		"hit",
		"smile",
		"troubled",
		"cry",
		"angry",
		"bewildered",
		"stunned",
		"blaze",
		"bowing",
		"cheers",
		"chu",
		"dam",
		"despair",
		"glitter",
		"hot",
		"hum",
		"love",
		"oops",
		"pain",
		"shine",
		"vomit",
		"wink"
	};

	Face::Face(int32_t faceid)
	{
		try {
			std::string strid = "000" + std::to_string(faceid);
			std::string facepath = strid + ".img";
			
			nl::node facenode = nl::nx::Character["Face"][facepath];
			
			if (!facenode) {
				// Try fallback to default face (20000)
				if (faceid != 20000) {
					facenode = nl::nx::Character["Face"]["00020000.img"];
				}
				
				if (!facenode) {
					name = "Missing Face";
					return;
				}
			}

			for (auto iter : Expression::names)
			{
				Expression::Id exp = iter.first;

				if (exp == Expression::Id::DEFAULT)
				{
					expressions[Expression::Id::DEFAULT].emplace(0, facenode["default"]);
				}
				else
				{
					const std::string& expname = iter.second;
					nl::node expnode = facenode[expname];

					for (uint8_t frame = 0; nl::node framenode = expnode[frame]; ++frame)
						expressions[exp].emplace(frame, framenode);
				}
			}

			try {
				name = nl::nx::String["Eqp.img"]["Eqp"]["Face"][std::to_string(faceid)]["name"];
			} catch (const std::exception& e) {
				name = "Face " + std::to_string(faceid); // Use fallback name
			}
			
		} catch (const std::exception& e) {
			// Don't re-throw, create empty face instead
			name = "Error Face";
			return;
		}
	}

	void Face::draw(Expression::Id expression, uint8_t frame, const DrawArgument& args) const
	{
		try {
			// Add bounds checking to prevent crashes with invalid expressions
			if (expression < 0 || expression >= Expression::Id::LENGTH) {
				return; // Skip drawing if expression ID is invalid
			}
			
			auto frameit = expressions[expression].find(frame);
			if (frameit != expressions[expression].end())
				frameit->second.texture.draw(args);
		}
		catch (...) {
			// Skip drawing if face data is corrupted
			return;
		}
	}

	uint8_t Face::nextframe(Expression::Id exp, uint8_t frame) const
	{
		try {
			// Add bounds checking to prevent crashes with invalid expressions
			if (exp < 0 || exp >= Expression::Id::LENGTH) {
				return 0; // Default frame if expression ID is invalid
			}
			
			return expressions[exp].count(frame + 1) ? frame + 1 : 0;
		}
		catch (...) {
			// Return default frame if face data is corrupted
			return 0;
		}
	}

	int16_t Face::get_delay(Expression::Id exp, uint8_t frame) const
	{
		try {
			// Add bounds checking to prevent crashes with invalid expressions
			if (exp < 0 || exp >= Expression::Id::LENGTH) {
				return 100; // Default delay if expression ID is invalid
			}
			
			auto delayit = expressions[exp].find(frame);
			return delayit != expressions[exp].end() ? delayit->second.delay : 100;
		}
		catch (...) {
			// Return default delay if face data is corrupted
			return 100;
		}
	}

	const std::string& Face::get_name() const
	{
		return name;
	}
}