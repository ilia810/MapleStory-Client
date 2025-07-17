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
#include "TestFramework.h"
#include "HeadlessMode.h"
#include "../Configuration.h"
#include "../Util/NxFiles.h"
#include <iostream>
#include <string>
#include <vector>
#include <algorithm>
#include <map>

void printUsage() {
    std::cout << "MapleStory Client Test Runner" << std::endl;
    std::cout << "=============================" << std::endl;
    std::cout << "Usage: MapleStory.exe --test [options]" << std::endl;
    std::cout << std::endl;
    std::cout << "Options:" << std::endl;
    std::cout << "  --test                Run all tests" << std::endl;
    std::cout << "  --test-suite <name>   Run specific test suite" << std::endl;
    std::cout << "  --test-case <suite> <test>  Run specific test case" << std::endl;
    std::cout << "  --test-list           List all available tests" << std::endl;
    std::cout << "  --test-timeout <ms>   Set test timeout in milliseconds (default: 30000)" << std::endl;
    std::cout << "  --test-graphics       Enable graphics during tests" << std::endl;
    std::cout << "  --test-help           Show this help message" << std::endl;
    std::cout << std::endl;
    std::cout << "Examples:" << std::endl;
    std::cout << "  MapleStory.exe --test" << std::endl;
    std::cout << "  MapleStory.exe --test-suite MapLoading" << std::endl;
    std::cout << "  MapleStory.exe --test-case MapLoading LoadHenesys" << std::endl;
}

void listTests() {
    std::cout << "Available Tests:" << std::endl;
    std::cout << "================" << std::endl;
    
    std::map<std::string, std::vector<std::string>> suites;
    
    for (const auto& entry : ms::Testing::TestRegistrar<ms::Testing::TestCase>::getRegistry()) {
        suites[entry.suiteName].push_back(entry.testName);
    }
    
    for (const auto& suite : suites) {
        std::cout << "\n" << suite.first << ":" << std::endl;
        for (const auto& test : suite.second) {
            std::cout << "  - " << test << std::endl;
        }
    }
}

int runTests(int argc, char* argv[]) {
    using namespace ms;
    using namespace ms::Testing;
    
    Configuration& config = Configuration::get();
    TestRunner runner;
    HeadlessMode& headless = HeadlessMode::getInstance();
    
    bool enableGraphics = false;
    std::string suiteName;
    std::string testName;
    int timeout = 30000;
    
    for (int i = 2; i < argc; i++) {
        std::string arg = argv[i];
        
        if (arg == "--test-help") {
            printUsage();
            return 0;
        }
        else if (arg == "--test-list") {
            listTests();
            return 0;
        }
        else if (arg == "--test-suite" && i + 1 < argc) {
            suiteName = argv[++i];
        }
        else if (arg == "--test-case" && i + 2 < argc) {
            suiteName = argv[++i];
            testName = argv[++i];
        }
        else if (arg == "--test-timeout" && i + 1 < argc) {
            timeout = std::stoi(argv[++i]);
        }
        else if (arg == "--test-graphics") {
            enableGraphics = true;
        }
    }
    
    std::cout << "Initializing test environment..." << std::endl;
    
    headless.enableGraphics(enableGraphics);
    
    Error initError = headless.init();
    if (initError) {
        std::cerr << "Failed to initialize headless mode: " << initError.get_message() << std::endl;
        return 1;
    }
    
    Error nxError = NxFiles::init();
    if (nxError) {
        std::cerr << "Failed to initialize NX files: " << nxError.get_message() << std::endl;
        return 1;
    }
    
    runner.setTimeout(timeout);
    
    std::map<std::string, std::unique_ptr<TestSuite>> testSuites;
    for (const auto& entry : TestRegistrar<TestCase>::getRegistry()) {
        if (testSuites.find(entry.suiteName) == testSuites.end()) {
            testSuites[entry.suiteName] = std::make_unique<TestSuite>(entry.suiteName);
        }
        testSuites[entry.suiteName]->addTest(entry.factory());
    }
    
    for (auto& pair : testSuites) {
        runner.addSuite(std::move(pair.second));
    }
    
    if (!suiteName.empty() && !testName.empty()) {
        runner.runTest(suiteName, testName);
    }
    else if (!suiteName.empty()) {
        runner.runSuite(suiteName);
    }
    else {
        runner.runAll();
    }
    
    headless.stop();
    
    return runner.allTestsPassed() ? 0 : 1;
}

namespace ms {
namespace Testing {

bool isTestMode(int argc, char* argv[]) {
    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        if (arg == "--test" || arg.find("--test-") == 0) {
            return true;
        }
    }
    return false;
}

} // namespace Testing
} // namespace ms