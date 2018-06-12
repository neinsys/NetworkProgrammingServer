cmake_minimum_required(VERSION 3.10)
PROJECT(ttac)
find_package(Threads)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/mysql/include)
link_directories(${CMAKE_CURRENT_SOURCE_DIR}/mysql/lib)


ADD_EXECUTABLE(ttac ttac.c stream.c list.c http.c connection_queue.c connection_queue.h string_util.c string_util.h)
target_link_libraries(ttac ${CMAKE_THREAD_LIBS_INIT})
target_link_libraries(ttac ${CMAKE_CURRENT_SOURCE_DIR}/mysql/lib/libmysqlclient.so)