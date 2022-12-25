cmake_minimum_required(VERSION 3.23)
project(hw1 C)

set(CMAKE_C_STANDARD 99)

add_executable(distlog reporter.c shmem.c logentry.c)
add_executable(udplogger logger.c shmem.c logentry.c)
add_executable(rexpmatch analyzer.c shmem.c logentry.c)
add_executable(udpclient udpclient.c)

#add_compile_definitions(PRINT_LOG)