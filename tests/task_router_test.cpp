#include <gtest/gtest.h>
#include "task_router.h"

class TaskRouterTest : public ::testing::Test {
protected:
    TaskRouter router;

    void SetUp() override {}
    void TearDown() override {}
};

TEST_F(TaskRouterTest, TaskRouterValidRoute) {
    TaskRequest request{3, 10, "mult", std::unordered_map<std::string, std::string>{{"2", "3"}, {"5", "4"}}};
    auto result = router.eval_route(request);
    ASSERT_EQ(result, std::optional<std::string>{"26"});
}

TEST_F(TaskRouterTest, TaskRouterInvalidRoute) {
    TaskRequest request{3, 10, "nofunc", std::unordered_map<std::string, std::string>{{"name1", "arg1"}, {"name2", "arg2"}}};
    auto result = router.eval_route(request);
    ASSERT_EQ(result, std::optional<std::string>{});
}