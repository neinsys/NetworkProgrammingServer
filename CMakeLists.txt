cmake_minimum_required(VERSION 3.10)
PROJECT(ttac)
find_package(Threads)
include_directories(/usr/include/mysql)

ADD_EXECUTABLE(ttac ttac.c stream.c list.c http.c)
target_link_libraries(ttac ${CMAKE_THREAD_LIBS_INIT})