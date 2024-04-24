#pragma once

#include <queue>
#include <mutex>
#include <condition_variable>
#include <memory>
#include <vector>
#include <atomic>
#include <iostream>
#include <thread>
#include <stdexcept>
#include <cstring>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <csignal>

#include "task.h"

class Master {
public:
    Master(int port);
    ~Master();
    void run();
    void stop();

private:
    void accept_connections();
    void handle_worker(int worker_socket);

    int server_fd_;
    std::queue<std::shared_ptr<Task>> task_queue_;
    std::mutex task_queue_mutex_;
    std::condition_variable task_available_;
    std::atomic<bool> should_stop_;
    std::vector<int> worker_sockets_;
    std::mutex worker_sockets_mutex_;
    std::thread accept_thread_;
};