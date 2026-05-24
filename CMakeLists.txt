
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
cmake_minimum_required(VERSION 3.10)
project(ThreadPool VERSION 1.0)
set(CMAKE_CXX_STANDARD 20)
file(GLOB THREADPOOL "*.cpp" "*.h")
add_executable(${PROJECT_NAME} ${THREADPOOL})

option(THREADPOOL_DEBUG_LOGGING "Enable threadpool debug logging" OFF)
if(THREADPOOL_DEBUG_LOGGING)
    target_compile_definitions(${PROJECT_NAME} PRIVATE THREADPOOL_DEBUG)
else()
    target_compile_definitions(${PROJECT_NAME} PRIVATE 
        $<$<CONFIG:Debug>:THREADPOOL_DEBUG>
    )
endif()

install(TARGETS ThreadPool RUNTIME DESTINATION .)