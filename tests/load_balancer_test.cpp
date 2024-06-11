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

// TEST_F(LoadBalancerTest, AddAndRemoveWorker) {
//     lb.addWorker(4);
//     EXPECT_EQ(lb.getAvailableWorker(), 4);
//     lb.removeWorker(4);
//     EXPECT_NE(lb.getAvailableWorker(), 4);
// }

// TEST_F(LoadBalancerTest, UpdateWorkerLoad) {
//     for (int i = 0; i < 5; ++i) {
//         lb.incLoad(1);
//     }
//     for (int i = 0; i < 3; ++i) {
//         lb.incLoad(2);
//     }
//     for (int i = 0; i < 7; ++i) {
//         lb.incLoad(3);
//     }
//     EXPECT_EQ(lb.getAvailableWorker(), 2);
// }

// TEST_F(LoadBalancerTest, AssignAndGetTask) {
//     auto task = std::make_shared<Task>(1, [](){}, 5);
//     lb.addTask(task);
//     EXPECT_TRUE(lb.hasTasks());
//     auto assigned_task = lb.getNextTask();
//     EXPECT_EQ(assigned_task->getId(), 1);
//     EXPECT_FALSE(lb.hasTasks());
// }