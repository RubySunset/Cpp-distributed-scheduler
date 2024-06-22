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
#include <sstream>

#include "task.h"
#include "task_router.h"

class Worker {
public:
    Worker(const std::string& master_address, int master_port);
    ~Worker();
    void run();
    void stop();

private:
    int socket_fd_;
    std::atomic<bool> should_stop_ = false;
    std::thread heartbeat_thread_;
    std::queue<int> task_queue_;
    std::mutex task_queue_mutex_;
    TaskRouter router;

    void sendHeartbeat();
    void executeTask(std::shared_ptr<TaskRequest> request);
};