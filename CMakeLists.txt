PROJECT(ttac)
find_package(Threads)
ADD_EXECUTABLE(ttac ttac.c stream.c list.c)
target_link_libraries(ttac ${CMAKE_THREAD_LIBS_INIT})