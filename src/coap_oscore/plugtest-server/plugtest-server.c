#include "plugtest-server.h"
#include "intermediate-integration.h"
#include <oscore/protection.h>

ssize_t plugtest_nonoscore_hello(coap_pkt_t *pdu, uint8_t *buf, size_t len, void *ctx)
{
    (void)ctx;

    static const char echo_response[] = "Hello World1!";

    gcoap_resp_init(pdu, buf, len, COAP_CODE_CONTENT);
    coap_opt_add_format(pdu, COAP_FORMAT_TEXT);
    size_t resp_len = coap_opt_finish(pdu, COAP_OPT_FINISH_PAYLOAD);
    memcpy(pdu->payload, echo_response, strlen(echo_response));
    resp_len += strlen(echo_response);

    return resp_len;
}

/** Return true only if exactly the options present in the messages have their
 * respective bit set in expected_options, and dump the options on stdout in
 * either case. (expected_options can't be -1). */
static bool options_are_as_expected(oscore_msg_protected_t *msg, uint64_t expected_options)
{
    uint64_t seen = 0;

    oscore_msg_protected_optiter_t iter;
    uint16_t opt_num;
    const uint8_t *opt_val;
    size_t opt_len;
    oscore_msg_protected_optiter_init(msg, &iter);
    while (oscore_msg_protected_optiter_next(msg, &iter, &opt_num, &opt_val, &opt_len))
    {
        printf("Checking option %d: \"", opt_num);
        for (size_t j = 0; j < opt_len; ++j)
        {
            if (opt_val[j] >= 32 && opt_val[j] < 127)
            {
                printf("%c", opt_val[j]);
            }
            else
            {
                printf("\\x%02x", opt_val[j]);
            }
        }
        printf("\"\n");

        if (opt_num >= 64)
        {
            printf("Option was high and unexpected\n");
            seen = -1;
        }
        else
        {
            if (((1 << opt_num) & expected_options) == 0)
            {
                printf("Option was unexpected\n");
            }
            seen |= 1 << opt_num;
        }
    }
    return !oscore_msgerr_protected_is_error(oscore_msg_protected_optiter_finish(msg, &iter)) && seen == expected_options;
}

/*static ssize_t _riot_test0_handler(coap_pkt_t *pdu, uint8_t *buf, size_t len, void *ctx)
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
    pdu->payload[payload_len - 1] = 0;
    payload_uint32_ptr = (uint32_t *)pdu->payload;
    *payload_uint32_ptr = departure_time;

    return resp_len + payload_len;
}*/

void test0_parse(oscore_msg_protected_t *in, void *vstate)
{
    struct test0_state *state = vstate;
    //state->options_ok = options_are_as_expected(in, 1 << 11 /* Uri-Path */);
    //state->code_ok = oscore_msg_protected_get_code(in) == 1 /* GET */;

    size_t payload_length;
    uint8_t *payload = get_message(in, &payload_length);
    uint32_t *payload_uint32_ptr = (uint32_t *)payload;
    state->departure_time = *payload_uint32_ptr;
    state->payload_length = payload_length;
}

void test0_build(oscore_msg_protected_t *out, const void *vstate, const struct observe_option *outer_observe)
{
    (void)outer_observe;
    const struct test0_state *state = vstate;

    /*if (!state->code_ok)
    {
        oscore_msg_protected_set_code(out, 0x85); // 4.05 Method Not Allowed
        goto error2;
    }*/

    oscore_msg_protected_set_code(out, 0x45 /* 2.05 content */);

    oscore_msgerr_protected_t err = oscore_msg_protected_append_option(out, 12 /* content-format */, (uint8_t *)"", 0);
    if (oscore_msgerr_protected_is_error(err))
        goto error;

    uint8_t *payload;
    size_t payload_length;
    oscore_msgerr_native_t err2 = oscore_msg_protected_map_payload(out, &payload, &payload_length);
    if (oscore_msgerr_protected_is_error(err2))
    {
        goto error;
    }

    memset(payload, 'a', state->payload_length);
    uint32_t *payload_uint32_ptr = (uint32_t *)payload;
    *payload_uint32_ptr = state->departure_time;

    err2 = oscore_msg_protected_trim_payload(out, state->payload_length);
    if (oscore_msgerr_protected_is_error(err2))
    {
        goto error;
    }

    return;

error:
    oscore_msg_protected_set_code(out, 0xa0 /* 5.00 internal error */);
    //error2:
    // not unwinding any option, there's no API for that and it doesn't really matter
    //oscore_msg_protected_trim_payload(out, 0);
}

void hello_parse(oscore_msg_protected_t *in, void *vstate)
{
    struct hello_state *state = vstate;
    state->options_ok = options_are_as_expected(in, 1 << 11 /* Uri-Path */);
    state->code_ok = oscore_msg_protected_get_code(in) == 1 /* GET */;
}

