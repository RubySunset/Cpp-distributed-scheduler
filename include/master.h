#pragma once

#include <queue>
#include <mutex>
#include <condition_variable>
#include <memory>
#include <vector>
#include <atomic>
#include <thread>
#include <iostream>
#include <stdexcept>
#include <cstring>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <csignal>
#include <limits>
#include <sstream>

#include "task.h"
#include "load_balancer.h"
#include "heartbeat_monitor.h"

class Master {
public:
    Master(int port);
    ~Master();
    void run();
    void stop();

private:
    void accept_connections();
    void handle_worker(int worker_socket);
    void assignTaskToWorker();
    void handleWorkerFailure(int worker_socket);

    int server_fd_;
    std::atomic<bool> should_stop_;
    std::vector<int> worker_sockets_;
    std::mutex worker_sockets_mutex_;
    std::thread accept_thread_;
    LoadBalancer load_balancer_;
    HeartbeatMonitor heartbeat_monitor_;
};