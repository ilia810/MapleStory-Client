//////////////////////////////////////////////////////////////////////////////////
//	This file is part of the continued Journey MMORPG client					//
//	Copyright (C) 2015-2019  Daniel Allendorf, Ryan Payton						//
//																				//
//	This program is free software: you can redistribute it and/or modify		//
//	it under the terms of the GNU Affero General Public License as published by//
//	the Free Software Foundation, either version 3 of the License, or			//
//	(at your option) any later version.										//
//																				//
//	This program is distributed in the hope that it will be useful,			//
//	but WITHOUT ANY WARRANTY; without even the implied warranty of				//
//	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the				//
//	GNU Affero General Public License for more details.						//
//																				//
//	You should have received a copy of the GNU Affero General Public License	//
//	along with this program.  If not, see <https://www.gnu.org/licenses/>.		//
//////////////////////////////////////////////////////////////////////////////////
#pragma once

#include "../Configuration.h"
#include "../Error.h"
#include <memory>
#include <functional>

namespace ms {

class Stage;
class UI;

namespace Testing {

class HeadlessMode {
public:
    static HeadlessMode& getInstance() {
        static HeadlessMode instance;
        return instance;
    }

    Error init();
    void run();
    void stop();

    void simulateKeyPress(int32_t keycode);
    void simulateKeyRelease(int32_t keycode);
    void simulateMouseMove(int16_t x, int16_t y);
    void simulateMouseClick(int16_t x, int16_t y, bool left = true);
    
    void waitForMapLoad(int32_t mapid, int timeout_ms = 10000);
    void waitForUIElement(const std::string& element, int timeout_ms = 5000);
    void waitForCondition(std::function<bool()> condition, int timeout_ms = 5000);
    
    bool isMapLoaded(int32_t mapid) const;
    bool isUIElementVisible(const std::string& element) const;
    bool isConnected() const;
    
    Stage* getStage() { return stage_.get(); }
    UI* getUI() { return ui_.get(); }

    void enableGraphics(bool enable) { graphics_enabled_ = enable; }
    bool isGraphicsEnabled() const { return graphics_enabled_; }

private:
    HeadlessMode();
    ~HeadlessMode();

    void update();
    void processEvents();

    std::unique_ptr<Stage> stage_;
    std::unique_ptr<UI> ui_;
    
    bool running_;
    bool graphics_enabled_;
    int64_t last_update_;
};

class MockGraphics {
public:
    static void enable();
    static void disable();
    static bool isEnabled();

private:
    static bool enabled_;
};

} // namespace Testing
} // namespace ms