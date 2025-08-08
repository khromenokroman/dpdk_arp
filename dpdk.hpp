#pragma once
#include <rte_mempool.h>

#include <array>
#include <cstdint>
#include <memory>
#include <stdexcept>
#include <vector>
struct rte_mbuf;

namespace dpdk::main {

struct DPDKEx : std::runtime_error {
    using std::runtime_error::runtime_error;
};
struct InitDpdk final : DPDKEx {
    using DPDKEx::DPDKEx;
};

class DPDK {
   public:
    explicit DPDK(std::vector<std::string> const& ports);
    ~DPDK();

    DPDK(DPDK const&) = delete;
    DPDK(DPDK&&) = delete;
    DPDK& operator=(DPDK const&) = delete;
    DPDK& operator=(DPDK&&) = delete;

    void setup_port(uint16_t port_id);
    void process_arp_requests(uint16_t port_id, uint32_t ip_address);
    void send_arp_reply(uint16_t port_id, struct rte_mbuf* arp_request, uint32_t src_ip);

   private:
    static constexpr std::array<char const*, 6> M_DRIVERS{"librte_net_i40e.so.24",    "librte_net_ixgbe.so.24", "librte_net_e1000.so.24",
                                                          "librte_net_vmxnet3.so.24", "librte_net_ice.so.24",   "librte_net_tap.so.24.0"}; // 48
    static constexpr auto M_DPDK_CORE_STATIC_NAME = "dpdk_core";                                                                           // 8
    static constexpr auto M_MEMPOOL_RING_DRIVER = "librte_mempool_ring.so.24";                                                             // 8
    static constexpr auto M_PCAP_DRIVER = "librte_net_pcap.so.24";                                                                         // 8
    rte_mempool* m_mbuf_pool = nullptr;                                                                                                    // 8
    static constexpr uint32_t M_MAIN_CORE = 0;                                                                                             // 4
    static constexpr uint16_t RX_RING_SIZE = 128;                                                                                          // 2
    static constexpr uint16_t TX_RING_SIZE = 512;                                                                                          // 2
    static constexpr uint16_t NUM_MBUFS = 8191;                                                                                            // 2
    static constexpr uint16_t MBUF_CACHE_SIZE = 250;                                                                                       // 2
    static constexpr uint16_t BURST_SIZE = 32;                                                                                             // 2
};
} // namespace dpdk::main