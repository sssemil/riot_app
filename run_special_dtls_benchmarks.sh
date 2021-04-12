#!/bin/bash

echo "GCOAP_ENABLE_DTLS	CONFIG_RUNS_COUNT	CONFIG_BYTES_COUNT	RTT_REPLIES_COUNT	BENCHMARK_TIME_SUM	FRAME_LEN_SUM" > special_benchmark_logs.csv

CONFIG_RUNS_COUNT=1000

GCOAP_ENABLE_DTLS=1 CONFIG_RUNS_COUNT=$CONFIG_RUNS_COUNT CONFIG_BYTES_COUNT=4 GCOAP_ENABLE_DTLS_HS=1 ./tshark_capture_dtls_hs.sh

#######################################

rm special_results.csv
echo "INPUT	CON_SUM	ACK_SUM	CON_COUNT	ACK_COUNT	DIFF	AVR	ACK_DELTA_AVR" >> special_results.csv

./stats_hn.sh ${CONFIG_RUNS_COUNT}_runs-4-bytes-coap-dtls_1 113 103