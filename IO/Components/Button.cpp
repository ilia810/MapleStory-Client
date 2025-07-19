#include "Button.h"

namespace ms
{
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