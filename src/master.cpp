#include "master.h"

Master::Master(int port) : heartbeat_monitor_(std::chrono::seconds(10)) {
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

    heartbeat_monitor_.setFailureCallback([this](int worker_socket) {
        handleWorkerFailure(worker_socket);
    });
    heartbeat_monitor_.start();
}

Master::~Master() {
    stop();
}

void Master::stop() {
    should_stop_ = true;
    if (server_fd_ != -1) {
        shutdown(server_fd_, SHUT_RDWR);
        close(server_fd_);
        server_fd_ = -1;
    }

    std::unique_lock<std::mutex> lock(worker_sockets_mutex_);
    for (int socket : worker_sockets_) {
        shutdown(socket, SHUT_RDWR);
        close(socket);
    }

    accept_thread_.join();
    heartbeat_monitor_.stop();
    load_balancer_.stopDispatchLoop();
}

void Master::run() {
    accept_thread_ = std::thread(&Master::accept_connections, this);
    load_balancer_.startDispatchLoop();

    while (!should_stop_) {
        int priority;
        std::cout << "Enter priority (1-10), or -1 to quit: ";
        if (std::cin >> priority) {
            if (priority == -1 || should_stop_) break;

            try {
                auto new_task = std::make_shared<Task>(++next_task_id, []() {}, priority);
                load_balancer_.addTask(new_task);
                std::cout << "Task " << next_task_id << " added with priority " << priority << std::endl;
            } catch (const std::invalid_argument& e) {
                std::cout << "Error: " << e.what() << std::endl;
            }
        } else {
            if (std::cin.eof() || should_stop_) {
                break;
            }
            std::cin.clear();
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            std::cout << "Invalid input. Please enter priority (1-10), or -1 to quit." << std::endl;
        }
    }
}

void Master::handle_worker(int worker_socket) {
    char buffer[1024] = {0};
    while (!should_stop_) {
        int valread = read(worker_socket, buffer, sizeof(buffer) - 1);
        if (valread <= 0) {
            std::cout << "Worker disconnected" << std::endl;
            break;
        }

        buffer[valread] = '\0';  // Ensure null-termination
        std::istringstream iss(buffer);
        std::string line;

        while (std::getline(iss, line)) {
            if (line.substr(0, 5) == "DONE ") {
                int task_id = std::stoi(line.substr(5));
                std::cout << "Task " << task_id << " completed by worker " << worker_socket << std::endl;
                load_balancer_.decLoad(worker_socket);
                heartbeat_monitor_.heartbeat(worker_socket);
            } else if (line == "HEARTBEAT") {
                heartbeat_monitor_.heartbeat(worker_socket);
            } else {
                std::cout << "Unknown message from worker " << worker_socket << ": " << line << std::endl;
            }
        }

        memset(buffer, 0, sizeof(buffer));
    }

    std::unique_lock<std::mutex> lock(worker_sockets_mutex_);
    std::remove(worker_sockets_.begin(), worker_sockets_.end(), worker_socket);
    load_balancer_.removeWorker(worker_socket);
    heartbeat_monitor_.removeWorker(worker_socket);
    close(worker_socket);
}

void Master::handleWorkerFailure(int worker_socket) {
    std::cout << "Worker " << worker_socket << " failed. Removing from system." << std::endl;
    
    std::unique_lock<std::mutex> lock(worker_sockets_mutex_);
    std::remove(worker_sockets_.begin(), worker_sockets_.end(), worker_socket);
    load_balancer_.removeWorker(worker_socket);
    heartbeat_monitor_.removeWorker(worker_socket);
    close(worker_socket);
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
            std::unique_lock<std::mutex> lock(worker_sockets_mutex_);
            worker_sockets_.push_back(new_socket);
        }
        load_balancer_.addWorker(new_socket);
        heartbeat_monitor_.addWorker(new_socket);

        std::thread worker_thread(&Master::handle_worker, this, new_socket);
        worker_thread.detach();

        std::cout << "Added new worker node" << std::endl;
    }
}