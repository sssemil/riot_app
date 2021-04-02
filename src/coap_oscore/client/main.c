#include <stdio.h>
#include <unistd.h>

#include <oscore_native/message.h>
#include <oscore/message.h>
#include <oscore/contextpair.h>
#include <oscore/context_impl/primitive.h>
#include <oscore/protection.h>

#include "msg.h"

#include "net/gcoap.h"
#include "net/sock.h"
#include "net/netif.h"
#include "od.h"
#include "benchmark.h"

#ifndef VERBOSE
#define VERBOSE 1
#endif

#ifndef CONFIG_RUNS_COUNT
#define CONFIG_RUNS_COUNT 1
#endif

#ifndef CONFIG_BYTES_COUNT
#define CONFIG_BYTES_COUNT 4
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
#define B_SENDER_KEY                                                                                                                                     \
    {                                                                                                                                                    \
        50, 136, 42, 28, 97, 144, 48, 132, 56, 236, 152, 230, 169, 50, 240, 32, 112, 143, 55, 57, 223, 228, 109, 119, 152, 155, 3, 155, 31, 252, 28, 172 \
    }
#define B_RECIPIENT_KEY                                                                                                                              \
    {                                                                                                                                                \
        213, 48, 30, 177, 141, 6, 120, 73, 149, 8, 147, 186, 42, 200, 145, 65, 124, 137, 174, 9, 223, 74, 56, 85, 170, 0, 10, 201, 255, 243, 135, 81 \
    }
#define B_COMMON_IV                                        \
    {                                                      \
        100, 240, 189, 49, 77, 75, 224, 60, 39, 12, 43, 28 \
    }

#define D_SENDER_KEY                                                                                                                                       \
    {                                                                                                                                                      \
        79, 177, 204, 118, 107, 180, 59, 3, 14, 118, 123, 233, 14, 12, 59, 241, 144, 219, 242, 68, 113, 65, 139, 251, 152, 212, 46, 145, 230, 180, 76, 252 \
    }
#define D_RECIPIENT_KEY                                                                                                                             \
    {                                                                                                                                               \
        173, 139, 170, 28, 148, 232, 23, 226, 149, 11, 247, 99, 61, 79, 20, 148, 10, 6, 12, 149, 135, 5, 18, 168, 164, 11, 216, 42, 13, 221, 69, 39 \
    }
#define D_COMMON_IV                                           \
    {                                                         \
        199, 178, 145, 95, 47, 133, 49, 117, 132, 37, 73, 212 \
    }

static struct oscore_context_primitive_immutables prim_imm_b = {
    .common_iv = B_COMMON_IV,

    //.recipient_id = "\xf1",
    .recipient_id_len = 0,
    .recipient_key = B_RECIPIENT_KEY,

    .sender_id_len = 1,
    .sender_id = "\x01",
    .sender_key = B_SENDER_KEY,

    .aeadalg = COSE_ALGO_CHACHA20POLY1305,
};
static struct oscore_context_primitive prim_b = {
    .immutables = &prim_imm_b,
};
oscore_context_t secc_b = {
    .type = OSCORE_CONTEXT_PRIMITIVE,
    .data = (void *)(&prim_b),
};

static struct oscore_context_primitive_immutables prim_imm_d = {
    .common_iv = D_COMMON_IV,

    //.recipient_id = "\x01",
    .recipient_id_len = 0,
    .recipient_key = D_RECIPIENT_KEY,

    .sender_id_len = 1,
    .sender_id = "\x01",
    .sender_key = D_SENDER_KEY,

    .aeadalg = COSE_ALGO_CHACHA20POLY1305,
};
static struct oscore_context_primitive prim_d = {
    .immutables = &prim_imm_d,
};
oscore_context_t secc_d = {
    .type = OSCORE_CONTEXT_PRIMITIVE,
    .data = (void *)(&prim_d),
};

