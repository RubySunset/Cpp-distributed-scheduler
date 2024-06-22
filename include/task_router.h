#pragma once

#include <optional>
#include <functional>

#include "task.h"

class TaskRouter {
public:
    TaskRouter();
    std::optional<std::string> eval_route(const TaskRequest &request);
private:
    // add routes here
    // each route has the signature std::string(const TaskRequest &request)
    std::string test_route(const TaskRequest &request);
    void add_routes();

    std::unordered_map<std::string, std::function<std::string(const TaskRequest&)>> routes;
};