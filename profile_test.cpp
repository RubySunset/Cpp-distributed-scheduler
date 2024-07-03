#include <iostream>
#include <stdexcept>
#include <csignal>
#include <memory>

#include "include/master.h"
#include "include/worker.h"

int main() {
    Master master(8080);

    std::vector<std::thread> workers;
    for (int i = 0; i < 2; ++i) {
        std::thread t([](){
            Worker worker("127.0.0.1", 8080);
            worker.run();
        });
        workers.push_back(std::move(t));
    }

    auto result = master.process(
        1,
        "test",
        std::unordered_map<std::string, std::string>{{"k1", "v1"}, {"k2", "v2"}}
    );

    if (result.has_value()) {
        std::cout << "return value = " << *result << '\n';
    } else {
        std::cout << "error\n";
    }
}