#include "task.h"

TaskRequest::TaskRequest(
    int id,
    int priority,
    const std::string &function_name,
    const std::unordered_map<std::string, std::string> &arguments)
{
    this->id = id;
    this->priority = priority;
    this->function_name = function_name;
    this->arguments = arguments;
}

TaskRequest::TaskRequest(const std::string &s) {
    task::TaskRequest request;
    std::string decoded_str = base64_decode(s);
    request.ParseFromString(decoded_str);
    id = request.id();
    function_name = request.function_name();
    for (auto& [k, v] : request.arguments()) {
        arguments[k] = v;
    }
}

std::string TaskRequest::to_string() {
    task::TaskRequest request;
    request.set_id(id);
    request.set_function_name(function_name);
    for (auto& [k, v] : arguments) {
        (*request.mutable_arguments())[k] = v;
    }
    std::string raw_str;
    request.SerializeToString(&raw_str);
    return base64_encode(raw_str);
}

TaskResponse::TaskResponse(int id, bool success, const std::string &return_value) {
    this->id = id;
    this->success = success;
    this->return_value = return_value;
}

TaskResponse::TaskResponse(const std::string &s) {
    task::TaskResponse response;
    std::string decoded_str = base64_decode(s);
    response.ParseFromString(decoded_str);
    id = response.id();
    success = response.success();
    return_value = response.return_value();
}

std::string TaskResponse::to_string() {
    task::TaskResponse response;
    response.set_id(id);
    response.set_success(success);
    response.set_return_value(return_value);
    std::string raw_str;
    response.SerializeToString(&raw_str);
    return base64_encode(raw_str);
}