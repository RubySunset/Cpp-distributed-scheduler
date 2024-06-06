# Cpp-distributed-scheduler

A scalable, fault-tolerant task scheduler written in C++. A single master node is responsible for dispatching tasks to worker nodes over a network.

- Workers can dynamically enter the system and leave, allowing for easy horizontal scalability.
- The master node keeps track of the load on each worker node, allowing it to perform load balancing.
- A heartbeat mechanism allows the master node to monitor the health of worker nodes. If a worker node is detected to be down, any tasks assigned to it will be reassigned to another healthy node.

## Structure

- `include` contains header files.
- `src` contains source files.
- `tests` contains a full suite of both unit tests and integration tests.
- CMake is recommended as the easiest way to build this program.