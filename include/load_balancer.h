#pragma once

#include <vector>
#include <unordered_map>
#include <queue>
#include <memory>
#include <mutex>

#include "task.h"

class LoadBalancer {
public:
    void addWorker(int worker_socket);
    void removeWorker(int worker_socket);
    void updateWorkerLoad(int worker_socket, int load);
    int getAvailableWorker();
    void assignTask(int worker_socket, std::shared_ptr<Task> task);
    std::shared_ptr<Task> getNextTask();
    void addTask(std::shared_ptr<Task> task);
    bool hasTasks();

private:
    std::unordered_map<int, int> worker_loads_;
    std::queue<std::shared_ptr<Task>> task_queue_;
    std::mutex mutex_;
};