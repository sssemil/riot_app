#include <stdio.h>
#include <unistd.h>

#include "msg.h"

#include "net/gcoap.h"
#include "net/dsm.h"
#include "net/sock.h"
#include "net/netif.h"
#include "od.h"
#include "benchmark.h"

#include "tinydtls_keys.h"
#include "net/credman.h"

#define ENABLE_DEBUG 0
#include "debug.h"

#define GCOAP_PDU_BUF_SIZE CONFIG_GCOAP_PDU_BUF_SIZE * 16

#define SOCK_DTLS_CLIENT_TAG (2)
static const uint8_t psk_id_0[] = PSK_DEFAULT_IDENTITY;
static const uint8_t psk_key_0[] = PSK_DEFAULT_KEY;

static const credman_credential_t credential = {
    .type = CREDMAN_TYPE_PSK,
    .tag = SOCK_DTLS_CLIENT_TAG,
    .params = {
        .psk = {
            .key = {
                .s = psk_key_0,
                .len = sizeof(psk_key_0) - 1,
            },
            .id = {
                .s = psk_id_0,
                .len = sizeof(psk_id_0) - 1,
            },
        }},
};

static void _resp_handler(const gcoap_request_memo_t *memo, coap_pkt_t *pdu,
                          const sock_udp_ep_t *remote);

/* Retain request path to re-request if response includes block. User must not
 * start a new request (with a new path) until any blockwise transfer
 * completes or times out. */
#define _LAST_REQ_PATH_MAX (64)
static char _last_req_path[_LAST_REQ_PATH_MAX];

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

/*
 * Response callback.
 */
static void _resp_handler(const gcoap_request_memo_t *memo, coap_pkt_t *pdu,
                          const sock_udp_ep_t *remote)
{
    (void)remote; /* not interested in the source currently */
    printf("_resp_handler called!!!\n");

    if (memo->state == GCOAP_MEMO_TIMEOUT)
    {
        printf("gcoap: timeout for msg ID %02u\n", coap_get_id(pdu));
        return;
    }
    else if (memo->state == GCOAP_MEMO_ERR)
    {
        printf("gcoap: error in response\n");
        return;
    }

    coap_block1_t block;
    if (coap_get_block2(pdu, &block) && block.blknum == 0)
    {
        puts("--- blockwise start ---");
    }

    size_t bytes_sent;
    char *class_str = (coap_get_code_class(pdu) == COAP_CLASS_SUCCESS)
                          ? "Success"
                          : "Error";
    printf("gcoap: response %s, code %1u.%02u", class_str,
           coap_get_code_class(pdu),
           coap_get_code_detail(pdu));
    if (pdu->payload_len)
    {
        unsigned content_type = coap_get_content_type(pdu);
        if (content_type == COAP_FORMAT_TEXT || content_type == COAP_FORMAT_LINK || coap_get_code_class(pdu) == COAP_CLASS_CLIENT_FAILURE || coap_get_code_class(pdu) == COAP_CLASS_SERVER_FAILURE)
        {
            /* Expecting diagnostic payload in failure cases */
            printf(", %u bytes\n%.*s\n", pdu->payload_len, pdu->payload_len,
                   (char *)pdu->payload);
        }
        else
        {
            printf(", %u bytes\n", pdu->payload_len);
            od_hex_dump(pdu->payload, pdu->payload_len, OD_WIDTH_DEFAULT);
        }
    }
    else
    {
        printf(", empty payload\n");
    }

    /* ask for next block if present */
    if (coap_get_block2(pdu, &block))
    {
        if (block.more)
        {
            unsigned msg_type = coap_get_type(pdu);
            if (block.blknum == 0 && !strlen(_last_req_path))
            {
                puts("Path too long; can't complete blockwise");
                return;
            }

            gcoap_req_init(pdu, (uint8_t *)pdu->hdr, CONFIG_GCOAP_PDU_BUF_SIZE,
                           COAP_METHOD_GET, _last_req_path);

            if (msg_type == COAP_TYPE_ACK)
            {
                coap_hdr_set_type(pdu->hdr, COAP_TYPE_CON);
            }
            block.blknum++;
            coap_opt_add_block2_control(pdu, &block);

            int len = coap_opt_finish(pdu, COAP_OPT_FINISH_NONE);
            bytes_sent = gcoap_req_send((uint8_t *)pdu->hdr, len, remote,
                                        _resp_handler, memo->context);
            if (bytes_sent <= 0)
            {
                printf("gcoap_cli: no bytes sent in get block2!!!\n");
            }
        }
        else
        {
            puts("--- blockwise complete ---");
        }
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
    char path[] = "/.well-known/core";
    //char path[] = "/riot/test0";
    size_t path_len = strlen(path);

    memset(_last_req_path, 0, _LAST_REQ_PATH_MAX);
    if (path_len < _LAST_REQ_PATH_MAX)
    {
        memcpy(_last_req_path, path, path_len);
    }

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

    size_t bytes_sent;
    uint32_t _benchmark_time;
    uint32_t _benchmark_time_sum = 0;
    for (unsigned long i = 0; i < runs; i++)
    {
        _benchmark_time = xtimer_now_usec();
        bytes_sent = gcoap_req_send((uint8_t *)pdu.hdr, len, remote,
                                    _resp_handler, NULL);
        if (bytes_sent <= 0)
        {
            printf("gcoap_cli: no bytes sent!!!\n");
        }
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

    if (IS_ACTIVE(CONFIG_GCOAP_ENABLE_DTLS))
    {
        int res = credman_add(&credential);
        if (res < 0 && res != CREDMAN_EXIST)
        {
            /* ignore duplicate credentials */
            printf("Error cannot add credential to system: %d\n", (int)res);
            return -1;
        }
    }

    if (IS_ACTIVE(CONFIG_GCOAP_ENABLE_DTLS))
    {
        printf("Connection secured with DTLS\n");
        printf("Free DTLS session slots: %d/%d\n", dsm_get_num_available_slots(),
               dsm_get_num_maximum_slots());
    }

    char *server_addr_str = "fe80::200:ff:fe00:ab";

    size_t runs = 1;
    uint32_t _benchmark_time_sum;

    for (size_t payload_length = 16; payload_length <= /* 1024 */ 16; payload_length *= 4)
    {
        printf("payload_length: %i\n", payload_length);
        _benchmark_time_sum = gcoap_test0(server_addr_str, payload_length, runs);
        //benchmark_print_time(_benchmark_time_sum, runs, "coap_send");
        (void)_benchmark_time_sum;
    }

    return 0;
}