void hello_build(oscore_msg_protected_t *out, const void *vstate, const struct observe_option *outer_observe)
{
    (void)outer_observe;
    const struct hello_state *state = vstate;

    if (!state->code_ok)
    {
        oscore_msg_protected_set_code(out, 0x85 /* 4.05 Method Not Allowed */);
        goto error2;
    }

    oscore_msg_protected_set_code(out, 0x45 /* 2.05 content */);

    oscore_msgerr_protected_t err = oscore_msg_protected_append_option(out, 12 /* content-format */, (uint8_t *)"", 0);
    if (oscore_msgerr_protected_is_error(err))
        goto error;

    if (!set_message(out, state->options_ok ? "Hello World2!" : "Hello Unexpected!"))
        goto error;

    return;

error:
    oscore_msg_protected_set_code(out, 0xa0 /* 5.00 internal error */);
error2:
    // not unwinding any option, there's no API for that and it doesn't really matter
    oscore_msg_protected_trim_payload(out, 0);
}

bool observation_id_valid = false;
oscore_requestid_t observation_id;

void observe1_parse(oscore_msg_protected_t *in, void *vstate)
{
    struct hello_state *state = vstate;
    state->options_ok = options_are_as_expected(in, (1 << 11 /* Uri-Path */) |
                                                        (1 << 6 /* Observe */));
    state->code_ok = oscore_msg_protected_get_code(in) == 1 /* GET */;
}

void observe1_build(oscore_msg_protected_t *out, const void *vstate, const struct observe_option *outer_observe)
{
    const struct hello_state *state = vstate;

    /* We're *not* storing this conditionally on whether this is an observation
     * -- for anything to make actual sense here, that'd need to allow for
     *  multiple concurrent observations in Gcoap anyway.
     *
     * What we *do* need to test very carefully is that this is only stored
     * with context B -- as that's the one hard-coded in cmdlin_notify. A more
     * practical implementation would store the context reference along with
     * the key -- but then it'd need to invalidate the observations at the time
     * of (or just prevent) modifications to the context, and eg. for the user
     * context that's not easily possible without making it unmodifiable
     * forever (given we're not informed of the end of observation).
     */
    if (out->secctx == &secctx_b)
    {
        oscore_requestid_clone(&observation_id, &out->request_id);
        observation_id_valid = true;
    }

    if (!state->code_ok)
    {
        oscore_msg_protected_set_code(out, 0x85 /* 4.05 Method Not Allowed */);
        goto error2;
    }

    oscore_msg_protected_set_code(out, 0x45 /* 2.05 content */);

    oscore_msgerr_protected_t err;

    if (outer_observe->length >= 0)
    {
        err = oscore_msg_protected_append_option(out, 6 /* observe */, outer_observe->data, outer_observe->length);
        if (oscore_msgerr_protected_is_error(err))
            goto error;
    }

    err = oscore_msg_protected_append_option(out, 12 /* content-format */, (uint8_t *)"", 0);
    if (oscore_msgerr_protected_is_error(err))
        goto error;

    if (!set_message(out, state->options_ok ? "one" : "one (with unexpected options)"))
        goto error;

    return;

error:
    oscore_msg_protected_set_code(out, 0xa0 /* 5.00 internal error */);
error2:
    // not unwinding any option, there's no API for that and it doesn't really matter
    oscore_msg_protected_trim_payload(out, 0);
}

void hello2_parse(oscore_msg_protected_t *in, void *vstate)
{
    struct hello_state *state = vstate;
    state->options_ok = options_are_as_expected(in, (1 << 11 /* Uri-Path */) | (1 << 15 /* Uri-Query */));
    state->code_ok = oscore_msg_protected_get_code(in) == 1 /* GET */;
}

void hello2_build(oscore_msg_protected_t *out, const void *vstate, const struct observe_option *outer_observe)
{
    (void)outer_observe;
    const struct hello_state *state = vstate;

    if (!state->code_ok)
    {
        oscore_msg_protected_set_code(out, 0x85 /* 4.05 Method Not Allowed */);
        goto error2;
    }

    oscore_msg_protected_set_code(out, 0x45 /* 2.05 content */);

    oscore_msgerr_protected_t err;
    err = oscore_msg_protected_append_option(out, 4 /* ETag */, (uint8_t *)"\x2b", 1);
    if (oscore_msgerr_protected_is_error(err))
        goto error;

    err = oscore_msg_protected_append_option(out, 12 /* content-format */, (uint8_t *)"", 0);
    if (oscore_msgerr_protected_is_error(err))
        goto error;

    if (!set_message(out, state->options_ok ? "Hello World3!" : "Hello Unexpected!"))
        goto error;

    return;

error:
    oscore_msg_protected_set_code(out, 0xa0 /* 5.00 internal error */);
error2:
    // not unwinding any option, there's no API for that and it doesn't really matter
    oscore_msg_protected_trim_payload(out, 0);
}

