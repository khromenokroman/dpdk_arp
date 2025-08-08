#include <arpa/inet.h>
#include <bits/socket.h>
#include <fmt/compile.h>

#include <iostream>

#include "arp.hpp"

// Функция для преобразования IP-адреса из строки в uint32_t
uint32_t ip_to_uint32(const char* ip_str) {
    in_addr addr{};
    inet_pton(AF_INET, ip_str, &addr);
    return addr.s_addr;
}

int main() {
    try {
        std::vector<std::string> ports{"0000:00:03.0"};
        // IP-адрес, для которого будем обрабатывать ARP-запросы
        char const* ip_str = "192.168.1.100";
        ::fmt::print("Запуск ARP-сервера для IP: {}\n", ip_str);

        dpdk::arp::Arp arp(ports);
        arp.start(ip_to_uint32(ip_str));
        return 0;
    } catch (std::exception const& ex) {
        ::fmt::print(stderr, "Exception: {}\n", ex.what());
        return -1;
    }
}