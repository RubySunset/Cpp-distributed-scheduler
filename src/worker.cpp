#include "worker.h"

Worker::Worker(const std::string& master_address, int master_port)
    : should_stop_(false), rng_(std::random_device{}()), load_dist_(1, 10) {
    socket_fd_ = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_fd_ == -1) {
        throw std::runtime_error("Failed to create socket");
    }

    sockaddr_in server_addr{};
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(master_port);
    if (inet_pton(AF_INET, master_address.c_str(), &server_addr.sin_addr) <= 0) {
        throw std::runtime_error("Invalid address / Address not supported");
    }

    if (connect(socket_fd_, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        throw std::runtime_error("Connection failed");
    }
}

Worker::~Worker() {
    stop();
}

void Worker::stop() {
    should_stop_ = true;
    heartbeat_thread_.join();
    if (socket_fd_ != -1) {
        shutdown(socket_fd_, SHUT_RDWR);
        close(socket_fd_);
        socket_fd_ = -1;
    }
}

void Worker::sendHeartbeat() {
    while (!should_stop_) {
        const char* heartbeat_msg = "HEARTBEAT\n";
        send(socket_fd_, heartbeat_msg, strlen(heartbeat_msg), 0);
        std::this_thread::sleep_for(std::chrono::seconds(5));
        std::cout << "send heartbeat" << std::endl;
    }
}

void Worker::executeTask(int task_id) {
    // Simulate task execution
    std::cout << "Worker executing task " << task_id << std::endl;
    std::this_thread::sleep_for(std::chrono::milliseconds(load_dist_(rng_) * 100));
    
    // Report task completion
    std::string done_msg = "DONE " + std::to_string(task_id) + "\n";
    ssize_t sent = send(socket_fd_, done_msg.c_str(), done_msg.length(), 0);
    if (sent <= 0) {
        std::cerr << "Failed to send completion message for task " << task_id << std::endl;
    } else {
        std::cout << "Worker sent completion message for task " << task_id << std::endl;
    }
}

void Worker::run() {
    heartbeat_thread_ = std::thread(&Worker::sendHeartbeat, this);

    while (!should_stop_) {
        char buffer[1024] = {0};
        int valread = read(socket_fd_, buffer, 1024);
        if (valread <= 0) {
            std::cout << "Master disconnected" << std::endl;
            break;
        }

        std::string task_str(buffer);
        if (task_str == "NO_TASK\n") {
            std::this_thread::sleep_for(std::chrono::seconds(1));
            continue;
        }

        int task_id = std::stoi(task_str);
        executeTask(task_id);  // Execute the task immediately
    }
}