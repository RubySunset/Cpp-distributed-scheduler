#include "heartbeat_monitor.h"

HeartbeatMonitor::HeartbeatMonitor(std::chrono::seconds timeout)
    : timeout_(timeout), should_stop_(false) {}

HeartbeatMonitor::~HeartbeatMonitor() {
    stop();
}

void HeartbeatMonitor::addWorker(int worker_socket) {
    std::unique_lock<std::mutex> lock(mutex_);
    last_heartbeats_[worker_socket] = std::chrono::steady_clock::now();
}

void HeartbeatMonitor::heartbeat(int worker_socket) {
    // std::cout << "received heartbeat from worker " << worker_socket << '\n';
    std::unique_lock<std::mutex> lock(mutex_);
    last_heartbeats_[worker_socket] = std::chrono::steady_clock::now();
}

void HeartbeatMonitor::start() {
    monitor_thread_ = std::thread(&HeartbeatMonitor::monitorLoop, this);
}

void HeartbeatMonitor::stop() {
    should_stop_ = true;
    monitor_thread_.join();
}

void HeartbeatMonitor::setFailureCallback(std::function<void(int)> callback) {
    failure_callback_ = std::move(callback);
}

void HeartbeatMonitor::monitorLoop() {
    while (!should_stop_) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
        
        std::unique_lock<std::mutex> lock(mutex_);
        auto now = std::chrono::steady_clock::now();
        
        for (auto it = last_heartbeats_.begin(); it != last_heartbeats_.end();) {
            if (now - it->second > timeout_) {
                if (failure_callback_) {
                    failure_callback_(it->first);
                }
                it = last_heartbeats_.erase(it);
            } else {
                ++it;
            }
        }
    }
}