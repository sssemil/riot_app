#!/bin/bash

echo "GCOAP_ENABLE_DTLS	CONFIG_RUNS_COUNT	CONFIG_BYTES_COUNT	RTT_REPLIES_COUNT	BENCHMARK_TIME_SUM	FRAME_LEN_SUM" > benchmark_logs.csv

CONFIG_RUNS_COUNT=1000000

GCOAP_ENABLE_DTLS=0 CONFIG_RUNS_COUNT=$CONFIG_RUNS_COUNT CONFIG_BYTES_COUNT=4 ./tshark_capture.sh
GCOAP_ENABLE_DTLS=0 CONFIG_RUNS_COUNT=$CONFIG_RUNS_COUNT CONFIG_BYTES_COUNT=8 ./tshark_capture.sh
GCOAP_ENABLE_DTLS=0 CONFIG_RUNS_COUNT=$CONFIG_RUNS_COUNT CONFIG_BYTES_COUNT=16 ./tshark_capture.sh
GCOAP_ENABLE_DTLS=0 CONFIG_RUNS_COUNT=$CONFIG_RUNS_COUNT CONFIG_BYTES_COUNT=32 ./tshark_capture.sh
GCOAP_ENABLE_DTLS=0 CONFIG_RUNS_COUNT=$CONFIG_RUNS_COUNT CONFIG_BYTES_COUNT=64 ./tshark_capture.sh

GCOAP_ENABLE_DTLS=1 CONFIG_RUNS_COUNT=$CONFIG_RUNS_COUNT CONFIG_BYTES_COUNT=4 ./tshark_capture.sh
GCOAP_ENABLE_DTLS=1 CONFIG_RUNS_COUNT=$CONFIG_RUNS_COUNT CONFIG_BYTES_COUNT=8 ./tshark_capture.sh
GCOAP_ENABLE_DTLS=1 CONFIG_RUNS_COUNT=$CONFIG_RUNS_COUNT CONFIG_BYTES_COUNT=16 ./tshark_capture.sh
GCOAP_ENABLE_DTLS=1 CONFIG_RUNS_COUNT=$CONFIG_RUNS_COUNT CONFIG_BYTES_COUNT=32 ./tshark_capture.sh
GCOAP_ENABLE_DTLS=1 CONFIG_RUNS_COUNT=$CONFIG_RUNS_COUNT CONFIG_BYTES_COUNT=64 ./tshark_capture.sh

CONFIG_RUNS_COUNT=$CONFIG_RUNS_COUNT CONFIG_BYTES_COUNT=4 ./tshark_capture_oscore.sh
CONFIG_RUNS_COUNT=$CONFIG_RUNS_COUNT CONFIG_BYTES_COUNT=8 ./tshark_capture_oscore.sh
CONFIG_RUNS_COUNT=$CONFIG_RUNS_COUNT CONFIG_BYTES_COUNT=16 ./tshark_capture_oscore.sh
CONFIG_RUNS_COUNT=$CONFIG_RUNS_COUNT CONFIG_BYTES_COUNT=32 ./tshark_capture_oscore.sh
CONFIG_RUNS_COUNT=$CONFIG_RUNS_COUNT CONFIG_BYTES_COUNT=64 ./tshark_capture_oscore.sh

#######################################

rm results.csv
echo "INPUT	CON_SUM	ACK_SUM	CON_COUNT	ACK_COUNT	DIFF	AVR	ACK_DELTA_AVR" >> results.csv

./stats.sh ${CONFIG_RUNS_COUNT}_runs-4-bytes-coap-dtls_0 84 73
./stats.sh ${CONFIG_RUNS_COUNT}_runs-8-bytes-coap-dtls_0 88 77
./stats.sh ${CONFIG_RUNS_COUNT}_runs-16-bytes-coap-dtls_0 96 85
./stats.sh ${CONFIG_RUNS_COUNT}_runs-32-bytes-coap-dtls_0 112 101
./stats.sh ${CONFIG_RUNS_COUNT}_runs-64-bytes-coap-dtls_0 144 133

./stats.sh ${CONFIG_RUNS_COUNT}_runs-4-bytes-coap-dtls_1 113 102
./stats.sh ${CONFIG_RUNS_COUNT}_runs-8-bytes-coap-dtls_1 117 106
./stats.sh ${CONFIG_RUNS_COUNT}_runs-16-bytes-coap-dtls_1 125 114
./stats.sh ${CONFIG_RUNS_COUNT}_runs-32-bytes-coap-dtls_1 141 130
./stats.sh ${CONFIG_RUNS_COUNT}_runs-64-bytes-coap-dtls_1 172 162

./stats.sh ${CONFIG_RUNS_COUNT}_runs-4-bytes-coap-oscore 105 93
./stats.sh ${CONFIG_RUNS_COUNT}_runs-8-bytes-coap-oscore 110 97
./stats.sh ${CONFIG_RUNS_COUNT}_runs-16-bytes-coap-oscore 118 105
./stats.sh ${CONFIG_RUNS_COUNT}_runs-32-bytes-coap-oscore 134 121
./stats.sh ${CONFIG_RUNS_COUNT}_runs-64-bytes-coap-oscore 166 153