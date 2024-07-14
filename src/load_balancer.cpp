#include "load_balancer.h"

void LoadBalancer::set_dispatch_callback(std::function<void(int, std::shared_ptr<TaskRequest>)> callback) {
    dispatch_callback = std::move(callback);
}

void LoadBalancer::set_default_dispatch_callback() {
    dispatch_callback = [this](int worker, std::shared_ptr<TaskRequest> task) {
        std::string task_msg = task->to_string();
        uint32_t len = htonl(static_cast<uint32_t>(task_msg.size()));
        std::string len_msg(reinterpret_cast<char*>(&len), 4);
        std::string total_msg = len_msg + task_msg;
        ssize_t sent = send(worker, total_msg.c_str(), total_msg.size(), 0);
        if (sent > 0) {
            incLoad(worker);
            std::unique_lock<std::mutex> lock(mutex_);
            dispatched_tasks[worker].push_back(task);
            std::cout << "sent task " << task->id << " to worker " << worker << '\n';
        } else {
            std::cerr << "failed to send task to worker, keeping in queue\n";
            addTask(task);
        }
    };
}

void LoadBalancer::addWorker(int worker_socket) {
    std::unique_lock<std::mutex> lock(mutex_);
    worker_loads_[worker_socket] = 0;
    lock.unlock();
    cv.notify_one();
}

void LoadBalancer::removeWorker(int worker_socket) {
    std::unique_lock<std::mutex> lock(mutex_);
    worker_loads_.erase(worker_socket);
    for (std::shared_ptr<TaskRequest> task : dispatched_tasks[worker_socket]) {
        tasks.push(task);
    }
    dispatched_tasks.erase(worker_socket);
}

void LoadBalancer::addTask(std::shared_ptr<TaskRequest> task) {
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
        std::shared_ptr<TaskRequest> task = tasks.top();
        tasks.pop();
        int worker = std::min_element(
            worker_loads_.begin(),
            worker_loads_.end(),
            [](const std::pair<int, int> &p1, const std::pair<int, int> &p2) {
                return p1.second < p2.second;
            }
        )->first;
        lock.unlock();
        dispatch_callback(worker, task);
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

void LoadBalancer::complete_task(int worker, int task_id) {
    std::unique_lock<std::mutex> lock(mutex_);
    for (auto it = dispatched_tasks[worker].begin(); it != dispatched_tasks[worker].end(); ++it) {
        if ((*it)->id == task_id) {
            dispatched_tasks[worker].erase(it);
            break;
        }
    }
}