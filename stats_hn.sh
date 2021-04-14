#!/bin/bash

#set -x

INPUT=$1

CON_LEN=$2
ACK_LEN=$3

CON_LEN_UPPER=$(echo "($CON_LEN+8)"| bc -l)
ACK_LEN_UPPER=$(echo "($ACK_LEN+8)"| bc -l)

#(frame.len == 129 and dtls.record.content_type == 22) or (frame.len == 77 and dtls.record.content_type == 21)

tshark -r ${INPUT}.pcapng -Y "(dtls.handshake.type == 1 and frame.len == 129) or (dtls.record.content_type == 23 and (frame.len >= $CON_LEN and frame.len <= $CON_LEN_UPPER))" -w tmp.pcap -F pcap
tshark -r tmp.pcap -Y "(dtls.record.content_type == 23 and (frame.len >= $CON_LEN and frame.len <= $CON_LEN_UPPER))" -T fields -e frame.time_delta > ${INPUT}_ACK_DELTA.csv

ACK_DELTA_COUNT_ARR=(`wc -l ${INPUT}_ACK_DELTA.csv`)
ACK_COUNT=${ACK_DELTA_COUNT_ARR[0]}
ACK_COUNT=$(echo "($ACK_COUNT - 1)"| bc -l)
ACK_DELTA_COUNT=$ACK_COUNT

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