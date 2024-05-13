#include "worker.h"

Worker::Worker(const std::string& master_address, int master_port)
    : should_stop_(false), current_load_(0), rng_(std::random_device{}()), load_dist_(1, 10) {
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
    if (socket_fd_ != -1) {
        shutdown(socket_fd_, SHUT_RDWR);
        close(socket_fd_);
        socket_fd_ = -1;
    }
}

void Worker::updateLoad() {
    current_load_ = load_dist_(rng_);
}

void Worker::reportLoad() {
    std::string load_msg = "LOAD " + std::to_string(current_load_) + "\n";
    send(socket_fd_, load_msg.c_str(), load_msg.length(), 0);
}

void Worker::run() {
    while (!should_stop_) {
        updateLoad();
        reportLoad();

        const char* ready_msg = "READY\n";
        send(socket_fd_, ready_msg, strlen(ready_msg), 0);

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
        std::cout << "Worker executing task " << task_id << std::endl;
        std::this_thread::sleep_for(std::chrono::milliseconds(load_dist_(rng_) * 100));

        std::string done_msg = "DONE " + std::to_string(task_id) + "\n";
        send(socket_fd_, done_msg.c_str(), done_msg.length(), 0);
    }
}