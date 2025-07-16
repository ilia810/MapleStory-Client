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
#include <iostream>
#include <iomanip>
#include <algorithm>

namespace ms {
namespace Testing {

void TestRunner::runAll() {
    std::cout << "Running all tests..." << std::endl;
    
    for (auto& suite : suites_) {
        std::cout << "\nSuite: " << suite->getName() << std::endl;
        std::cout << "========================================" << std::endl;
        
        for (auto& test : suite->getTests()) {
            runTestCase(*test);
        }
    }
    
    printResults();
}

void TestRunner::runSuite(const std::string& suiteName) {
    auto it = std::find_if(suites_.begin(), suites_.end(),
        [&suiteName](const std::unique_ptr<TestSuite>& suite) {
            return suite->getName() == suiteName;
        });
    
    if (it == suites_.end()) {
        std::cerr << "Suite not found: " << suiteName << std::endl;
        return;
    }
    
    std::cout << "Running suite: " << suiteName << std::endl;
    std::cout << "========================================" << std::endl;
    
    for (auto& test : (*it)->getTests()) {
        runTestCase(*test);
    }
    
    printResults();
}

void TestRunner::runTest(const std::string& suiteName, const std::string& testName) {
    auto suiteIt = std::find_if(suites_.begin(), suites_.end(),
        [&suiteName](const std::unique_ptr<TestSuite>& suite) {
            return suite->getName() == suiteName;
        });
    
    if (suiteIt == suites_.end()) {
        std::cerr << "Suite not found: " << suiteName << std::endl;
        return;
    }
    
    auto& tests = (*suiteIt)->getTests();
    auto testIt = std::find_if(tests.begin(), tests.end(),
        [&testName](const std::unique_ptr<TestCase>& test) {
            return test->getName() == testName;
        });
    
    if (testIt == tests.end()) {
        std::cerr << "Test not found: " << testName << " in suite " << suiteName << std::endl;
        return;
    }
    
    runTestCase(**testIt);
    printResults();
}

void TestRunner::runTestCase(TestCase& test) {
    test.result_.name = test.getName();
    test.result_.status = TestStatus::RUNNING;
    test.status_ = TestStatus::RUNNING;
    
    auto start = std::chrono::steady_clock::now();
    
    try {
        std::cout << "Running test: " << test.getName() << "... ";
        
        test.setUp();
        test.run();
        test.tearDown();
        
        test.status_ = TestStatus::PASSED;
        test.result_.status = TestStatus::PASSED;
        test.result_.message = "Test passed";
        
        std::cout << "[PASS]" << std::endl;
    }
    catch (const std::exception& e) {
        if (test.status_ != TestStatus::FAILED && test.status_ != TestStatus::SKIPPED) {
            test.status_ = TestStatus::FAILED;
            test.result_.status = TestStatus::FAILED;
            test.result_.message = e.what();
        }
        
        if (test.status_ == TestStatus::SKIPPED) {
            std::cout << "[SKIP]" << std::endl;
        } else {
            std::cout << "[FAIL]" << std::endl;
            std::cerr << "  Error: " << e.what() << std::endl;
        }
        
        try {
            test.tearDown();
        } catch (...) {
            // Ignore tearDown failures
        }
    }
    
    auto end = std::chrono::steady_clock::now();
    test.result_.duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    
    results_.push_back(test.result_);
}

void TestRunner::printResults() const {
    std::cout << "\n========================================" << std::endl;
    std::cout << "Test Results Summary" << std::endl;
    std::cout << "========================================" << std::endl;
    
    int passed = 0, failed = 0, skipped = 0;
    
    for (const auto& result : results_) {
        switch (result.status) {
            case TestStatus::PASSED:
                passed++;
                break;
            case TestStatus::FAILED:
                failed++;
                break;
            case TestStatus::SKIPPED:
                skipped++;
                break;
            default:
                break;
        }
    }
    
    std::cout << "Total: " << results_.size() << std::endl;
    std::cout << "Passed: " << passed << std::endl;
    std::cout << "Failed: " << failed << std::endl;
    std::cout << "Skipped: " << skipped << std::endl;
    
    if (failed > 0) {
        std::cout << "\nFailed Tests:" << std::endl;
        for (const auto& result : results_) {
            if (result.status == TestStatus::FAILED) {
                std::cout << "  - " << result.name << ": " << result.message << std::endl;
            }
        }
    }
}

bool TestRunner::allTestsPassed() const {
    return std::all_of(results_.begin(), results_.end(),
        [](const TestResult& result) {
            return result.status == TestStatus::PASSED || result.status == TestStatus::SKIPPED;
        });
}

} // namespace Testing
} // namespace ms