void hello3_parse(oscore_msg_protected_t *in, void *vstate)
{
    struct hello_state *state = vstate;
    state->options_ok = options_are_as_expected(in, (1 << 11 /* Uri-Path */) | (1 << 17 /* Accept */));
    state->code_ok = oscore_msg_protected_get_code(in) == 1 /* GET */;
}

void hello3_build(oscore_msg_protected_t *out, const void *vstate, const struct observe_option *outer_observe)
{
    (void)outer_observe;
    const struct hello_state *state = vstate;

    if (!state->code_ok)
    {
        oscore_msg_protected_set_code(out, 0x85 /* 4.05 Method Not Allowed */);
        goto error2;
    }

    oscore_msg_protected_set_code(out, 0x45 /* 2.05 content */);

    oscore_msgerr_protected_t err;
    err = oscore_msg_protected_append_option(out, 12 /* content-format */, (uint8_t *)"", 0);
    if (oscore_msgerr_protected_is_error(err))
        goto error;

    err = oscore_msg_protected_append_option(out, 14 /* Max-Age */, (uint8_t *)"\x05", 1);
    if (oscore_msgerr_protected_is_error(err))
        goto error;

    if (!set_message(out, state->options_ok ? "Hello World4!" : "Hello Unexpected!"))
        goto error;

    return;

error:
    oscore_msg_protected_set_code(out, 0xa0 /* 5.00 internal error */);
error2:
    // not unwinding any option, there's no API for that and it doesn't really matter
    oscore_msg_protected_trim_payload(out, 0);
}

void hello6_parse(oscore_msg_protected_t *in, void *vstate)
{
    struct hello_state *state = vstate;
    state->options_ok = options_are_as_expected(in, (1 << 11 /* Uri-Path */) | (1 << 12 /* Content-Format */));
    state->code_ok = oscore_msg_protected_get_code(in) == 2 /* POST */;
    /* FIXME check payload */
}

void hello6_build(oscore_msg_protected_t *out, const void *vstate, const struct observe_option *outer_observe)
{
    (void)outer_observe;
    const struct hello_state *state = vstate;

    if (!state->code_ok)
    {
        oscore_msg_protected_set_code(out, 0x85 /* 4.05 Method Not Allowed */);
        goto error2;
    }

    oscore_msg_protected_set_code(out, 0x44 /* 2.04 Changed */);

    oscore_msgerr_protected_t err;
    err = oscore_msg_protected_append_option(out, 12 /* content-format */, (uint8_t *)"", 0);
    if (oscore_msgerr_protected_is_error(err))
        goto error;

    if (!set_message(out, state->options_ok ? "\x4a" : "Would have been 0x4a if options matched"))
        goto error;

    return;

error:
    oscore_msg_protected_set_code(out, 0xa0 /* 5.00 internal error */);
error2:
    // not unwinding any option, there's no API for that and it doesn't really matter
    oscore_msg_protected_trim_payload(out, 0);
}

void hello7_parse(oscore_msg_protected_t *in, void *vstate)
{
    struct hello_state *state = vstate;
    state->options_ok = options_are_as_expected(in, (1 << 11 /* Uri-Path */) | (1 << 12 /* Content-Format */) | (1 << 1 /* If-Match */));
    state->code_ok = oscore_msg_protected_get_code(in) == 3 /* PUT */;
    /* FIXME check payload */
}

void hello7_build(oscore_msg_protected_t *out, const void *vstate, const struct observe_option *outer_observe)
{
    (void)outer_observe;
    const struct hello_state *state = vstate;

    if (!state->code_ok)
    {
        oscore_msg_protected_set_code(out, 0x85 /* 4.05 Method Not Allowed */);
    }
    else if (state->options_ok)
    {
        // For test 9
        oscore_msg_protected_set_code(out, 0x44 /* 2.04 Changed */);
    }
    else
    {
        // For test 10 -- FIXME this could be sharper, but hey it's enough to
        // pass, and we're not checking every detail of the requests anyway
        oscore_msg_protected_set_code(out, 0x8c /* 4.12 Precondition Failed */);
    }

    oscore_msg_protected_trim_payload(out, 0);
}

void delete_parse(oscore_msg_protected_t *in, void *vstate)
{
    struct hello_state *state = vstate;
    state->options_ok = options_are_as_expected(in, (1 << 11 /* Uri-Path */));
    state->code_ok = oscore_msg_protected_get_code(in) == 4 /* DELETE */;
}

void delete_build(oscore_msg_protected_t *out, const void *vstate, const struct observe_option *outer_observe)
{
    (void)outer_observe;
    const struct hello_state *state = vstate;

    if (!state->code_ok)
    {
        oscore_msg_protected_set_code(out, 0x85 /* 4.05 Method Not Allowed */);
    }
    else if (state->options_ok)
    {
        // For test 9
        oscore_msg_protected_set_code(out, 0x42 /* 2.02 Deleted */);
    }
    else
    {
        oscore_msg_protected_set_code(out, 0x80 /* 4.00 Bad Request */);
    }

    oscore_msg_protected_trim_payload(out, 0);
}
