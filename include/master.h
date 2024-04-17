#pragma once

#include <queue>
#include <mutex>
#include <condition_variable>
#include <memory>
#include <iostream>
#include <thread>
#include <vector>
#include <cstring>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>

#include "task.h"

class Master {
public:
    Master(int port);
    void run();

private:
    void accept_connections();
    void handle_worker(int worker_socket);

    int server_fd_;
    std::queue<std::shared_ptr<Task>> task_queue_;
    std::mutex task_queue_mutex_;
    std::condition_variable task_available_;
    bool should_stop_;
};