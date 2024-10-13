# Distributed Task Scheduler

A scalable, fault-tolerant distributed task scheduler written in C++. A single master node is responsible for dispatching tasks to worker nodes over a network, and returning results via a blocking interface.

- Workers can dynamically enter the system and leave, allowing for easy horizontal scalability. Epoll is used to allow the master to handle a large number of worker nodes.
- The master node provides an interface in the form of a blocking function, allowing other software components to treat it as an RPC.
- Tasks can be registered in a task router, taking a map of key-value pairs as arguments and returning a string value (if successful). Any arbitrary logic can be processed.
- The master node performs load balancing by keeping track of the load on each worker node, and priority-based scheduling is implemented on the master-side.
- A heartbeat mechanism allows the master node to monitor the health of worker nodes. If a worker node is detected to be down, any tasks assigned to it will be reassigned to another healthy node.
- Uses Protobuf for messaging for speed and efficient use of bandwidth.
- Unit-tested and integration-tested using the GoogleTest framework.

Note: this scheduler could, in principle, be extended for use as a load balancer in front of an array of web servers. 

## Structure

- `include` contains header files.
- `proto` contains Protobuf definitions.
- `src` contains source files.
- `tests` contains a full suite of both unit tests and integration tests.
- CMake is the recommended build system.