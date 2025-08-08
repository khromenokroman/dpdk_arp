#include "arp.hpp"

namespace dpdk::arp {
Arp::Arp(std::vector<std::string> const& ports) { main::DPDK::init(ports); }
void Arp::start() {}
} // namespace dpdk::arp