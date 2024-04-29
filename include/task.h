#pragma once

#include <functional>
#include <stdexcept>

class Task {
public:
    Task(int id, std::function<void()> func, int priority);
    void execute();
    int getId() const;
    int getPriority() const;

private:
    int id_;
    std::function<void()> func_;
    int priority_;
};