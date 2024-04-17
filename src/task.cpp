#include "task.h"

Task::Task(int id, std::function<void()> func) : id_(id), func_(std::move(func)) {}

void Task::execute() { func_(); }

int Task::getId() const { return id_; }