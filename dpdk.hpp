#pragma once
#include <array>
#include <cstdint>
#include <stdexcept>
#include <vector>

namespace dpdk::main {

struct DPDKEx : std::runtime_error {
    using std::runtime_error::runtime_error;
};
struct InitDpdk final : DPDKEx {
    using DPDKEx::DPDKEx;
};

class DPDK {
   public:
    DPDK() = default;
    ~DPDK();

    DPDK(DPDK const&) = delete;
    DPDK(DPDK&&) = delete;
    DPDK& operator=(DPDK const&) = delete;
    DPDK& operator=(DPDK&&) = delete;

    static void init(std::vector<std::string> const& ports);

   private:
    static constexpr std::array<char const*, 6> M_DRIVERS{"librte_net_i40e.so.24",    "librte_net_ixgbe.so.24", "librte_net_e1000.so.24",
                                                          "librte_net_vmxnet3.so.24", "librte_net_ice.so.24",   "librte_net_tap.so.24.0"}; // 48
    static constexpr auto M_DPDK_CORE_STATIC_NAME = "dpdk_core";                                                                           // 8
    static constexpr auto M_MEMPOOL_RING_DRIVER = "librte_mempool_ring.so.24";                                                             // 8
    static constexpr auto M_PCAP_DRIVER = "librte_net_pcap.so.24";                                                                         // 8
    static constexpr uint32_t M_MAIN_CORE = 0;                                                                                             // 4
};
} // namespace dpdk::main