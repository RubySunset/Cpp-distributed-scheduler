#include <algorithm>

#include "load_balancer.h"

void LoadBalancer::addWorker(int worker_socket) {
    std::unique_lock<std::mutex> lock(mutex_);
    worker_loads_[worker_socket] = 0;
    lock.unlock();
    cv.notify_one();
}

void LoadBalancer::removeWorker(int worker_socket) {
    std::unique_lock<std::mutex> lock(mutex_);
    worker_loads_.erase(worker_socket);
}

void LoadBalancer::addTask(std::shared_ptr<Task> task) {
    std::unique_lock<std::mutex> lock(mutex_);
    tasks.push(task);
    lock.unlock();
    cv.notify_one();
}

void LoadBalancer::dispatchLoop() {
    while (!stop) {
        std::unique_lock<std::mutex> lock(mutex_);
        if (!canDispatch()) {
            cv.wait(lock, [this](){ return canDispatch(); });
        }
        if (stop) {
            break;
        }
        std::shared_ptr<Task> task = tasks.top();
        tasks.pop();
        int worker = std::min_element(
            worker_loads_.begin(),
            worker_loads_.end(),
            [](const std::pair<int, int> &p1, const std::pair<int, int> &p2) {
                return p1.second < p2.second;
            }
        )->first;
        lock.unlock();
        std::string task_msg = std::to_string(task->getId()) + "\n";
        ssize_t sent = send(worker, task_msg.c_str(), task_msg.length(), 0);
        if (sent > 0) {
            incLoad(worker);
            std::cout << "Sent task " << task->getId() << " (priority " << task->getPriority() << ") to worker " << worker << std::endl;
        } else {
            std::cerr << "Failed to send task to worker. Keeping in queue." << std::endl;
            addTask(task);
        }
    }
}

void LoadBalancer::startDispatchLoop() {
    dispatch_thread = std::thread(&LoadBalancer::dispatchLoop, this);
}

void LoadBalancer::stopDispatchLoop() {
    stop = true;
    cv.notify_one();
    dispatch_thread.join();
}

void LoadBalancer::incLoad(int worker_socket) {
    std::unique_lock<std::mutex> lock(mutex_);
    worker_loads_[worker_socket]++;
}

void LoadBalancer::decLoad(int worker_socket) {
    std::unique_lock<std::mutex> lock(mutex_);
    worker_loads_[worker_socket]--;
    lock.unlock();
    cv.notify_one();
}

bool LoadBalancer::canDispatch() {
    auto min_worker = std::min_element(
        worker_loads_.begin(),
        worker_loads_.end(),
        [](const std::pair<int, int> &a, const std::pair<int, int> &b) {
            return a.second < b.second;
        }
    );
    return stop || (!tasks.empty() && min_worker != worker_loads_.end() && min_worker->second <= 10);
    // need at least 1 task + at least one worker with load <= 10)
}