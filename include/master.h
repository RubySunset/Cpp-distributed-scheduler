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
#include <optional>
#include <sys/epoll.h>

#include "task.h"
#include "load_balancer.h"
#include "heartbeat_monitor.h"

#define MAX_EVENTS 1024

class Master {
public:
    Master(int port);
    ~Master();
    // void run();
    std::optional<std::string> process(int priority, const std::string &function_name, const std::unordered_map<std::string, std::string> &arguments);
    void stop();

private:
    void accept_connections();
    void handle_worker(int worker_socket);
    bool read_msg(int fd);
    void handle_client(int fd, const std::string &message);
    void handleWorkerFailure(int worker_socket);

    std::atomic<int> next_task_id = 0;
    int server_fd, epoll_fd;
    std::atomic<bool> should_stop = false;
    std::vector<int> worker_sockets;
    std::mutex worker_sockets_mutex;
    std::thread accept_thread;
    std::thread dispatch_thread;
    LoadBalancer load_balancer;
    HeartbeatMonitor heartbeat_monitor;
    std::unordered_map<int, std::shared_ptr<TaskResponse>> mailbox;
    std::mutex mailbox_mut;
    std::condition_variable mailbox_cv;
    std::unordered_map<int, std::string> client_buf;
};