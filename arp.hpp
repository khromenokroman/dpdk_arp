#pragma once
#include <atomic>

#include "dpdk.hpp"

namespace dpdk::arp {
class Arp {
   public:
    Arp() = default;
    ~Arp() = default;

    Arp(Arp const&) = delete;
    Arp(Arp&&) = delete;
    Arp& operator=(Arp const&) = delete;
    Arp& operator=(Arp&&) = delete;

    void start();

   private:
    main::DPDK m_dpdk;           // 4
    std::atomic<bool> m_running; // 1
};
} // namespace dpdk::arp