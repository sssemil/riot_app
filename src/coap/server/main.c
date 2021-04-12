#include <stdio.h>
#include <unistd.h>

#include "msg.h"

#include "net/gcoap.h"
#include "net/sock.h"
#include "net/netif.h"
#include "benchmark.h"

#include "tinydtls_keys.h"
#include "net/credman.h"

#ifndef VERBOSE
#define VERBOSE 0
#endif

static ssize_t _riot_test0_handler(coap_pkt_t *pdu, uint8_t *buf, size_t len, void *ctx);
static ssize_t _encode_link(const coap_resource_t *resource, char *buf,
                            size_t maxlen, coap_link_encoder_ctx_t *context);

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

static const coap_resource_t _resources[] = {
    {"/riot/test0", COAP_GET, _riot_test0_handler, NULL},
};

static const char *_link_params[] = {
    NULL};

static gcoap_listener_t _listener = {
    &_resources[0],
    ARRAY_SIZE(_resources),
    _encode_link,
    NULL,
    NULL};

/* Adds link format params to resource list */
static ssize_t _encode_link(const coap_resource_t *resource, char *buf,
                            size_t maxlen, coap_link_encoder_ctx_t *context)
{
    ssize_t res = gcoap_encode_link(resource, buf, maxlen, context);
    if (res > 0)
    {
        if (_link_params[context->link_pos] && (strlen(_link_params[context->link_pos]) < (maxlen - res)))
        {
            if (buf)
            {
                memcpy(buf + res, _link_params[context->link_pos],
                       strlen(_link_params[context->link_pos]));
            }
            return res + strlen(_link_params[context->link_pos]);
        }
    }

    return res;
}

static ssize_t _riot_test0_handler(coap_pkt_t *pdu, uint8_t *buf, size_t len, void *ctx)
{
    (void)ctx;

    uint32_t departure_time;
    uint32_t *payload_uint32_ptr = (uint32_t *)pdu->payload;
    departure_time = *payload_uint32_ptr;
    uint16_t payload_len = pdu->payload_len;

#if VERBOSE
    printf("_riot_test0_handler, departure_time: %i, payload_len: %i\n", departure_time, payload_len);
#endif

    gcoap_resp_init(pdu, buf, len, COAP_CODE_CONTENT);
    coap_opt_add_format(pdu, COAP_FORMAT_TEXT);
    size_t resp_len = coap_opt_finish(pdu, COAP_OPT_FINISH_PAYLOAD);

    memset(pdu->payload, 'a', payload_len);
    //pdu->payload[payload_len - 1] = 0;
    payload_uint32_ptr = (uint32_t *)pdu->payload;
    *payload_uint32_ptr = departure_time;

    return resp_len + payload_len;
}

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
    printf("======================================================================\n");
    netif_t *iface = NULL;

    if ((iface = netif_iter(iface)))
    {
        print_iface(iface);
    }
    else
    {
        printf("gcoap_client: not interface found\n");
        return -1;
    }

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

    gcoap_register_listener(&_listener);

    printf("CoAP server is listening on port %u\n", CONFIG_GCOAP_PORT);

#if IS_ACTIVE(CONFIG_GCOAP_ENABLE_DTLS)
    uint8_t last_slot_count = DTLS_PEER_MAX;
    while (true)
    {
        if (last_slot_count != dsm_get_num_available_slots())
        {
            printf("Free DTLS session slots: %d/%d\n", dsm_get_num_available_slots(),
                   dsm_get_num_maximum_slots());
            last_slot_count = dsm_get_num_available_slots();
        }
        usleep(5000 * 1000);
    }
#endif

    return 0;
}
