#include <stdio.h>
#include <unistd.h>

#include "msg.h"

#include "net/gcoap.h"
#include "net/sock.h"
#include "net/netif.h"
#include "od.h"
#include "benchmark.h"

#ifndef VERBOSE
#define VERBOSE 0
#endif

#ifndef CONFIG_RUNS_COUNT
#define CONFIG_RUNS_COUNT 1000000
#endif

#ifndef CONFIG_BYTES_COUNT
#define CONFIG_BYTES_COUNT 4
#endif

#define GCOAP_PDU_BUF_SIZE CONFIG_GCOAP_PDU_BUF_SIZE * 16

#if IS_ACTIVE(CONFIG_GCOAP_ENABLE_DTLS)
#include "net/credman.h"
#include "net/dsm.h"
#include "tinydtls_keys.h"

static const uint8_t psk_id_0[] = PSK_DEFAULT_IDENTITY;
static const uint8_t psk_key_0[] = PSK_DEFAULT_KEY;
static const credman_credential_t credential = {
    .type = CREDMAN_TYPE_PSK,
    .tag = CONFIG_GCOAP_DTLS_CREDENTIAL_TAG,
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
#endif

uint32_t benchmark_time_sum = 0;
uint32_t rtt_replies_count = 0;

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
#if VERBOSE
    char *class_str = (coap_get_code_class(pdu) == COAP_CLASS_SUCCESS)
                          ? "Success"
                          : "Error";

    printf("gcoap: response %s, code %1u.%02u", class_str,
           coap_get_code_class(pdu),
           coap_get_code_detail(pdu));
#endif

    if (pdu->payload_len)
    {
        unsigned content_type = coap_get_content_type(pdu);
        if (content_type == COAP_FORMAT_TEXT)
        {
            uint32_t *payload_uint32_ptr = (uint32_t *)pdu->payload;
            uint32_t _benchmark_time = xtimer_now_usec() - *payload_uint32_ptr;

#if VERBOSE
            printf("rtt: %i μs\n", _benchmark_time);
#endif

            benchmark_time_sum += _benchmark_time;
            rtt_replies_count++;

            // if (content_type == COAP_FORMAT_TEXT || content_type == COAP_FORMAT_LINK || coap_get_code_class(pdu) == COAP_CLASS_CLIENT_FAILURE || coap_get_code_class(pdu) == COAP_CLASS_SERVER_FAILURE)
            // {
            //     /* Expecting diagnostic payload in failure cases */
            //     printf(", %u bytes\n%.*s\n", pdu->payload_len, pdu->payload_len,
            //            (char *)pdu->payload);
            // }
            // else
            // {
            //     printf(", %u bytes\n", pdu->payload_len);
            //     od_hex_dump(pdu->payload, pdu->payload_len, OD_WIDTH_DEFAULT);
            // }
        }
    }
    else
    {
#if VERBOSE
        printf(", empty payload\n");
#endif
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
    //char path[] = "/.well-known/core";
    char path[] = "/riot/test0";

    // set prev path for any path
    size_t path_len = strlen(path);
    memset(_last_req_path, 0, _LAST_REQ_PATH_MAX);
    if (path_len < _LAST_REQ_PATH_MAX)
    {
        memcpy(_last_req_path, path, path_len);
    }

    len = gcoap_req_init(&pdu, &buf[0], GCOAP_PDU_BUF_SIZE,
                         COAP_METHOD_GET, &path[0]);

#if VERBOSE
    printf("len: %i\n", len);
#endif

    coap_hdr_set_type(pdu.hdr, COAP_TYPE_CON);
    coap_opt_add_format(&pdu, COAP_FORMAT_JSON);
    len += coap_opt_finish(&pdu, COAP_OPT_FINISH_PAYLOAD);

#if VERBOSE
    printf("len new: %i\n", len);
#endif

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

    uint32_t *payload_uint32_ptr = (uint32_t *)pdu.payload;
    for (unsigned long i = 0; i < runs; i++)
    {
        *payload_uint32_ptr = xtimer_now_usec();

        gcoap_req_send((uint8_t *)pdu.hdr, len, remote,
                       _resp_handler, NULL);
        usleep(50 * 1000);
    }

    return 0;
}

int main(void)
{
    printf("======================================================================\n");
    char *server_addr_str = "fe80::200:ff:fe00:ab";

#if IS_ACTIVE(CONFIG_GCOAP_ENABLE_DTLS)
    int res = credman_add(&credential);
    if (res < 0 && res != CREDMAN_EXIST)
    {
        /* ignore duplicate credentials */
        printf("Error cannot add credential to system: %d\n", (int)res);
        return -1;
    }

    printf("Connection secured with DTLS\n");
    printf("Free DTLS session slots: %d/%d\n", dsm_get_num_available_slots(),
           dsm_get_num_maximum_slots());
#endif

    printf("CONFIG_RUNS_COUNT: %i\n", CONFIG_RUNS_COUNT);
    printf("CONFIG_BYTES_COUNT: %i\n", CONFIG_BYTES_COUNT);

    size_t runs = CONFIG_RUNS_COUNT;
    size_t payload_length = CONFIG_BYTES_COUNT;

    benchmark_time_sum = 0;
    rtt_replies_count = 0;

    gcoap_test0(server_addr_str, payload_length, runs);

    // wait for the "rest" of the replies
    do
    {
        printf("rtt_replies_count: %i\n", rtt_replies_count);
        usleep(1000 * 1000);
    } while (rtt_replies_count < runs - (runs / 10));

    benchmark_print_time(benchmark_time_sum, rtt_replies_count, "coap_rtt");

    // clean output for the analysis script
    fprintf(stderr, "rtt_replies_count benchmark_time_sum\n%i %i", rtt_replies_count, benchmark_time_sum);

    exit(0);
    return 0;
}
