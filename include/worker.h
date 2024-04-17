#pragma once

#include <string>
#include <iostream>
#include <thread>
#include <chrono>
#include <cstring>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>

class Worker {
public:
    Worker(const std::string& master_address, int master_port);
    void run();

private:
    int socket_fd_;
};