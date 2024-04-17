#pragma once

#include <functional>

class Task {
public:
    Task(int id, std::function<void()> func);
    void execute();
    int getId() const;

private:
    int id_;
    std::function<void()> func_;
};