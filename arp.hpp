#pragma once
#include <atomic>

#include "dpdk.hpp"

namespace dpdk::arp {
class Arp {
   public:
    explicit Arp(std::vector<std::string> const& ports);
    ~Arp() = default;

    Arp(Arp const&) = delete;
    Arp(Arp&&) = delete;
    Arp& operator=(Arp const&) = delete;
    Arp& operator=(Arp&&) = delete;

    void start(uint32_t ip_address);

   private:
    main::DPDK m_dpdk;           // 8
    std::atomic<bool> m_running; // 1
    static_assert(sizeof m_dpdk == 8);
};
} // namespace dpdk::arp