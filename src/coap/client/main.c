#include <stdio.h>
#include <unistd.h>

#include "msg.h"

#include "net/gcoap.h"
#include "net/sock.h"
#include "net/netif.h"
#include "benchmark.h"

#define GCOAP_PDU_BUF_SIZE CONFIG_GCOAP_PDU_BUF_SIZE * 16

void print_iface(netif_t *iface)
{
    char name[CONFIG_NETIF_NAMELENMAX];
    netif_get_name(iface, name);
    printf("iface: %s\n", name);

    ipv6_addr_t ipv6_addr[1];

    if (netif_get_opt(iface, NETOPT_IPV6_ADDR, 0, ipv6_addr, sizeof(ipv6_addr)) >= 0)
    {
        char addr_str[40];
        ipv6_addr_to_str(addr_str, ipv6_addr, sizeof(addr_str));
        printf("inet6 addr: %s\n", addr_str);
    }
}

int gcoap_test0(char *server_addr_str, size_t payload_length, size_t runs)
{
    // build request net layer
    sock_udp_ep_t remote_mem;
    sock_udp_ep_t *remote = &remote_mem;
    ipv6_addr_t addr;
    remote->family = AF_INET6;
    remote->port = CONFIG_GCOAP_PORT;

    netif_t *iface = NULL;

    if ((iface = netif_iter(iface)))
    {
        print_iface(iface);
        remote->netif = netif_get_id(iface);
    }
    else
    {
        printf("gcoap_client: not interface found\n");
        return -1;
    }

    if (ipv6_addr_from_str(&addr, server_addr_str) == NULL)
    {
        printf("gcoap_client: unable to parse destination address\n");
        return -1;
    }
    memcpy(&remote->addr.ipv6[0], &addr.u8[0], sizeof(addr.u8));

    // build request app layer
    uint8_t buf[GCOAP_PDU_BUF_SIZE];
    coap_pkt_t pdu;
    size_t len;
    //char path[] = "/.well-known/core";
    char path[] = "/riot/test0";

    len = gcoap_req_init(&pdu, &buf[0], GCOAP_PDU_BUF_SIZE,
                         COAP_METHOD_GET, &path[0]);

    printf("len: %i\n", len);

    coap_hdr_set_type(pdu.hdr, COAP_TYPE_CON);
    coap_opt_add_format(&pdu, COAP_FORMAT_JSON);
    len += coap_opt_finish(&pdu, COAP_OPT_FINISH_PAYLOAD);
    printf("len new: %i\n", len);

    char payload[payload_length];
    memset(payload, 'a', payload_length);
    payload[payload_length - 1] = 0;
    //size_t paylen = strlen(payload);
    if (pdu.payload_len >= payload_length)
    {
        memcpy(pdu.payload, payload, payload_length);
        len += payload_length;
    }
    else
    {
        printf("gcoap_cli: msg buffer too small\n");
        return 1;
    }

    uint32_t _benchmark_time;
    uint32_t _benchmark_time_sum = 0;
    for (unsigned long i = 0; i < runs; i++)
    {
        _benchmark_time = xtimer_now_usec();
        gcoap_req_send((uint8_t *)pdu.hdr, len, remote,
                       NULL, NULL);
        _benchmark_time = (xtimer_now_usec() - _benchmark_time);
        _benchmark_time_sum += _benchmark_time;
        //printf("bytes_sent: %i\n", bytes_sent);
        usleep(10 * 1000);
    }

    return _benchmark_time_sum;
}

int main(void)
{
    printf("======================================================================\n");
    char *server_addr_str = "fe80::200:ff:fe00:ab";

    size_t runs = 10000;
    uint32_t _benchmark_time_sum;

    for (size_t payload_length = 16; payload_length <= 1024; payload_length *= 4)
    {
        printf("payload_length: %i\n", payload_length);
        _benchmark_time_sum = gcoap_test0(server_addr_str, payload_length, runs);
        benchmark_print_time(_benchmark_time_sum, runs, "coap_send");
    }

    return 0;
}
