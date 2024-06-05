#include <gtest/gtest.h>
#include <thread>
#include <arpa/inet.h>

#include "master.h"

class MasterTest : public ::testing::Test {
protected:
    Master* master;
    std::thread master_thread;

    void SetUp() override {
        master = new Master(8080);
        master_thread = std::thread([this]() {
            master->run();
        });
    }

    void TearDown() override {
        master->stop();
        if (master_thread.joinable()) {
            master_thread.join();
        }
        delete master;
    }
};

TEST_F(MasterTest, AcceptWorkerAndAssignTask) {
    int worker_socket = socket(AF_INET, SOCK_STREAM, 0);
    sockaddr_in address{};
    address.sin_family = AF_INET;
    address.sin_port = htons(8080);
    inet_pton(AF_INET, "127.0.0.1", &address.sin_addr);

    ASSERT_NE(connect(worker_socket, (struct sockaddr*)&address, sizeof(address)), -1);

    const char* ready_msg = "READY\n";
    send(worker_socket, ready_msg, strlen(ready_msg), 0);

    char buffer[1024] = {0};
    read(worker_socket, buffer, 1024);
    EXPECT_NE(strcmp(buffer, "NO_TASK\n"), 0);

    const char* done_msg = "DONE 1\n";
    send(worker_socket, done_msg, strlen(done_msg), 0);

    close(worker_socket);
}