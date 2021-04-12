#!/bin/bash

rm results.csv
echo "INPUT	CON_SUM	ACK_SUM	CON_COUNT	ACK_COUNT	DIFF	AVR	ACK_DELTA_AVR" >> results.csv

PIDS=""

./stats.sh 10000_runs-4-bytes-coap-dtls_1 115 103&
PIDS="$PIDS $!"

./stats.sh 10000_runs-8-bytes-coap-dtls_1 119 107&
PIDS="$PIDS $!"

./stats.sh 10000_runs-16-bytes-coap-dtls_1 127 115&
PIDS="$PIDS $!"

./stats.sh 10000_runs-32-bytes-coap-dtls_1 143 131&
PIDS="$PIDS $!"

./stats.sh 10000_runs-64-bytes-coap-dtls_1 175 163&
PIDS="$PIDS $!"

echo $PIDS

wait $PIDS