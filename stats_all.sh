#!/bin/bash

rm results.csv
echo "INPUT	CON_SUM	ACK_SUM	CON_COUNT	ACK_COUNT	DIFF	AVR	ACK_DELTA_AVR" >> results.csv

./stats.sh 10000_runs-4-bytes-coap-dtls_0 86 74
./stats.sh 10000_runs-8-bytes-coap-dtls_0 90 78
./stats.sh 10000_runs-16-bytes-coap-dtls_0 98 86
./stats.sh 10000_runs-32-bytes-coap-dtls_0 114 102
./stats.sh 10000_runs-64-bytes-coap-dtls_0 146 134

./stats.sh 10000_runs-4-bytes-coap-dtls_1 115 103
./stats.sh 10000_runs-8-bytes-coap-dtls_1 119 107
./stats.sh 10000_runs-16-bytes-coap-dtls_1 127 115
./stats.sh 10000_runs-32-bytes-coap-dtls_1 143 131
./stats.sh 10000_runs-64-bytes-coap-dtls_1 175 163