#include <gtest/gtest.h>
#include "load_balancer.h"

class LoadBalancerTest : public ::testing::Test {
protected:
    LoadBalancer lb;
    void SetUp() override {
        lb.addWorker(1);
        lb.addWorker(2);
        lb.addWorker(3);
    }
    void TearDown() override {}
};

TEST_F(LoadBalancerTest, AddAndRemoveWorker) {
    lb.addWorker(4);
    EXPECT_EQ(lb.getAvailableWorker(), 4);
    lb.removeWorker(4);
    EXPECT_NE(lb.getAvailableWorker(), 4);
}

TEST_F(LoadBalancerTest, UpdateWorkerLoad) {
    lb.updateWorkerLoad(1, 5);
    lb.updateWorkerLoad(2, 3);
    lb.updateWorkerLoad(3, 7);
    EXPECT_EQ(lb.getAvailableWorker(), 2);
}

TEST_F(LoadBalancerTest, AssignAndGetTask) {
    auto task = std::make_shared<Task>(1, [](){}, 5);
    lb.addTask(task);
    EXPECT_TRUE(lb.hasTasks());
    auto assigned_task = lb.getNextTask();
    EXPECT_EQ(assigned_task->getId(), 1);
    EXPECT_FALSE(lb.hasTasks());
}