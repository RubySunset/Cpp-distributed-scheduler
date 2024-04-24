#include "master.h"

Master::Master(int port) : should_stop_(false), server_fd_(-1) {
    server_fd_ = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd_ == -1) {
        throw std::runtime_error("Failed to create socket");
    }

    int opt = 1;
    if (setsockopt(server_fd_, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))) {
        throw std::runtime_error("Failed to set socket options");
    }

    sockaddr_in address{};
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port);

    if (bind(server_fd_, (struct sockaddr*)&address, sizeof(address)) < 0) {
        throw std::runtime_error("Failed to bind to port");
    }

    if (listen(server_fd_, 3) < 0) {
        throw std::runtime_error("Failed to listen");
    }
}

Master::~Master() {
    stop();
    if (accept_thread_.joinable()) {
        accept_thread_.join();
    }
}

void Master::stop() {
    should_stop_ = true;
    if (server_fd_ != -1) {
        shutdown(server_fd_, SHUT_RDWR);
        close(server_fd_);
        server_fd_ = -1;
    }

    std::lock_guard<std::mutex> lock(worker_sockets_mutex_);
    for (int socket : worker_sockets_) {
        shutdown(socket, SHUT_RDWR);
        close(socket);
    }
    worker_sockets_.clear();
}

void Master::run() {
    accept_thread_ = std::thread(&Master::accept_connections, this);

    while (!should_stop_) {
        int task_id;
        std::cout << "Enter task ID (or -1 to quit): ";
        if (std::cin >> task_id) {
            if (task_id == -1 || should_stop_) break;

            {
                std::lock_guard<std::mutex> lock(task_queue_mutex_);
                task_queue_.push(std::make_shared<Task>(task_id, []() {}));
            }
            task_available_.notify_one();
        } else {
            // Handle invalid input or EOF
            if (std::cin.eof() || should_stop_) {
                break;
            }
            std::cin.clear();
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            std::cout << "Invalid input. Please enter a number or -1 to quit." << std::endl;
        }
    }

    stop();
    if (accept_thread_.joinable()) {
        accept_thread_.join();
    }
}

void Master::accept_connections() {
    while (!should_stop_) {
        sockaddr_in address{};
        int addrlen = sizeof(address);
        int new_socket = accept(server_fd_, (struct sockaddr*)&address, (socklen_t*)&addrlen);
        if (new_socket < 0) {
            if (!should_stop_) {
                std::cerr << "Failed to accept connection" << std::endl;
            }
            continue;
        }

        {
            std::lock_guard<std::mutex> lock(worker_sockets_mutex_);
            worker_sockets_.push_back(new_socket);
        }

        std::thread worker_thread(&Master::handle_worker, this, new_socket);
        worker_thread.detach();
    }
}

void Master::handle_worker(int worker_socket) {
    char buffer[1024] = {0};
    while (!should_stop_) {
        int valread = read(worker_socket, buffer, 1024);
        if (valread <= 0) {
            std::cout << "Worker disconnected" << std::endl;
            break;
        }

        std::string request(buffer);
        memset(buffer, 0, sizeof(buffer));

        if (request == "READY\n") {
            std::unique_lock<std::mutex> lock(task_queue_mutex_);
            if (task_queue_.empty()) {
                const char* no_task_msg = "NO_TASK\n";
                send(worker_socket, no_task_msg, strlen(no_task_msg), 0);
            } else {
                auto task = task_queue_.front();
                task_queue_.pop();
                std::string task_msg = std::to_string(task->getId()) + "\n";
                send(worker_socket, task_msg.c_str(), task_msg.length(), 0);
            }
        } else if (request.substr(0, 5) == "DONE ") {
            int task_id = std::stoi(request.substr(5));
            std::cout << "Task " << task_id << " completed" << std::endl;
        }
    }

    std::lock_guard<std::mutex> lock(worker_sockets_mutex_);
    worker_sockets_.erase(std::remove(worker_sockets_.begin(), worker_sockets_.end(), worker_socket), worker_sockets_.end());
    close(worker_socket);
}