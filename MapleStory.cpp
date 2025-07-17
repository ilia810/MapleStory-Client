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
#include "Gameplay/Stage.h"
#include "IO/UI.h"
#include "IO/Window.h"
#include "quick_nx_test.cpp"
#include "Net/Session.h"
#include "Util/HardwareInfo.h"
#include "Util/ScreenResolution.h"

#include <iostream>
#include <fstream>

#ifdef USE_NX
#include "Util/NxFiles.h"
#else
#include "Util/WzFiles.h"
#endif

// #include "Testing/TestMain.cpp"

namespace ms
{
	Error init()
	{
		if (Error error = Session::get().init())
			return error;

#ifdef USE_NX
		if (Error error = NxFiles::init())
			return error;
#else
		if (Error error = WzFiles::init())
			return error;
#endif

		if (Error error = Window::get().init())
			return error;

		if (Error error = Sound::init())
			return error;

		if (Error error = Music::init())
			return error;

		Char::init();
		DamageNumber::init();
		MapPortals::init();
		Stage::get().init();
		UI::get().init();

		return Error::NONE;
	}

	void update()
	{
		Window::get().check_events();
		Window::get().update();
		Stage::get().update();
		UI::get().update();
		Session::get().read();
	}

	void draw(float alpha)
	{
		Window::get().begin();
		Stage::get().draw(alpha);
		UI::get().draw(alpha);
		Window::get().end();
	}

	bool running()
	{
		bool session_connected = Session::get().is_connected();
		bool ui_not_quitted = UI::get().not_quitted();
		bool window_not_closed = Window::get().not_closed();
		
		// Allow running during login screen even without session connection
		bool should_run = ui_not_quitted && window_not_closed;
		
		// Main loop state check
		
		return should_run;
	}

	void loop()
	{
		Timer::get().start();

		int64_t timestep = Constants::TIMESTEP * 1000;
		int64_t accumulator = timestep;

		int64_t period = 0;
		int32_t samples = 0;

		bool show_fps = Configuration::get().get_show_fps();

		while (running())
		{
			int64_t elapsed = Timer::get().stop();

			// Update game with constant timestep as many times as possible.
			for (accumulator += elapsed; accumulator >= timestep; accumulator -= timestep)
				update();

			// Draw the game. Interpolate to account for remaining time.
			float alpha = static_cast<float>(accumulator) / timestep;
			draw(alpha);

			if (show_fps)
			{
				if (samples < 100)
				{
					period += elapsed;
					samples++;
				}
				else if (period)
				{
					int64_t fps = (samples * 1000000) / period;

					LOG(LOG_INFO, "FPS: " << fps);

					period = 0;
					samples = 0;
				}
			}
		}

		Sound::close();
	}

	void start()
	{
		// Initialize and check for errors
		if (Error error = init())
		{
			const char* message = error.get_message();
			const char* args = error.get_args();
			bool can_retry = error.can_retry();

			if (args && args[0])
				LOG(LOG_ERROR, message << args);
			else
				LOG(LOG_ERROR, message);

			if (can_retry)
				LOG(LOG_INFO, "Enter 'retry' to try agC:\HeavenClient\NoLifeWzToNxain.");

			std::string command;
			std::cin >> command;

			if (can_retry && command == "retry")
				start();
		}
		else
		{
			loop();
		}
	}
}

#ifdef _DEBUG
int main(int argc, char* argv[])
#else
int APIENTRY WinMain(HINSTANCE hInst, HINSTANCE hInstPrev, PSTR cmdline, int cmdshow)
#endif
{
#ifdef _DEBUG
	// Check if running in test mode
	// if (ms::Testing::isTestMode(argc, argv))
	// {
	//	return runTests(argc, argv);
	// }
#else
	// Parse command line in release mode
	int argc = __argc;
	char** argv = __argv;
	
	// if (ms::Testing::isTestMode(argc, argv))
	// {
	//	return runTests(argc, argv);
	// }
#endif

	// Redirect stdout and stderr to debug_output.txt, clearing it on each run
	//static std::ofstream debug_file("debug_output.txt", std::ios::trunc);
	//static std::streambuf* orig_cout = std::cout.rdbuf();
	//static std::streambuf* orig_cerr = std::cerr.rdbuf();
	
	//std::cout.rdbuf(debug_file.rdbuf());
	//std::cerr.rdbuf(debug_file.rdbuf());
	
	ms::HardwareInfo();
	ms::ScreenResolution();
	ms::start();

	// Restore original streams before exit
	//std::cout.rdbuf(orig_cout);
	//std::cerr.rdbuf(orig_cerr);
	//debug_file.close();

	return 0;
}