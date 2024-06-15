#pragma once

#include <vector>
#include <unordered_map>
#include <queue>
#include <memory>
#include <mutex>
#include <condition_variable>
#include <sys/socket.h>
#include <netinet/in.h>
#include <iostream>
#include <thread>
#include <algorithm>

#include "task.h"

struct TaskComp {
    bool operator()(const std::shared_ptr<TaskRequest> t1, const std::shared_ptr<TaskRequest> t2) const {
        return t1->priority < t2->priority;
    }
};

class LoadBalancer {
public:
    void addWorker(int worker_socket);
    void removeWorker(int worker_socket);
    void addTask(std::shared_ptr<TaskRequest> task);
    void startDispatchLoop();
    void stopDispatchLoop();
    void incLoad(int worker_socket);
    void decLoad(int worker_socket);
private:
    bool canDispatch();
    void dispatchLoop();
    std::unordered_map<int, int> worker_loads_;
    std::priority_queue<std::shared_ptr<TaskRequest>, std::vector<std::shared_ptr<TaskRequest>>, TaskComp> tasks;
    std::mutex mutex_;
    std::condition_variable cv;
    bool stop = false;
    std::thread dispatch_thread;
};