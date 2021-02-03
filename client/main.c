#include <stdio.h>
#include "msg.h"

#include "net/gcoap.h"
#include "net/sock.h"
#include "net/netif.h"
#include "benchmark.h"

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

int main(void)
{

    // build request net layer
    char *addr_str = "fe80::200:ff:fe00:ab";
    size_t bytes_sent;
    sock_udp_ep_t remote_mem;
    sock_udp_ep_t *remote = &remote_mem;
    ipv6_addr_t addr;
    remote->family = AF_INET6;

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

    if (ipv6_addr_from_str(&addr, addr_str) == NULL)
    {
        printf("gcoap_client: unable to parse destination address\n");
        return -1;
    }
    memcpy(&remote->addr.ipv6[0], &addr.u8[0], sizeof(addr.u8));

    // build request app layer
    uint8_t buf[CONFIG_GCOAP_PDU_BUF_SIZE];
    coap_pkt_t pdu;
    size_t len;
    char path[] = "/.well-known/core";

    len = gcoap_req_init(&pdu, &buf[0], CONFIG_GCOAP_PDU_BUF_SIZE,
                         COAP_METHOD_GET, &path[0]);

    printf("len: %i\n", len);

    //coap_hdr_set_type(pdu.hdr, COAP_TYPE_CON);
    coap_opt_add_format(&pdu, COAP_FORMAT_JSON);
    len += coap_opt_finish(&pdu, COAP_OPT_FINISH_PAYLOAD);
    printf("len new: %i\n", len);

    char *payload = "1234567890abcdef";
    size_t paylen = strlen(payload);
    if (pdu.payload_len >= paylen)
    {
        memcpy(pdu.payload, payload, paylen);
        len += paylen;
    }
    else
    {
        printf("gcoap_cli: msg buffer too small\n");
        return 1;
    }
    bytes_sent = gcoap_req_send((uint8_t *)pdu.hdr, len, remote,
                                NULL, NULL);
    printf("bytes_sent: %i\n", bytes_sent);

    /*unsigned long runs = 10000;
    uint32_t _benchmark_time = xtimer_now_usec();
    for (unsigned long i = 0; i < runs; i++)
    {
    }
    _benchmark_time = (xtimer_now_usec() - _benchmark_time);
    benchmark_print_time(_benchmark_time, runs, "coap_send");*/

    return 0;
}
