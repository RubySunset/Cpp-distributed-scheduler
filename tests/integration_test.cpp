#include <gtest/gtest.h>
#include <thread>
#include <vector>
#include <atomic>
#include <chrono>
#include <sstream>
#include <iostream>
#include <queue>
#include <mutex>
#include <condition_variable>

#include "master.h"
#include "worker.h"

class IntegrationTest : public ::testing::Test {
protected:
    Master master{8080};
    std::vector<std::unique_ptr<Worker>> workers;
    std::vector<std::thread> worker_threads;

    void SetUp() override {
        for (int i = 0; i < 3; ++i) {
            workers.push_back(std::make_unique<Worker>("127.0.0.1", 8080));
            worker_threads.emplace_back([this, i]() {
                workers[i]->run();
            });
        }

        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    void TearDown() override {
        master.stop();
        for (int i = 0; i < workers.size(); ++i) {
            workers[i]->stop();
            if (worker_threads[i].joinable()) {
                worker_threads[i].join();
            }
        }
    }
};

TEST_F(IntegrationTest, TaskDistributionAndCompletion) {
    std::unordered_map<std::string, std::string> arguments{{"k1", "v1"}};
    std::vector<std::thread> task_threads;
    for (int i = 0; i < 100; ++i) {
        task_threads.emplace_back([&]{
            auto result = master.process(1, "test", arguments);
            ASSERT_EQ(result, std::optional<std::string>{"k1:v1&"});
        });
    }
    for (int i = 0; i < 100; ++i) {
        if (task_threads[i].joinable()) {
            task_threads[i].join();
        }
    }
}