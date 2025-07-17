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

#include <string>
#include <vector>
#include <memory>
#include <functional>
#include <chrono>
#include <iostream>
#include <sstream>

namespace ms {
namespace Testing {

enum class TestStatus {
    NOT_RUN,
    RUNNING,
    PASSED,
    FAILED,
    SKIPPED,
    TIMEOUT
};

struct TestResult {
    std::string name;
    TestStatus status;
    std::string message;
    std::chrono::milliseconds duration;
    std::vector<std::string> logs;
};

class TestCase {
public:
    TestCase(const std::string& name) : name_(name), status_(TestStatus::NOT_RUN) {}
    virtual ~TestCase() = default;

    virtual void setUp() {}
    virtual void tearDown() {}
    virtual void run() = 0;

    const std::string& getName() const { return name_; }
    TestStatus getStatus() const { return status_; }
    const TestResult& getResult() const { return result_; }

protected:
    void assert(bool condition, const std::string& message) {
        if (!condition) {
            fail(message);
        }
    }

    void assertEqual(int expected, int actual, const std::string& message = "") {
        if (expected != actual) {
            std::stringstream ss;
            ss << "Expected: " << expected << ", Actual: " << actual;
            if (!message.empty()) {
                ss << " - " << message;
            }
            fail(ss.str());
        }
    }

    void assertNotNull(void* ptr, const std::string& message = "") {
        if (ptr == nullptr) {
            fail(message.empty() ? "Pointer is null" : message);
        }
    }

    void log(const std::string& message) {
        result_.logs.push_back(message);
    }

    void fail(const std::string& message) {
        status_ = TestStatus::FAILED;
        result_.message = message;
        throw std::runtime_error(message);
    }

    void skip(const std::string& reason) {
        status_ = TestStatus::SKIPPED;
        result_.message = reason;
        throw std::runtime_error("Test skipped: " + reason);
    }

private:
    friend class TestRunner;
    
    std::string name_;
    TestStatus status_;
    TestResult result_;
};

class TestSuite {
public:
    TestSuite(const std::string& name) : name_(name) {}

    void addTest(std::unique_ptr<TestCase> test) {
        tests_.push_back(std::move(test));
    }

    const std::string& getName() const { return name_; }
    const std::vector<std::unique_ptr<TestCase>>& getTests() const { return tests_; }

private:
    std::string name_;
    std::vector<std::unique_ptr<TestCase>> tests_;
};

class TestRunner {
public:
    TestRunner() : timeout_ms_(30000) {}

    void setTimeout(int milliseconds) {
        timeout_ms_ = milliseconds;
    }

    void addSuite(std::unique_ptr<TestSuite> suite) {
        suites_.push_back(std::move(suite));
    }

    void runAll();
    void runSuite(const std::string& suiteName);
    void runTest(const std::string& suiteName, const std::string& testName);

    void printResults() const;
    bool allTestsPassed() const;

private:
    void runTestCase(TestCase& test);

    std::vector<std::unique_ptr<TestSuite>> suites_;
    std::vector<TestResult> results_;
    int timeout_ms_;
};

template<typename T>
class TestRegistrar {
public:
    TestRegistrar(const std::string& suiteName, const std::string& testName) {
        getRegistry().emplace_back(suiteName, testName, []() { return std::make_unique<T>(); });
    }

    struct Entry {
        std::string suiteName;
        std::string testName;
        std::function<std::unique_ptr<TestCase>()> factory;
    };

    static std::vector<Entry>& getRegistry() {
        static std::vector<Entry> registry;
        return registry;
    }
};

#define TEST(SuiteName, TestName) \
    class SuiteName##_##TestName : public ::ms::Testing::TestCase { \
    public: \
        SuiteName##_##TestName() : TestCase(#TestName) {} \
        void run() override; \
    }; \
    static ::ms::Testing::TestRegistrar<SuiteName##_##TestName> \
        SuiteName##_##TestName##_registrar(#SuiteName, #TestName); \
    void SuiteName##_##TestName::run()

} // namespace Testing
} // namespace ms