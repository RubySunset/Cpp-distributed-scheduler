#include <gtest/gtest.h>
#include "task.h"

class TaskTest : public ::testing::Test {
protected:
    void SetUp() override {}
    void TearDown() override {}
};

TEST_F(TaskTest, ConstructorAndGetters) {
    Task task(1, [](){}, 5);
    EXPECT_EQ(task.getId(), 1);
    EXPECT_EQ(task.getPriority(), 5);
}

TEST_F(TaskTest, Execute) {
    bool executed = false;
    Task task(1, [&executed](){ executed = true; }, 5);
    task.execute();
    EXPECT_TRUE(executed);
}

TEST_F(TaskTest, InvalidPriority) {
    EXPECT_THROW(Task(1, [](){}, 0), std::invalid_argument);
    EXPECT_THROW(Task(1, [](){}, 11), std::invalid_argument);
}