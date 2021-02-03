#include <stdio.h>
#include "msg.h"

#include "net/gcoap.h"
#include "net/sock.h"
#include "net/netif.h"

int main(void)
{
    uint8_t buf[CONFIG_GCOAP_PDU_BUF_SIZE];
    coap_pkt_t pdu;
    size_t len;
    char path[] = "/.well-known/core";

    // prepare
    len = gcoap_request(&pdu, &buf[0], CONFIG_GCOAP_PDU_BUF_SIZE,
                        COAP_METHOD_GET, &path[0]);

    printf("len: %i\n", len);

    // build request
    char *addr_str = "fe80::7029:f6ff:fe08:af0b";
    size_t bytes_sent;
    sock_udp_ep_t remote_mem;
    sock_udp_ep_t *remote = &remote_mem;
    ipv6_addr_t addr;
    remote->family = AF_INET6;

    netif_t *iface = NULL;

    if ((iface = netif_iter(iface)))
    {
        char name[CONFIG_NETIF_NAMELENMAX];
        netif_get_name(iface, name);
        printf("iface: %s\n", name);
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

    // send
    bytes_sent = gcoap_req_send(buf, len, remote, NULL, NULL);

    printf("bytes_sent: %i\n", bytes_sent);
    return 0;
}
