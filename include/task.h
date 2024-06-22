#pragma once

#include <string>
#include <unordered_map>

#include "base64.h"
#include "proto/task.pb.h"

class TaskRequest {
public:
    TaskRequest(int id, int priority, const std::string &function_name, const std::unordered_map<std::string, std::string> &arguments);
    TaskRequest(const std::string &s);
    std::string to_string();

    int id;
    int priority;
    std::string function_name;
    std::unordered_map<std::string, std::string> arguments; // arguments represented as key-value pairs
};

class TaskResponse {
public:
    TaskResponse() = default;
    TaskResponse(int id, bool success, const std::string &return_value);
    TaskResponse(const std::string &s);
    std::string to_string();

    int id;
    bool success;
    std::string return_value;
};