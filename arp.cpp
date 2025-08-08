#include "arp.hpp"

#include <fmt/format.h>

namespace dpdk::arp {
Arp::Arp(std::vector<std::string> const& ports) : m_dpdk{ports} {
    m_dpdk.setup_port(0);
}
void Arp::start(uint32_t ip_address) {
    ::fmt::print("Запуск обработчика ARP с IP-адресом {}.{}.{}.{}\n",
                (ip_address >> 0) & 0xFF,
                (ip_address >> 8) & 0xFF,
                (ip_address >> 16) & 0xFF,
                (ip_address >> 24) & 0xFF);

    // Запуск обработки ARP-запросов
    m_dpdk.process_arp_requests(0, ip_address);

}
} // namespace dpdk::arp