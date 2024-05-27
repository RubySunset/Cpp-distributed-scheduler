#pragma once

#include <string>
#include <atomic>
#include <thread>
#include <chrono>
#include <cstring>
#include <stdexcept>
#include <iostream>
#include <random>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <queue>
#include <mutex>

class Worker {
public:
    Worker(const std::string& master_address, int master_port);
    ~Worker();
    void run();
    void stop();

private:
    int socket_fd_;
    std::atomic<bool> should_stop_;
    std::atomic<int> current_load_;
    std::mt19937 rng_;
    std::uniform_int_distribution<> load_dist_;
    std::thread heartbeat_thread_;
    std::queue<int> task_queue_;
    std::mutex task_queue_mutex_;

    void sendHeartbeat();
    void updateLoad();
    void reportLoad();
    void executeTask(int task_id);
};