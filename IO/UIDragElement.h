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

#include "UIElement.h"
#include "UIScale.h"

#include "../Configuration.h"

namespace ms
{
	template <typename T>
	// Base class for UI Windows which can be moved with the mouse cursor.
	class UIDragElement : public UIElement
	{
	public:
		void remove_cursor() override
		{
			UIElement::remove_cursor();

			if (dragged)
			{
				dragged = false;

				// Clamp position to keep window on screen before saving
				clamp_position_to_screen();
				Setting<T>::get().save(position);
			}
		}

		void makeactive() override
		{
			UIElement::makeactive();
			// Clamp position when window becomes active (handles off-screen saved positions)
			clamp_position_to_screen();
		}

		Cursor::State send_cursor(bool clicked, Point<int16_t> cursorpos) override
		{
			// Transform cursor from screen space to UI space for scaled dragging
			Point<int16_t> ui_cursor = UIScale::get().screen_to_ui(cursorpos);

			if (clicked)
			{
				if (dragged)
				{
					// Update position in UI space (unscaled coordinates)
					position = ui_cursor - cursoroffset;

					return Cursor::State::CLICKING;
				}
				else if (indragrange(ui_cursor))
				{
					// Store offset in UI space
					cursoroffset = ui_cursor - position;
					dragged = true;

					return UIElement::send_cursor(clicked, cursorpos);
				}
			}
			else
			{
				if (dragged)
				{
					dragged = false;

					// Clamp position to keep window on screen before saving
					clamp_position_to_screen();
					Setting<T>::get().save(position);
				}
			}

			return UIElement::send_cursor(clicked, cursorpos);
		}

	protected:
		UIDragElement() : UIDragElement(Point<int16_t>(0, 0)) {}

		UIDragElement(Point<int16_t> d) : dragarea(d)
		{
			position = Setting<T>::get().load();
		}

		bool dragged = false;
		Point<int16_t> dragarea;
		Point<int16_t> cursoroffset;

	private:
		virtual bool indragrange(Point<int16_t> cursorpos) const
		{
			// cursorpos is already in UI space (transformed in send_cursor)
			auto bounds = Rectangle<int16_t>(position, position + dragarea);

			return bounds.contains(cursorpos);
		}
	};
}