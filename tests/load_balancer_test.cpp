#include <gtest/gtest.h>
#include "load_balancer.h"

class LoadBalancerTest : public ::testing::Test {
protected:
    LoadBalancer lb;
    int assigned_worker = -1;
    int assigned_task = -1;
    void SetUp() override {
        lb.set_dispatch_callback([this](int worker, std::shared_ptr<TaskRequest> request){
            if (assigned_worker == -1) {
                assigned_worker = worker;
                assigned_task = request->id;
            }
        });
        lb.startDispatchLoop();
    }
    void TearDown() override {
        lb.stopDispatchLoop();
    }
};

TEST_F(LoadBalancerTest, DispatchTask) {
    lb.addWorker(1);
    lb.addTask(std::make_shared<TaskRequest>(1, 1, "func", std::unordered_map<std::string, std::string>{}));
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    ASSERT_EQ(assigned_worker, 1);
    ASSERT_EQ(assigned_task, 1);
}

TEST_F(LoadBalancerTest, DispatchTaskPriority) {
    lb.addTask(std::make_shared<TaskRequest>(1, 1, "func", std::unordered_map<std::string, std::string>{}));
    lb.addTask(std::make_shared<TaskRequest>(2, 2, "func", std::unordered_map<std::string, std::string>{}));
    lb.addWorker(1);
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    ASSERT_EQ(assigned_worker, 1);
    ASSERT_EQ(assigned_task, 2);
}

TEST_F(LoadBalancerTest, DispatchTaskWorkerLoad) {
    lb.addWorker(1);
    lb.addWorker(2);
    lb.incLoad(2);
    lb.addTask(std::make_shared<TaskRequest>(1, 1, "func", std::unordered_map<std::string, std::string>{}));
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    ASSERT_EQ(assigned_worker, 1);
    ASSERT_EQ(assigned_task, 1);
}

TEST_F(LoadBalancerTest, DispatchTaskNoWorker) {
    lb.addWorker(1);
    for (int i = 0; i < 100; ++i) {
        lb.incLoad(1);
    }
    lb.addTask(std::make_shared<TaskRequest>(1, 1, "func", std::unordered_map<std::string, std::string>{}));
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    ASSERT_EQ(assigned_worker, -1);
    ASSERT_EQ(assigned_task, -1);
}