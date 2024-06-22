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
        std::cout << "heartbeat not detected\n";
        handleWorkerFailure(worker_socket);
    });
    heartbeat_monitor_.start();

    accept_thread_ = std::thread(&Master::accept_connections, this);
    load_balancer_.startDispatchLoop();
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

std::optional<std::string> Master::process(
    int priority,
    const std::string &function_name,
    const std::unordered_map<std::string, std::string> &arguments)
{
    int id = ++next_task_id;
    auto request = std::make_shared<TaskRequest>(id, priority, function_name, arguments);
    load_balancer_.addTask(request);
    std::unique_lock<std::mutex> lock(mailbox_mut);
    mailbox_cv.wait(lock, [this, id](){
        return mailbox.find(id) != mailbox.end();
    });
    std::shared_ptr<TaskResponse> response = mailbox[id];
    mailbox.erase(id);
    if (response->success) {
        return response->return_value;
    } else {
        return {};
    }
}

// void Master::run() {
//     while (!should_stop_) {
//         int priority;
//         std::cout << "Enter priority (1-10), or -1 to quit: ";
//         if (std::cin >> priority) {
//             if (priority == -1 || should_stop_) break;

//             try {
//                 auto new_task = std::make_shared<Task>(++next_task_id, []() {}, priority);
//                 load_balancer_.addTask(new_task);
//                 std::cout << "Task " << next_task_id << " added with priority " << priority << std::endl;
//             } catch (const std::invalid_argument& e) {
//                 std::cout << "Error: " << e.what() << std::endl;
//             }
//         } else {
//             if (std::cin.eof() || should_stop_) {
//                 break;
//             }
//             std::cin.clear();
//             std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
//             std::cout << "Invalid input. Please enter priority (1-10), or -1 to quit." << std::endl;
//         }
//     }
// }

void Master::handle_worker(int worker_socket) {
    char buffer[1024] = {0};
    while (!should_stop_) {
        int valread = read(worker_socket, buffer, sizeof(buffer) - 1);
        if (valread <= 0) {
            std::cout << "worker disconnected\n";
            break;
        }

        buffer[valread] = '\0';  // Ensure null-termination
        std::istringstream iss(buffer);
        std::string line;

        while (std::getline(iss, line)) {
            heartbeat_monitor_.heartbeat(worker_socket);
            if (line != "HEARTBEAT") {
                auto response = std::make_shared<TaskResponse>(line);
                std::cout << "task " << response->id << " completed by worker " << worker_socket << "\n";
                load_balancer_.decLoad(worker_socket);
                std::unique_lock lock(mailbox_mut);
                mailbox[response->id] = response;
                lock.unlock();
                mailbox_cv.notify_all();
            }
        }

        memset(buffer, 0, sizeof(buffer));
    }

    handleWorkerFailure(worker_socket);
}

void Master::handleWorkerFailure(int worker_socket) {
    std::unique_lock<std::mutex> lock(worker_sockets_mutex_);
    if (std::find(worker_sockets_.begin(), worker_sockets_.end(), worker_socket) == worker_sockets_.end()) {
        return; // if this failure has already been handled, skip
    }
    worker_sockets_.erase(std::remove(worker_sockets_.begin(), worker_sockets_.end(), worker_socket), worker_sockets_.end());
    load_balancer_.removeWorker(worker_socket);
    shutdown(worker_socket, SHUT_RDWR);
    close(worker_socket);
    std::cout << "worker " << worker_socket << " failed, removing from system.\n";
}

void Master::accept_connections() {
    while (!should_stop_) {
        sockaddr_in address{};
        int addrlen = sizeof(address);
        int new_socket = accept(server_fd_, (struct sockaddr*)&address, (socklen_t*)&addrlen);
        if (new_socket < 0) {
            if (!should_stop_) {
                std::cerr << "failed to accept connection\n";
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

        std::cout << "added new worker node\n";
    }
}