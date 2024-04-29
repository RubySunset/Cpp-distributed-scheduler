#include "task.h"

Task::Task(int id, std::function<void()> func, int priority) 
    : id_(id), func_(std::move(func)), priority_(priority) {
    if (priority < 1 || priority > 10) {
        throw std::invalid_argument("Priority must be between 1 and 10");
    }
}

void Task::execute() { func_(); }

int Task::getId() const { return id_; }

int Task::getPriority() const { return priority_; }