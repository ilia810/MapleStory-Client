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
#include "HeadlessMode.h"
#include "../Gameplay/Stage.h"
#include "../IO/UI.h"
#include "../IO/Window.h"
#include "../Net/Session.h"
#include "../Audio/Audio.h"
#include "../Constants.h"
#include "../Timer.h"
#include <chrono>
#include <thread>

namespace ms {
namespace Testing {

bool MockGraphics::enabled_ = false;

void MockGraphics::enable() {
    enabled_ = true;
}

void MockGraphics::disable() {
    enabled_ = false;
}

bool MockGraphics::isEnabled() {
    return enabled_;
}

HeadlessMode::HeadlessMode() : running_(false), graphics_enabled_(false), last_update_(0) {
}

HeadlessMode::~HeadlessMode() {
    stop();
}

Error HeadlessMode::init() {
    MockGraphics::disable();
    
    if (graphics_enabled_) {
        Error window_error = Window::get().init();
        if (window_error) {
            return window_error;
        }
    }
    
    Audio::get().set_sfxvolume(0);
    Audio::get().set_bgmvolume(0);
    
    stage_ = std::make_unique<Stage>();
    ui_ = std::make_unique<UI>();
    
    Session::get().init();
    
    return Error::NONE;
}

void HeadlessMode::run() {
    running_ = true;
    
    while (running_) {
        update();
        processEvents();
        
        std::this_thread::sleep_for(std::chrono::milliseconds(16));
    }
}

void HeadlessMode::stop() {
    running_ = false;
    
    if (Session::get().is_connected()) {
        Session::get().disconnect();
    }
}

void HeadlessMode::update() {
    int64_t now = Timer::get().time();
    int16_t timestep = static_cast<int16_t>(now - last_update_);
    last_update_ = now;
    
    Window::get().update();
    stage_->update();
    ui_->update();
    Session::get().update();
}

void HeadlessMode::processEvents() {
    Window::get().poll_events();
}

void HeadlessMode::simulateKeyPress(int32_t keycode) {
    if (ui_) {
        ui_->send_key(keycode, true);
    }
}

void HeadlessMode::simulateKeyRelease(int32_t keycode) {
    if (ui_) {
        ui_->send_key(keycode, false);
    }
}

void HeadlessMode::simulateMouseMove(int16_t x, int16_t y) {
    if (ui_) {
        ui_->send_cursor(false, Point<int16_t>(x, y));
    }
}

void HeadlessMode::simulateMouseClick(int16_t x, int16_t y, bool left) {
    if (ui_) {
        ui_->send_cursor(true, Point<int16_t>(x, y));
        if (left) {
            ui_->doubleclick(Point<int16_t>(x, y));
        } else {
            ui_->rightclick(Point<int16_t>(x, y));
        }
    }
}

void HeadlessMode::waitForMapLoad(int32_t mapid, int timeout_ms) {
    auto condition = [this, mapid]() {
        return isMapLoaded(mapid);
    };
    waitForCondition(condition, timeout_ms);
}

void HeadlessMode::waitForUIElement(const std::string& element, int timeout_ms) {
    auto condition = [this, element]() {
        return isUIElementVisible(element);
    };
    waitForCondition(condition, timeout_ms);
}

void HeadlessMode::waitForCondition(std::function<bool()> condition, int timeout_ms) {
    auto start = std::chrono::steady_clock::now();
    auto timeout = std::chrono::milliseconds(timeout_ms);
    
    while (!condition()) {
        update();
        processEvents();
        
        auto elapsed = std::chrono::steady_clock::now() - start;
        if (elapsed >= timeout) {
            break;
        }
        
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
}

bool HeadlessMode::isMapLoaded(int32_t mapid) const {
    return stage_ && stage_->get_mapid() == mapid;
}

bool HeadlessMode::isUIElementVisible(const std::string& element) const {
    return ui_ && ui_->is_visible(UIElement::Type::NUM_TYPES);
}

bool HeadlessMode::isConnected() const {
    return Session::get().is_connected();
}

} // namespace Testing
} // namespace ms