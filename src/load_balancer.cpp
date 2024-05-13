#include <algorithm>

#include "load_balancer.h"

void LoadBalancer::addWorker(int worker_socket) {
    std::lock_guard<std::mutex> lock(mutex_);
    worker_loads_[worker_socket] = 0;
}

void LoadBalancer::removeWorker(int worker_socket) {
    std::lock_guard<std::mutex> lock(mutex_);
    worker_loads_.erase(worker_socket);
}

void LoadBalancer::updateWorkerLoad(int worker_socket, int load) {
    std::lock_guard<std::mutex> lock(mutex_);
    worker_loads_[worker_socket] = load;
}

int LoadBalancer::getAvailableWorker() {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = std::min_element(worker_loads_.begin(), worker_loads_.end(),
        [](const auto& a, const auto& b) { return a.second < b.second; });
    
    if (it != worker_loads_.end() && it->second < 10) { // Assuming max load is 10
        return it->first;
    }
    return -1;
}

void LoadBalancer::assignTask(int worker_socket, std::shared_ptr<Task> task) {
    std::lock_guard<std::mutex> lock(mutex_);
    worker_loads_[worker_socket]++;
}

void LoadBalancer::addTask(std::shared_ptr<Task> task) {
    std::lock_guard<std::mutex> lock(mutex_);
    task_queue_.push(task);
}

std::shared_ptr<Task> LoadBalancer::getNextTask() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (task_queue_.empty()) {
        return nullptr;
    }
    auto task = task_queue_.front();
    task_queue_.pop();
    return task;
}

bool LoadBalancer::hasTasks() {
    std::lock_guard<std::mutex> lock(mutex_);
    return !task_queue_.empty();
}