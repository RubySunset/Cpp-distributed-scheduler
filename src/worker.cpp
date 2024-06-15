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
        std::cout << "sent heartbeat\n";
        std::this_thread::sleep_for(std::chrono::seconds(5));
    }
}

void Worker::executeTask(std::shared_ptr<TaskRequest> request) {
    std::thread t([=](){
        std::cout << "worker executing task " << request->id << '\n';

        // TODO replace this with some actual logic
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));

        TaskResponse response{
            request->id,
            true,
            "test"
        };
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