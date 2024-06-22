#include "worker.h"

Worker::Worker(const std::string& master_address, int master_port) {
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

    int wait = 1;
    while (connect(socket_fd_, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        std::cout << "connection to master failed, trying again in " << wait << " seconds\n";
        std::this_thread::sleep_for(std::chrono::seconds(wait));
        if (wait < 32) {
            wait *= 2;
        }
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
        int num_sent = send(socket_fd_, heartbeat_msg, strlen(heartbeat_msg), 0);
        if (num_sent <= 0) {
            std::cout << "master disconnected\n";
            should_stop_ = true;
            break;
        }
        std::cout << "sent heartbeat\n";
        std::this_thread::sleep_for(std::chrono::seconds(5));
    }
}

void Worker::executeTask(std::shared_ptr<TaskRequest> request) {
    std::thread t([=](){
        std::cout << "worker executing task " << request->id << '\n';

        // TODO delay to make testing easier
        std::this_thread::sleep_for(std::chrono::seconds(2));

        TaskResponse response;
        if (auto return_val = router.eval_route(*request); return_val.has_value()) {
            response = {
                request->id,
                true,
                *return_val
            };
        } else {
            response = {
                request->id,
                false,
                ""
            };
        }

        std::string response_str = response.to_string();
        
        ssize_t sent = send(socket_fd_, response_str.c_str(), response_str.size(), 0);
        if (sent <= 0) {
            std::cerr << "failed to send completion message for task " << response.id << '\n';
        } else {
            std::cout << "worker sent completion message for task " << response.id << '\n';
        }
    });
    t.detach();
}

void Worker::run() {
    heartbeat_thread_ = std::thread(&Worker::sendHeartbeat, this);

    while (!should_stop_) {
        char buffer[1024] = {0};
        int valread = read(socket_fd_, buffer, sizeof(buffer) - 1);
        if (valread <= 0) {
            std::cout << "master disconnected\n";
            break;
        }

        buffer[valread] = '\0';  // Ensure null-termination
        std::istringstream iss(buffer);
        std::string line;

        while (std::getline(iss, line)) {
            auto request = std::make_shared<TaskRequest>(line);
            executeTask(request);
        }
    }
}