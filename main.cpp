#include <iostream>

#include "include/master.h"
#include "include/worker.h"

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <mode> [master_address master_port]" << std::endl;
        return 1;
    }

    std::string mode = argv[1];

    try {
        if (mode == "master") {
            Master master(8080);
            master.run();
        } else if (mode == "worker" && argc == 4) {
            Worker worker(argv[2], std::stoi(argv[3]));
            worker.run();
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