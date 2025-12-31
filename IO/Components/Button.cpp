#include "Button.h"

namespace ms
{
	void Button::draw(Point<int16_t> parentpos, const DrawArgument& scale_args) const
	{
		// Default implementation: ignore scale and call unscaled draw
		// Override in derived classes for proper scaled rendering
		(void)scale_args;  // Unused in base implementation
		draw(parentpos);
	}

	bool Button::in_combobox(Point<int16_t>)
	{
		return false;
	}

	uint16_t Button::get_selected() const
	{
		return uint16_t();
	}

	void Button::set_position(Point<int16_t> pos)
	{
		position = pos;
	}

	void Button::set_state(State s)
	{
		if (s == Button::State::IDENTITY)
			return;

		state = s;
	}

	void Button::set_active(bool a)
	{
		active = a;
	}

	void Button::toggle_pressed()
	{
		pressed = !pressed;
	}

	bool Button::is_active() const
	{
		return active && state != Button::State::DISABLED;
	}

	Button::State Button::get_state() const
	{
		return state;
	}

	bool Button::is_pressed() const
	{
		return pressed;
	}
}		