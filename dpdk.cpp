#include "dpdk.hpp"

#include <fmt/compile.h>
#include <fmt/format.h>
#include <rte_eal.h>
#include <rte_errno.h>

#include <iostream>
#include <string>
#include <vector>

namespace dpdk::main {
DPDK::~DPDK() {rte_eal_cleanup();}
void DPDK::init(std::vector<std::string> const &ports) {
    std::vector<std::string> eal_params;

    eal_params.emplace_back("./app");

    eal_params.emplace_back("-l");
    eal_params.emplace_back(std::to_string(0UL));

    eal_params.emplace_back("-d");
    eal_params.emplace_back(M_MEMPOOL_RING_DRIVER);

    for (auto one_driver : M_DRIVERS) {
        eal_params.emplace_back("-d");
        eal_params.emplace_back(one_driver);
    }

    eal_params.emplace_back("-d");
    eal_params.emplace_back(M_PCAP_DRIVER);

    eal_params.emplace_back("--file-prefix");
    eal_params.emplace_back(M_DPDK_CORE_STATIC_NAME);
    eal_params.emplace_back("--proc-type=primary");

    if (!ports.empty()) {
        for (auto const &one_port : ports) {
            eal_params.emplace_back("--allow");
            eal_params.emplace_back(one_port);
        }
    }

    std::vector<char *> eal_printer;
    for (auto const &one_param : eal_params) {
        std::cout << "\t" << one_param << std::endl;
        eal_printer.emplace_back(const_cast<char *>(one_param.data()));
    }

    {
        auto const INIT_RES = rte_eal_init(static_cast<int>(eal_printer.size()), eal_printer.data());
        auto const TMP_ERRNO = rte_errno;
        if (INIT_RES < 0) {
            throw InitDpdk(::fmt::format(FMT_COMPILE("Init: {}"), rte_strerror(TMP_ERRNO)));
        }
    }
}
} // namespace dpdk::main