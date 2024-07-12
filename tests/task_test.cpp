#include <gtest/gtest.h>
#include "task.h"

class TaskTest : public ::testing::Test {
protected:
    void SetUp() override {}
    void TearDown() override {}
};

TEST_F(TaskTest, TaskRequestSerialization) {
    TaskRequest request{3, 10, "func", std::unordered_map<std::string, std::string>{{"name1", "arg1"}, {"name2", "arg2"}}};
    TaskRequest recovered_request(request.to_string());
    ASSERT_EQ(request, recovered_request);
}

TEST_F(TaskTest, TaskResponseSerialization) {
    TaskResponse response{3, true, "return_val"};
    TaskResponse recovered_response(response.to_string());
    ASSERT_EQ(response, recovered_response);
}