int gcoap_test0(char *server_addr_str, size_t payload_length, size_t runs)
{
    (void)payload_length;
    (void)runs;

    oscore_msg_protected_t oscmsg;
    oscore_requestid_t request_id;

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
    //char path[] = "/.well-known/core";
    char path[] = "/riot/test0";

    // set prev path for any path
    size_t path_len = strlen(path);
    memset(_last_req_path, 0, _LAST_REQ_PATH_MAX);
    if (path_len < _LAST_REQ_PATH_MAX)
    {
        memcpy(_last_req_path, path, path_len);
    }
    int err;
    err = gcoap_req_init(&pdu, &buf[0], GCOAP_PDU_BUF_SIZE,
                         COAP_METHOD_POST, NULL);
    if (err != 0)
    {
        printf("Failed to initialize request\n");
        return -1;
    }

    coap_hdr_set_type(pdu.hdr, COAP_TYPE_CON);

    oscore_msg_native_t native = {.pkt = &pdu};

    oscore_crypto_aeadalg_t aeadalg = oscore_context_get_aeadalg(&secc_b);
    size_t tag_length = oscore_crypto_aead_get_taglength(aeadalg);
    printf("aeadalg tag_length: %i\n", tag_length);
    printf("aeadalg: %i\n", aeadalg);

    if (oscmsg.flags & OSCORE_MSG_PROTECTED_FLAG_REQUEST)
        printf("is_request: true\n");
    else
        printf("is_request: false\n");

    if (oscore_prepare_request(native, &oscmsg, &secc_b, &request_id) != OSCORE_PREPARE_OK)
    {
        printf("Failed to prepare request encryption\n");
        return -1;
    }

    if (oscmsg.flags & OSCORE_MSG_PROTECTED_FLAG_REQUEST)
        printf("is_request: true\n");
    else
        printf("is_request: false\n");

    oscore_msg_protected_set_code(&oscmsg, COAP_GET);

    oscore_msgerr_protected_t oscerr;
    oscerr = oscore_msg_protected_append_option(&oscmsg, COAP_OPT_URI_PATH, (uint8_t *)"temperature", 11);
    if (oscore_msgerr_protected_is_error(oscerr))
    {
        printf("error;Failed to add option\n");
        return -1;
    }

    //uint8_t opt_val[2] = {0x09};
    //coap_opt_add_opaque(&pdu, 9, opt_val, 1);

    uint8_t *_payload;
    size_t _payload_length;
    oscerr = oscore_msg_protected_map_payload(&oscmsg, &_payload, &_payload_length);
    //=======================================================================
    printf("pdu.options_len: %i\n", pdu.options_len);
    for (int i = 0; i < pdu.options_len; i++)
    {
        printf("pdu.options[%i].opt_num: %i\n", i, pdu.options[i].opt_num);
        printf("pdu.options[%i].offset: %i\n", i, pdu.options[i].offset);
    }
    //=======================================================================

    if (oscore_msgerr_protected_is_error(oscerr))
    {
        printf("Failed to map payload\n");
        return -1;
    }
    printf("_payload_length: %i\n", _payload_length);

    memset(_payload, 'a', payload_length);
    _payload[payload_length - 1] = 0;

    oscerr = oscore_msg_protected_trim_payload(&oscmsg, payload_length);
    if (oscore_msgerr_protected_is_error(oscerr))
    {
        printf("error;Failed to truncate payload\n");
        return -1;
    }

    oscore_msg_native_t pdu_write_out;
    int res = oscore_encrypt_message(&oscmsg, &pdu_write_out);
    if (res != OSCORE_FINISH_OK)
    {
        printf("error;Failed to encrypt message: %i, %i\n", res, oscmsg.tag_length);
        return -1;
    }

    //request_id.sctx = (void *)secc;

    printf("request_id.used_bytes: %i\n", request_id.used_bytes);
    buf[6] = 0x92;

    if (oscmsg.flags & OSCORE_MSG_PROTECTED_FLAG_REQUEST)
        printf("is_request: true\n");
    else
        printf("is_request: false\n");

    int bytes_sent = gcoap_req_send(buf, pdu.payload - (uint8_t *)pdu.hdr + pdu.payload_len, remote, NULL, &request_id);
    if (bytes_sent <= 0)
    {
        printf("error;sending\n");
        return -1;
    }

    return 0;
}

int main(void)
{
    printf("======================================================================\n");
    char *server_addr_str = "fe80::200:ff:fe00:ab";

    printf("CONFIG_RUNS_COUNT: %i\n", CONFIG_RUNS_COUNT);
    printf("CONFIG_BYTES_COUNT: %i\n", CONFIG_BYTES_COUNT);

    size_t runs = CONFIG_RUNS_COUNT;
    size_t payload_length = CONFIG_BYTES_COUNT;

    gcoap_test0(server_addr_str, payload_length, runs);

    usleep(1000 * 1000);

    return 0;
}
