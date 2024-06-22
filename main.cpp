#include <iostream>
#include <stdexcept>
#include <csignal>
#include <memory>

#include "include/master.h"
#include "include/worker.h"

std::unique_ptr<Master> master_ptr;
std::unique_ptr<Worker> worker_ptr;

// void signal_handler(int signum) {
//     std::cout << "\nInterrupt signal (" << signum << ") received.\n";
//     if (master_ptr) {
//         std::cout << "Stopping Master...\n";
//         master_ptr->stop();
//     }
//     if (worker_ptr) {
//         std::cout << "Stopping Worker...\n";
//         worker_ptr->stop();
//     }
// }

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <mode> [master_address master_port]" << std::endl;
        return 1;
    }

    std::string mode = argv[1];

    // // Register signal handler
    // std::signal(SIGINT, signal_handler);

    try {
        if (mode == "master") {
            master_ptr = std::make_unique<Master>(8080);
            // master_ptr->run();
            int foo;
            while (true) {
                std::cin >> foo;
                if (foo == -1) {
                    break;
                }
                std::thread t{[](){
                    auto result = master_ptr->process(
                        1,
                        "test",
                        std::unordered_map<std::string, std::string>{{"k1", "v1"}, {"k2", "v2"}}
                    );
                    if (result.has_value()) {
                        std::cout << "return value = " << *result << '\n';
                    } else {
                        std::cout << "error\n";
                    }
                }};
                t.detach();
            }
        } else if (mode == "worker" && argc == 4) {
            worker_ptr = std::make_unique<Worker>(argv[2], std::stoi(argv[3]));
            worker_ptr->run();
        } else {
            std::cerr << "Invalid mode or missing arguments" << std::endl;
            return 1;
        }
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}