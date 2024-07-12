#include "master.h"

Master::Master(int port) : heartbeat_monitor(std::chrono::seconds(10)) {
    server_fd = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);
    if (server_fd == -1) {
        throw std::runtime_error("Failed to create socket");
    }

    int opt = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))) {
        throw std::runtime_error("Failed to set socket options");
    }

    sockaddr_in address{};
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port);

    if (bind(server_fd, (struct sockaddr*)&address, sizeof(address)) < 0) {
        throw std::runtime_error("Failed to bind to port");
    }

    if (listen(server_fd, 3) < 0) {
        throw std::runtime_error("Failed to listen");
    }

    epoll_fd = epoll_create1(0);
    if (epoll_fd == -1) {
        throw std::runtime_error("Failed to create epoll file descriptor");
    }
    epoll_event event;
    event.events = EPOLLIN | EPOLLET;
    event.data.fd = server_fd;
    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, server_fd, &event) == -1) {
        throw std::runtime_error("Failed to add file descriptor to epoll");
    }

    heartbeat_monitor.setFailureCallback([this](int worker_socket) {
        std::cout << "heartbeat not detected\n";
        handleWorkerFailure(worker_socket);
    });
    heartbeat_monitor.start();

    accept_thread = std::thread(&Master::accept_connections, this);
    load_balancer.set_default_dispatch_callback();
    load_balancer.startDispatchLoop();
}

Master::~Master() {
    if (!stopped) {
        stop();
    }
}

void Master::stop() {
    should_stop = true;
    if (accept_thread.joinable()) {
        accept_thread.join();
    }
    heartbeat_monitor.stop();
    load_balancer.stopDispatchLoop();

    if (server_fd != -1) {
        shutdown(server_fd, SHUT_RDWR);
        close(server_fd);
        server_fd = -1;
    }

    std::unique_lock<std::mutex> lock(worker_sockets_mutex);
    for (int socket : worker_sockets) {
        shutdown(socket, SHUT_RDWR);
        close(socket);
    }

    close(epoll_fd);

    stopped = true;
}

std::optional<std::string> Master::process(
    int priority,
    const std::string &function_name,
    const std::unordered_map<std::string, std::string> &arguments)
{
    int id = ++next_task_id;
    auto request = std::make_shared<TaskRequest>(id, priority, function_name, arguments);
    load_balancer.addTask(request);
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

void Master::handle_worker(int worker_socket) {
    char buffer[4096] = {0};
    int bytes_read = read(worker_socket, buffer, sizeof(buffer));
    if (bytes_read <= 0) {
        std::cout << "worker disconnected\n";
        handleWorkerFailure(worker_socket);
        return;
    }
    std::string message(buffer, bytes_read);
    client_buf[worker_socket] += message;
    while (read_msg(worker_socket)) {}
}

bool Master::read_msg(int fd) {
    if (client_buf[fd].size() < 4) {
        return false;
    }
    uint32_t len;
    std::memcpy(&len, client_buf[fd].data(), 4);
    uint32_t host_len = ntohl(len);
    if (client_buf[fd].size() < 4 + host_len) {
        return false;
    }
    std::string message = client_buf[fd].substr(4, host_len);
    client_buf[fd].erase(0, 4 + host_len);
    handle_client(fd, message);
    return true;
}

void Master::handle_client(int fd, const std::string &message) {
    heartbeat_monitor.heartbeat(fd);
    if (message == "") {
        return;
    }
    auto response = std::make_shared<TaskResponse>(message);
    std::cout << "task " << response->id << " completed by worker " << fd << "\n";
    load_balancer.decLoad(fd);
    std::unique_lock lock(mailbox_mut);
    mailbox[response->id] = response;
    lock.unlock();
    mailbox_cv.notify_all();
}

void Master::handleWorkerFailure(int worker_socket) {
    std::unique_lock<std::mutex> lock(worker_sockets_mutex);
    if (std::find(worker_sockets.begin(), worker_sockets.end(), worker_socket) == worker_sockets.end()) {
        return; // if this failure has already been handled, skip
    }
    worker_sockets.erase(std::remove(worker_sockets.begin(), worker_sockets.end(), worker_socket), worker_sockets.end());
    load_balancer.removeWorker(worker_socket);
    shutdown(worker_socket, SHUT_RDWR);
    close(worker_socket);
    std::cout << "worker " << worker_socket << " failed, removing from system.\n";
}

void Master::accept_connections() {
    epoll_event events[MAX_EVENTS];
    while (!should_stop) {
        int nfds = epoll_wait(epoll_fd, events, MAX_EVENTS, 1000);
        if (nfds == -1) {
            std::cerr << "Epoll wait error" << std::endl;
            continue;
        }
        for (int i = 0; i < nfds; ++i) {
            int fd = events[i].data.fd;
            if (fd == server_fd) {
                int new_socket = accept4(server_fd, NULL, NULL, SOCK_NONBLOCK);
                if (new_socket == -1) {
                    if (errno == EAGAIN || errno == EWOULDBLOCK) {
                        break;
                    } else {
                        std::cerr << "Accept error" << std::endl;
                        break;
                    }
                }
                std::unique_lock<std::mutex> lock(worker_sockets_mutex);
                worker_sockets.push_back(new_socket);
                lock.unlock();
                load_balancer.addWorker(new_socket);
                heartbeat_monitor.addWorker(new_socket);
                epoll_event event;
                event.events = EPOLLIN | EPOLLET;
                event.data.fd = new_socket;
                if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, new_socket, &event) == -1) {
                    throw std::runtime_error("Failed to add file descriptor to epoll");
                }
                std::cout << "added new worker node\n";
            } else {
                handle_worker(fd);
            }
        }
    }
}