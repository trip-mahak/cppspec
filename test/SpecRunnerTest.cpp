/*
 * Copyright 2009 Timo Puronen
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <gtest/gtest.h>
#include "SpecRunner.h"
#include "OutputStreamStub.h"
#include "SpecificationRegistry.h"
#include "SpecDoxReporter.h"
#include "JUnitReporter.h"
#include "Runnable.h"
#include "TimerStub.h"
#include "SpecResult.h"
#include "Needle/Binder.h"

using CppSpec::SpecRunner;
using CppSpec::SpecificationRegistry;
using CppSpec::SpecDoxReporter;
using CppSpec::JUnitReporter;
using CppSpec::Runnable;
using CppSpec::Reporter;
using CppSpec::SpecResult;

namespace CppSpec {
    class SpecRunnerTestAccessor {
    public:
        template<class ExpectedReporterType>
        void checkThatGivenReporterIsCreated(SpecRunner& specRunner) {
            OutputStreamStub output;
            Reporter* reporter = specRunner.createReporter(output);
            ExpectedReporterType* expectedReporterType = dynamic_cast<ExpectedReporterType*>(reporter);
            EXPECT_TRUE(expectedReporterType != NULL);
            delete reporter;
        }
    };
}
using CppSpec::SpecRunnerTestAccessor;

class SpecRunnerStub : public CppSpec::SpecRunner {
public:
    SpecRunnerStub(int argc, const char* argv[]) : CppSpec::SpecRunner(argc, argv) {}
protected:
    CppSpec::OutputStream* createOutputStream() {return new OutputStreamStub();} 
};

SpecRunner* createSpecRunner() {
    const char* args[] = {"test"};
	SpecificationRegistry::instance().clear();
    return new SpecRunnerStub(1, args);
}

class SpecRunnerTest : public ::testing::Test {
public:
    void SetUp() {
        Needle::Binder::instance().bind<CppSpec::Timer>(new TimerStub(), "behavior");
        Needle::Binder::instance().bind<CppSpec::Timer>(new TimerStub(), "spec");
    }
    
    void TearDown() {
        Needle::Binder::instance().deleteBindings();
    }
};

TEST_F(SpecRunnerTest, SpecDoxReporterIsReturnedByDefault) {
	SpecRunner* specRunner = createSpecRunner();
    SpecRunnerTestAccessor().checkThatGivenReporterIsCreated<SpecDoxReporter>(*specRunner);
	delete specRunner;
}

TEST_F(SpecRunnerTest, CreateReporterReturnsJUnitReporterIfGivenInArguments) {
    const char* args[] = {"test", "-o", "junit", "--report-dir", "foo"};
    SpecRunner specRunner(5, args);
    SpecRunnerTestAccessor().checkThatGivenReporterIsCreated<JUnitReporter>(specRunner);
}

TEST_F(SpecRunnerTest, CreateReporterReturnsJUnitReporterWithoutLogFilesIfGivenInArguments) {
    const char* args[] = {"test", "-o", "junit", "--no-logs"};
    SpecRunner specRunner(4, args);
    SpecRunnerTestAccessor().checkThatGivenReporterIsCreated<JUnitReporter>(specRunner);
}

TEST_F(SpecRunnerTest, ReturnZeroIfNoTestsExecuted) {
    SpecRunner* specRunner = createSpecRunner();
	EXPECT_EQ(0, specRunner->runSpecifications());
	delete specRunner;
}

struct DummyRunnable : public Runnable {
	DummyRunnable() : name("dummy") {}
	void operator()() {
        Needle::Inject<Reporter> reporter;
        SpecResult result("dummy");
        result.addFail("fail", "00.01000", "dummy failed");
        reporter->addSpecification(result);
	}
	const std::string& getName() const {return name;}
	unsigned int getBehaviorCount() const {return 1;}
	const std::string name;
};

TEST_F(SpecRunnerTest, ReturnOneIfATestFails) {
	SpecRunner* specRunner = createSpecRunner();
	DummyRunnable runnable;
	SpecificationRegistry::instance().addSpecification(&runnable);
	EXPECT_EQ(1, specRunner->runSpecifications());
	delete specRunner;
}
