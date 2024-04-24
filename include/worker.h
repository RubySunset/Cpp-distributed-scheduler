#pragma once

#include <string>
#include <atomic>
#include <iostream>
#include <thread>
#include <chrono>
#include <cstring>
#include <stdexcept>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <csignal>

class Worker {
public:
    Worker(const std::string& master_address, int master_port);
    ~Worker();
    void run();
    void stop();

private:
    int socket_fd_;
    std::atomic<bool> should_stop_;
};