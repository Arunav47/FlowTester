#include <gtest/gtest.h>

#include "flowtester/workflow.hpp"
#include "parser.hpp"
#include <string>

namespace flowtester {

class WorkflowParserTest : public ::testing::Test {
protected:
    WorkflowParser parser;

    Workflow parseExample() {
        return parser.parseFile(std::string(FLOWTESTER_SOURCE_DIR) + "/examples/ci.yml");
    }
};

TEST_F(WorkflowParserTest, ParsesWorkflowName) {
    const Workflow workflow = parseExample();

    EXPECT_EQ(workflow.name, "FlowTester CI");
}

TEST_F(WorkflowParserTest, ParsesPushTrigger) {
    const Workflow workflow = parseExample();

    ASSERT_FALSE(workflow.triggers.empty());

    EXPECT_EQ(workflow.triggers[0].event, "push");

    ASSERT_EQ(workflow.triggers[0].branches.size(), 2);
    EXPECT_EQ(workflow.triggers[0].branches[0], "main");
    EXPECT_EQ(workflow.triggers[0].branches[1], "develop");
}

TEST_F(WorkflowParserTest, ParsesJobs) {
    const Workflow workflow = parseExample();

    ASSERT_EQ(workflow.jobs.size(), 2);

    EXPECT_EQ(workflow.jobs[0].id, "test");
    EXPECT_EQ(workflow.jobs[0].name, "Run Tests");

    EXPECT_EQ(workflow.jobs[1].id, "build");
    EXPECT_EQ(workflow.jobs[1].name, "Build Application");
}

TEST_F(WorkflowParserTest, ParsesJobDependencies) {
    const Workflow workflow = parseExample();

    ASSERT_EQ(workflow.jobs.size(), 2);

    ASSERT_EQ(workflow.jobs[1].needs.size(), 1);
    EXPECT_EQ(workflow.jobs[1].needs[0], "test");
}

TEST_F(WorkflowParserTest, ParsesSteps) {
    const Workflow workflow = parseExample();

    ASSERT_FALSE(workflow.jobs.empty());

    const Job& job = workflow.jobs[0];

    ASSERT_EQ(job.steps.size(), 2);

    EXPECT_EQ(job.steps[0].name, "Checkout");
    EXPECT_EQ(job.steps[0].uses, "actions/checkout@v4");

    EXPECT_EQ(job.steps[1].name, "Run unit tests");
    EXPECT_EQ(job.steps[1].run, "npm test");
}

TEST_F(WorkflowParserTest, ParsesEnvironmentVariables) {
    const Workflow workflow = parseExample();

    ASSERT_FALSE(workflow.jobs.empty());

    const Job& job = workflow.jobs[0];

    ASSERT_TRUE(job.env.contains("NODE_ENV"));
    EXPECT_EQ(job.env.at("NODE_ENV"), "test");
}

} // namespace flowrunner