cmake_minimum_required(VERSION 3.5.0)

project(MYTHREAD_POOL)

# find_package(Threads REQUIRED)
include_directories(include)

add_executable(mythread_pool_cmake src/condition.c src/threadpool.c main.c)

target_link_libraries(mythread_pool_cmake 
                        pthread)