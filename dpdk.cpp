#include "dpdk.hpp"

#include <fmt/compile.h>
#include <fmt/format.h>
#include <rte_eal.h>
#include <rte_errno.h>
#include <rte_ethdev.h>

#include <string>
#include <vector>

namespace dpdk::main {
DPDK::DPDK(std::vector<std::string> const &ports) {
    std::vector<std::string> eal_params;

    eal_params.emplace_back("./app");

    eal_params.emplace_back("-l");
    eal_params.emplace_back(std::to_string(0UL));

    eal_params.emplace_back("-d");
    eal_params.emplace_back(M_MEMPOOL_RING_DRIVER);

    for (auto const &one_driver : M_DRIVERS) {
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

    ::fmt::print("DPDK parameters:\n");
    std::vector<char *> eal_printer;
    for (auto const &one_param : eal_params) {
        ::fmt::print("\t{}\n", one_param);
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
DPDK::~DPDK() { rte_eal_cleanup(); }
void DPDK::setup_port(uint16_t port_id) {
    // Проверка наличия доступных портов Ethernet
    uint16_t nb_ports = rte_eth_dev_count_avail();
    if (nb_ports == 0) {
        throw DPDKEx("Не найдено портов Ethernet");
    }

    if (port_id >= nb_ports) {
        throw DPDKEx(::fmt::format("Указанный порт {} недоступен. Максимальный номер порта: {}", port_id, nb_ports - 1));
    }

    // Создание мемпула для пакетов
    m_mbuf_pool = rte_pktmbuf_pool_create("MBUF_POOL",               // Имя пула
                                          NUM_MBUFS,                 // Количество буферов
                                          MBUF_CACHE_SIZE,           // Размер кэша на каждое ядро
                                          0,                         // Приватные данные размера
                                          RTE_MBUF_DEFAULT_BUF_SIZE, // Размер буфера
                                          rte_socket_id()            // Сокет для выделения памяти
    );

    if (m_mbuf_pool == nullptr) {
        throw DPDKEx(::fmt::format("Не удалось создать мемпул: {}", rte_strerror(rte_errno)));
    }

    // Настройка порта
    struct rte_eth_conf port_conf = {};

    // Настройка выбранного порта
    int ret = rte_eth_dev_configure(port_id, 1, 1, &port_conf);
    if (ret < 0) {
        throw DPDKEx(::fmt::format("Не удалось настроить порт {}: {}", port_id, rte_strerror(rte_errno)));
    }

    // Настройка TX и RX очередей
    ret = rte_eth_rx_queue_setup(port_id, 0, RX_RING_SIZE, rte_eth_dev_socket_id(port_id), nullptr, m_mbuf_pool);
    if (ret < 0) {
        throw DPDKEx(::fmt::format("Не удалось настроить rx очередь: {}", rte_strerror(rte_errno)));
    }

    ret = rte_eth_tx_queue_setup(port_id, 0, TX_RING_SIZE, rte_eth_dev_socket_id(port_id), nullptr);
    if (ret < 0) {
        throw DPDKEx(::fmt::format("Не удалось настроить tx очередь: {}", rte_strerror(rte_errno)));
    }

    // Запуск порта
    ret = rte_eth_dev_start(port_id);
    if (ret < 0) {
        throw DPDKEx(::fmt::format("Не удалось запустить порт {}: {}", port_id, rte_strerror(rte_errno)));
    }

    ::fmt::print("Порт {} успешно настроен и запущен\n", port_id);
}
void DPDK::process_arp_requests(uint16_t port_id, uint32_t ip_address) {
    // if (!m_initialized || m_mbuf_pool == nullptr) {
    // throw DPDKEx("DPDK не инициализирован или mbuf_pool не создан");
    // }

    ::fmt::print("Ожидание ARP запросов на порту {}...\n", port_id);

    // Буфер для приема пакетов
    std::array<rte_mbuf *, BURST_SIZE> rx_buffs{};

    while (true) {
        // Получаем пакеты

        if (const uint16_t RECV_PACKET = rte_eth_rx_burst(port_id, 0, rx_buffs.data(), BURST_SIZE); RECV_PACKET > 0) {
            ::fmt::print("Получено {} пакетов\n", RECV_PACKET);

            // Обрабатываем каждый принятый пакет
            for (std::size_t i = 0; i < RECV_PACKET; ++i) {
                auto *pkt = rx_buffs[i];

                // Получаем указатель на Ethernet заголовок
                auto *eth_hdr = rte_pktmbuf_mtod(pkt, rte_ether_hdr *);

                // Проверяем, является ли пакет ARP-запросом
                if (rte_be_to_cpu_16(eth_hdr->ether_type) == RTE_ETHER_TYPE_ARP) {
                    // Получаем ARP-заголовок
                    auto *arp_hdr = reinterpret_cast<rte_arp_hdr *>(eth_hdr + 1);

                    // Проверяем, что это ARP-запрос (ARPOP_REQUEST = 1)
                    if (rte_be_to_cpu_16(arp_hdr->arp_opcode) == RTE_ARP_OP_REQUEST) {
                        // Проверяем, что IP-адрес назначения соответствует нашему
                        uint32_t target_ip = arp_hdr->arp_data.arp_tip;

                        if (target_ip == ip_address) {
                            ::fmt::print("Получен ARP-запрос для IP {}.{}.{}.{}\n", (target_ip >> 0) & 0xFF, (target_ip >> 8) & 0xFF,
                                         (target_ip >> 16) & 0xFF, (target_ip >> 24) & 0xFF);

                            // Отправляем ARP-ответ
                            send_arp_reply(port_id, pkt, ip_address);
                        }
                    }
                }

                // Освобождаем пакет
                rte_pktmbuf_free(pkt);
            }
        }
    }
}
void DPDK::send_arp_reply(uint16_t port_id, struct rte_mbuf *arp_request, uint32_t src_ip) {
    // // Получаем Ethernet и ARP заголовки из запроса
    // struct rte_ether_hdr* req_eth_hdr = rte_pktmbuf_mtod(arp_request, struct rte_ether_hdr*);
    // struct rte_arp_hdr* req_arp_hdr = (struct rte_arp_hdr*)(req_eth_hdr + 1);
    //
    // // Создаем новый буфер для ответа
    // struct rte_mbuf* arp_reply = rte_pktmbuf_alloc(m_mbuf_pool);
    // if (arp_reply == nullptr) {
    //     ::fmt::print("Ошибка: не удалось выделить память для ARP-ответа\n");
    //     return;
    // }
    //
    // // Получаем указатель на данные пакета
    // char* reply_data = rte_pktmbuf_mtod(arp_reply, char*);
    //
    // // Формируем Ethernet заголовок
    // struct rte_ether_hdr* reply_eth_hdr = reinterpret_cast<struct rte_ether_hdr*>(reply_data);
    //
    // // MAC-адрес получателя - это MAC-адрес отправителя ARP-запроса
    // rte_ether_addr_copy(&req_eth_hdr->src_addr, &reply_eth_hdr->dst_addr);
    //
    // // Получаем MAC-адрес нашего порта
    // struct rte_ether_addr my_mac;
    // rte_eth_macaddr_get(port_id, &my_mac);
    //
    // // MAC-адрес отправителя - наш MAC
    // rte_ether_addr_copy(&my_mac, &reply_eth_hdr->src_addr);
    //
    // // Тип Ethernet - ARP
    // reply_eth_hdr->ether_type = rte_cpu_to_be_16(RTE_ETHER_TYPE_ARP);
    //
    // // Формируем ARP заголовок
    // struct rte_arp_hdr* reply_arp_hdr = (struct rte_arp_hdr*)(reply_eth_hdr + 1);
    //
    // // Копируем основные поля из запроса
    // reply_arp_hdr->arp_hardware = req_arp_hdr->arp_hardware;
    // reply_arp_hdr->arp_protocol = req_arp_hdr->arp_protocol;
    // reply_arp_hdr->arp_hlen = req_arp_hdr->arp_hlen;
    // reply_arp_hdr->arp_plen = req_arp_hdr->arp_plen;
    //
    // // Устанавливаем тип операции как ответ (ARPOP_REPLY = 2)
    // reply_arp_hdr->arp_opcode = rte_cpu_to_be_16(RTE_ARP_OP_REPLY);
    //
    // // Заполняем ARP данные
    // // SHA (отправитель hardware address) - наш MAC
    // rte_ether_addr_copy(&my_mac, &reply_arp_hdr->arp_data.arp_sha);
    //
    // // SPA (отправитель protocol address) - наш IP
    // reply_arp_hdr->arp_data.arp_sip = src_ip;
    //
    // // THA (получатель hardware address) - MAC-адрес отправителя запроса
    // rte_ether_addr_copy(&req_arp_hdr->arp_data.arp_sha, &reply_arp_hdr->arp_data.arp_tha);
    //
    // // TPA (получатель protocol address) - IP-адрес отправителя запроса
    // reply_arp_hdr->arp_data.arp_tip = req_arp_hdr->arp_data.arp_sip;
    //
    // // Устанавливаем длину пакета
    // arp_reply->data_len = sizeof(struct rte_ether_hdr) + sizeof(struct rte_arp_hdr);
    // arp_reply->pkt_len = arp_reply->data_len;
    //
    // ::fmt::print("Отправка ARP-ответа размером {} байт через порт {}\n", arp_reply->pkt_len, port_id);
    //
    // // Отправляем ответ
    // uint16_t nb_tx = rte_eth_tx_burst(port_id, 0, &arp_reply, 1);
    // if (nb_tx != 1) {
    //     ::fmt::print("Ошибка при отправке ARP-ответа: отправлено {} из 1\n", nb_tx);
    //     rte_pktmbuf_free(arp_reply);
    // } else {
    //     ::fmt::print("ARP-ответ успешно отправлен\n");
    // }
}
} // namespace dpdk::main