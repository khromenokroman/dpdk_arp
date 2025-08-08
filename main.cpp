#include <fmt/compile.h>

#include <iostream>

#include "arp.hpp"

int main() {
    try {
        std::vector<std::string> ports{"0000:00:03.0"};
        dpdk::arp::Arp arp(ports);
        arp.start();
        return 0;
    } catch (std::exception const &ex) {
        ::fmt::print(stderr, "Exception: {}\n", ex.what());
        return -1;
    }
}