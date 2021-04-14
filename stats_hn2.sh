#!/bin/bash

#set -x

INPUT=$1

CON_LEN=$2
ACK_LEN=$3

CON_LEN_UPPER=$(echo "($CON_LEN+8)"| bc -l)
ACK_LEN_UPPER=$(echo "($ACK_LEN+8)"| bc -l)

tshark -r ${INPUT}.pcapng -Y "not icmpv6 and (frame.len >= $CON_LEN and frame.len <= $CON_LEN_UPPER)" -T fields -e frame.time_relative > ${INPUT}_CON.csv&
CON_PID=$!
tshark -r ${INPUT}.pcapng -Y "not icmpv6 and (frame.len >= $ACK_LEN and frame.len <= $ACK_LEN_UPPER)" -T fields -e frame.time_relative > ${INPUT}_ACK.csv&
ACK_PID=$!
tshark -r ${INPUT}.pcapng -Y "not icmpv6 and (frame.len >= $ACK_LEN and frame.len <= $ACK_LEN_UPPER)" -T fields -e frame.time_delta > ${INPUT}_ACK_DELTA.csv&
DELTA_PID=$!

wait $CON_PID $ACK_PID

CON_COUNT_ARR=(`wc -l ${INPUT}_CON.csv`)
ACK_COUNT_ARR=(`wc -l ${INPUT}_ACK.csv`)
CON_COUNT=${CON_COUNT_ARR[0]}
ACK_COUNT=${ACK_COUNT_ARR[0]}
CON_COUNT=$(echo "($CON_COUNT - 1)"| bc -l)
ACK_COUNT=$(echo "($ACK_COUNT - 1)"| bc -l)
ACK_DELTA_COUNT=$ACK_COUNT
MIN_COUNT=$(( CON_COUNT < ACK_COUNT ? CON_COUNT : ACK_COUNT ))

head -${MIN_COUNT} ${INPUT}_CON.csv > ${INPUT}_CON_min.csv
head -${MIN_COUNT} ${INPUT}_ACK.csv > ${INPUT}_ACK_min.csv
CON_COUNT=$MIN_COUNT
ACK_COUNT=$MIN_COUNT

CON_SUM=`paste -sd+ ${INPUT}_CON_min.csv | bc`
ACK_SUM=`paste -sd+ ${INPUT}_ACK_min.csv | bc`
ACK_DELTA_SUM=`paste -sd+ ${INPUT}_ACK_DELTA.csv | bc`


DIFF=$(echo "($ACK_SUM-$CON_SUM)"| bc -l)
AVR=$(echo "(1000000*2*$DIFF/($ACK_COUNT+$CON_COUNT))"| bc -l)
ACK_DELTA_AVR=$(echo "($ACK_DELTA_SUM/$ACK_DELTA_COUNT)"| bc -l)

echo "INPUT: $INPUT"
echo "CON_SUM: $CON_SUM"
echo "ACK_SUM: $ACK_SUM"
echo "CON_COUNT: $CON_COUNT"
echo "ACK_COUNT: $ACK_COUNT"
echo "DIFF: $DIFF"
echo "AVR: $AVR microseconds"
echo "ACK_DELTA_AVR: $ACK_DELTA_AVR"

echo "$INPUT	$CON_SUM	$ACK_SUM	$CON_COUNT	$ACK_COUNT	$DIFF	$AVR	$ACK_DELTA_AVR" >> special_results.csv