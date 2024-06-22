#include <gtest/gtest.h>
#include "heartbeat_monitor.h"

class HeartbeatMonitorTest : public ::testing::Test {
protected:
    HeartbeatMonitor hm{std::chrono::seconds(1)};
    void SetUp() override {
        hm.addWorker(1);
        hm.addWorker(2);
        hm.addWorker(3);
    }
    void TearDown() override {}
};

TEST_F(HeartbeatMonitorTest, HeartbeatAndFailure) {
    bool worker_failed = false;
    int failed_worker_id = -1;
    hm.setFailureCallback([&](int worker_id) {
        worker_failed = true;
        failed_worker_id = worker_id;
    });

    hm.start();
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    hm.heartbeat(1);
    hm.heartbeat(2);
    std::this_thread::sleep_for(std::chrono::milliseconds(700));

    EXPECT_TRUE(worker_failed);
    EXPECT_EQ(failed_worker_id, 3);

    hm.stop();
}