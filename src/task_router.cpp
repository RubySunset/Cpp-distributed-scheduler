#include "task_router.h"

TaskRouter::TaskRouter() {
    add_routes();
}

std::optional<std::string> TaskRouter::eval_route(const TaskRequest &request) {
    if (routes.find(request.function_name) != routes.end()) {
        return routes[request.function_name](request);
    } else {
        return {};
    }
}

std::string TaskRouter::test_route(const TaskRequest &request) {
    std::string response;
    for (auto& [k, v] : request.arguments) {
        response += k + ":" + v + "&";
    }
    return response;
}

std::string TaskRouter::mult_route(const TaskRequest &request) {
    int acc = 0;
    for (auto& [k, v] : request.arguments) {
        acc += std::stoi(k) * std::stoi(v);
    }
    return std::to_string(acc);
}


void TaskRouter::add_routes() {
    routes["test"] = [this](const TaskRequest &request){ return test_route(request); };
    routes["mult"] = [this](const TaskRequest &request){ return mult_route(request); };
}