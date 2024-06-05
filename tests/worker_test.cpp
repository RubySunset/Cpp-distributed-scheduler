#include <gtest/gtest.h>
#include "worker.h"
#include <thread>

class WorkerTest : public ::testing::Test {
protected:
    int server_socket;
    std::thread server_thread;

    void SetUp() override {
        server_socket = socket(AF_INET, SOCK_STREAM, 0);
        sockaddr_in address{};
        address.sin_family = AF_INET;
        address.sin_addr.s_addr = INADDR_ANY;
        address.sin_port = htons(8080);
        bind(server_socket, (struct sockaddr*)&address, sizeof(address));
        listen(server_socket, 3);

        server_thread = std::thread([this]() {
            int client_socket = accept(server_socket, nullptr, nullptr);
            char buffer[1024] = {0};
            read(client_socket, buffer, 1024);
            EXPECT_STREQ(buffer, "READY\n");
            const char* task_msg = "1\n";
            send(client_socket, task_msg, strlen(task_msg), 0);
            memset(buffer, 0, sizeof(buffer));
            read(client_socket, buffer, 1024);
            EXPECT_STREQ(buffer, "DONE 1\n");
            close(client_socket);
        });
    }

    void TearDown() override {
        close(server_socket);
        if (server_thread.joinable()) {
            server_thread.join();
        }
    }
};

TEST_F(WorkerTest, ExecuteTask) {
    Worker worker("127.0.0.1", 8080);
    std::thread worker_thread([&worker]() {
        worker.run();
    });

    std::this_thread::sleep_for(std::chrono::seconds(2));
    worker.stop();
    worker_thread.join();
}