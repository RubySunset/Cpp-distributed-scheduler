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
    std::unique_ptr<Master> master;
    std::vector<std::unique_ptr<Worker>> workers;
    std::thread master_thread;
    std::vector<std::thread> worker_threads;
    std::atomic<bool> should_stop{false};

    // For simulating cin input
    std::queue<std::string> input_queue;
    std::mutex input_mutex;
    std::condition_variable input_cv;

    // For capturing cout output
    std::stringstream output_buffer;
    std::mutex output_mutex;

    void SetUp() override {
        // Redirect cout
        std::cout.rdbuf(output_buffer.rdbuf());

        master = std::make_unique<Master>(8080);
        master_thread = std::thread([this]() {
            master->run();
        });

        for (int i = 0; i < 3; ++i) {
            workers.push_back(std::make_unique<Worker>("127.0.0.1", 8080));
            worker_threads.emplace_back([this, i]() {
                workers[i]->run();
            });
        }

        // Give some time for connections to establish
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    void TearDown() override {
        should_stop = true;
        master->stop();
        for (auto& worker : workers) {
            worker->stop();
        }

        if (master_thread.joinable()) {
            master_thread.join();
        }
        for (auto& thread : worker_threads) {
            if (thread.joinable()) {
                thread.join();
            }
        }

        // Restore cout
        std::cout.rdbuf(std::cout.rdbuf());
    }

    void addTasksToMaster(int num_tasks) {
        for (int i = 1; i <= num_tasks; ++i) {
            std::string input = std::to_string(i) + " " + std::to_string(i % 10 + 1) + "\n";
            {
                std::lock_guard<std::mutex> lock(input_mutex);
                input_queue.push(input);
            }
            input_cv.notify_one();
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
        // Add termination signal
        {
            std::lock_guard<std::mutex> lock(input_mutex);
            input_queue.push("-1 0\n");
        }
        input_cv.notify_one();
    }

    std::string getOutput() {
        std::lock_guard<std::mutex> lock(output_mutex);
        std::string output = output_buffer.str();
        output_buffer.str("");
        return output;
    }

    int countCompletedTasks(const std::string& output) {
        int count = 0;
        size_t pos = 0;
        while ((pos = output.find("Task ", pos)) != std::string::npos) {
            if (output.find(" completed", pos) != std::string::npos) {
                count++;
            }
            pos += 5;
        }
        return count;
    }
};

TEST_F(IntegrationTest, TaskDistributionAndCompletion) {
    const int NUM_TASKS = 10;

    // Start a thread to simulate user input
    std::thread input_thread([this, NUM_TASKS]() { 
        addTasksToMaster(NUM_TASKS); 
    });

    // Simulate cin for the master thread
    std::thread cin_simulation([this]() {
        while (!should_stop) {
            std::unique_lock<std::mutex> lock(input_mutex);
            input_cv.wait(lock, [this] { return !input_queue.empty(); });
            if (!input_queue.empty()) {
                std::string input = input_queue.front();
                input_queue.pop();
                std::istringstream iss(input);
                std::cin.rdbuf(iss.rdbuf());
                input_cv.notify_one();
            }
        }
    });

    // Wait for tasks to be processed
    auto start_time = std::chrono::steady_clock::now();
    int completed_count = 0;
    while (completed_count < NUM_TASKS) {
        std::string output = getOutput();
        completed_count += countCompletedTasks(output);

        if (std::chrono::steady_clock::now() - start_time > std::chrono::seconds(30)) {
            FAIL() << "Timeout: Not all tasks completed within the expected time.";
            break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    EXPECT_EQ(completed_count, NUM_TASKS);

    should_stop = true;
    if (input_thread.joinable()) {
        input_thread.join();
    }
    if (cin_simulation.joinable()) {
        cin_simulation.join();
    }
}