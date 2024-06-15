cmake_minimum_required(VERSION 3.5.0)
project(Cpp-distributed-scheduler VERSION 0.1.0 LANGUAGES C CXX)

find_package(Protobuf REQUIRED)
ADD_SUBDIRECTORY(proto)
INCLUDE_DIRECTORIES(${CMAKE_CURRENT_BINARY_DIR})
INCLUDE_DIRECTORIES(${Protobuf_INCLUDE_DIRS})

# INCLUDE_DIRECTORIES(include)
# ADD_EXECUTABLE(main main.cpp)
# TARGET_LINK_LIBRARIES(main proto ${Protobuf_LIBRARIES})




# Compile source as library
include_directories(include)
file(GLOB SOURCES "src/*")
add_library(Sources ${SOURCES})
target_link_libraries(Sources proto ${Protobuf_LIBRARIES})

# Link main to libraries
add_executable(main "main.cpp")
target_link_libraries(main Sources)

# include(FetchContent)
# FetchContent_Declare(
#   googletest
#   URL https://github.com/google/googletest/archive/03597a01ee50ed33e9dfd640b249b4be3799d395.zip
# )
# # For Windows: Prevent overriding the parent project's compiler/linker settings
# set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
# FetchContent_MakeAvailable(googletest)

# enable_testing()

# file(GLOB TEST_SOURCES "tests/*.cpp")

# add_executable(TestExec ${TEST_SOURCES})
# target_link_libraries(TestExec Sources)
# target_link_libraries(TestExec GTest::gtest_main)

# include(GoogleTest)
# gtest_discover_tests(TestExec)