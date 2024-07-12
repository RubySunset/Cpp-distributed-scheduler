#include "worker.h"

Worker::Worker(const std::string& master_address, int master_port) {
    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == -1) {
        throw std::runtime_error("Failed to create socket");
    }

    sockaddr_in server_addr{};
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(master_port);
    if (inet_pton(AF_INET, master_address.c_str(), &server_addr.sin_addr) <= 0) {
        throw std::runtime_error("Invalid address / Address not supported");
    }

    int wait = 1;
    while (connect(sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        std::cout << "connection to master failed, trying again in " << wait << " seconds\n";
        std::this_thread::sleep_for(std::chrono::seconds(wait));
        if (wait < 32) {
            wait *= 2;
        }
    }
}

Worker::~Worker() {
    if (!stopped) {
        stop();
    }
}

void Worker::stop() {
    should_stop_ = true;
    if (heartbeat_thread_.joinable()) {
        heartbeat_thread_.join();
    }
    if (sock != -1) {
        shutdown(sock, SHUT_RDWR);
        close(sock);
        sock = -1;
    }
    stopped = true;
}

void Worker::sendHeartbeat() {
    while (!should_stop_) {
        const char* heartbeat_msg = "\0\0\0\0";
        int num_sent = send(sock, heartbeat_msg, 4, 0);
        if (num_sent <= 0) {
            std::cout << "master disconnected\n";
            should_stop_ = true;
            break;
        }
        std::cout << "sent heartbeat\n";
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
}

void Worker::executeTask(std::shared_ptr<TaskRequest> request) {
    std::thread t([=](){
        std::cout << "worker executing task " << request->id << '\n';

        // TODO delay to make testing easier
        // std::this_thread::sleep_for(std::chrono::seconds(2));

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
        uint32_t len = htonl(static_cast<uint32_t>(response_str.size()));
        std::string len_str(reinterpret_cast<char*>(&len), 4);
        std::string total_str = len_str + response_str;
        
        ssize_t sent = send(sock, total_str.c_str(), total_str.size(), 0);
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
        char buffer[4096];
        int bytes_read = read(sock, buffer, sizeof(buffer));
        if (bytes_read <= 0) {
            std::cout << "master disconnected\n";
            should_stop_ = true;
            break;
        }
        std::string message(buffer, bytes_read);
        server_buf += message;
        while (read_msg()) {}
    }
}

bool Worker::read_msg() {
    if (server_buf.size() < 4) {
        return false;
    }
    uint32_t len;
    std::memcpy(&len, server_buf.data(), 4);
    uint32_t host_len = ntohl(len);
    if (server_buf.size() < 4 + host_len) {
        return false;
    }
    std::string message = server_buf.substr(4, host_len);
    server_buf.erase(0, 4 + host_len);
    auto task_ptr = std::make_shared<TaskRequest>(message);
    executeTask(task_ptr);
    return true;
}