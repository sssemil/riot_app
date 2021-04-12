#!/bin/bash

rm results.csv
echo "INPUT	CON_SUM	ACK_SUM	CON_COUNT	ACK_COUNT	DIFF	AVR	ACK_DELTA_AVR" >> results.csv

./stats.sh 10000_runs-4-bytes-coap-oscore 105 93
./stats.sh 10000_runs-8-bytes-coap-oscore 110 97
./stats.sh 10000_runs-16-bytes-coap-oscore 118 105
./stats.sh 10000_runs-32-bytes-coap-oscore 134 121
./stats.sh 10000_runs-64-bytes-coap-oscore 166 153