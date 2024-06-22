#pragma once

#include <unordered_map>
#include <chrono>
#include <thread>
#include <mutex>
#include <atomic>
#include <functional>
#include <iostream>

class HeartbeatMonitor {
public:
    HeartbeatMonitor(std::chrono::seconds timeout);
    ~HeartbeatMonitor();

    void addWorker(int worker_socket);
    void heartbeat(int worker_socket);
    void start();
    void stop();

    void setFailureCallback(std::function<void(int)> callback);

private:
    void monitorLoop();

    std::unordered_map<int, std::chrono::steady_clock::time_point> last_heartbeats_;
    std::chrono::seconds timeout_;
    std::mutex mutex_;
    std::thread monitor_thread_;
    std::atomic<bool> should_stop_;
    std::function<void(int)> failure_callback_;
};