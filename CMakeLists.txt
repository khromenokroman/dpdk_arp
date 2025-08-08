cmake_minimum_required(VERSION 3.31)
project(dpdk_sender)

set(CMAKE_CXX_STANDARD 20)

find_package(PkgConfig REQUIRED)
find_package(fmt REQUIRED)
pkg_check_modules(DPDK REQUIRED libdpdk)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mssse3 -msse4.2")

add_executable(${PROJECT_NAME} main.cpp arp.cpp dpdk.cpp)

target_link_libraries(${PROJECT_NAME} PRIVATE ${DPDK_LIBRARIES} fmt::fmt)

add_compile_definitions(
        RTE_LIBRTE_MEMPOOL_DEBUG
        RTE_LIBRTE_MEMPOOL_STATS
)