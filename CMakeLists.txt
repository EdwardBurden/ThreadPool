cmake_minimum_required(VERSION 3.10)
project(ThreadPool VERSION 1.0)

set(CMAKE_CXX_STANDARD 20)

file(GLOB THREADPOOL "*.cpp" "*.h")

add_executable(${PROJECT_NAME} ${THREADPOOL})

install(TARGETS ThreadPool RUNTIME DESTINATION .)