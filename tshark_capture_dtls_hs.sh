#!/bin/bash

#set -o xtrace

./setup_iface.sh

# make sure that no other instances are running
while killall -s SIGKILL default.elf; do
    echo "killing other RIOT-OS native instances..."
done
while killall -s SIGKILL test_plugtest_server.elf; do
    echo "killing other RIOT-OS native instances..."
done

# enable DTLS by default
GCOAP_ENABLE_DTLS=${GCOAP_ENABLE_DTLS:-1}
echo "GCOAP_ENABLE_DTLS: $GCOAP_ENABLE_DTLS"

# configure benchmark
CONFIG_RUNS_COUNT=${CONFIG_RUNS_COUNT:-10000}
CONFIG_BYTES_COUNT=${CONFIG_BYTES_COUNT:-4}
echo "CONFIG_RUNS_COUNT: $CONFIG_RUNS_COUNT"
echo "CONFIG_BYTES_COUNT: $CONFIG_BYTES_COUNT"

# build
cd src/coap/client
../../../conditional_clean.sh
CONFIG_RUNS_COUNT=$CONFIG_RUNS_COUNT CONFIG_BYTES_COUNT=$CONFIG_BYTES_COUNT make all -j$(nproc)
cd ../../../

cd src/coap/server
../../../conditional_clean.sh
make all -j$(nproc)
cd ../../../

# capture
tshark -i tapbr0 -w ${CONFIG_RUNS_COUNT}_runs-${CONFIG_BYTES_COUNT}-bytes-coap-dtls_${GCOAP_ENABLE_DTLS}.pcapng &
tshark_pid=$!

# start server
cd src/coap/server
(PORT=tap0 make term) &
cd ../../../

# wait for run
sleep 2

# start client
cd src/coap/client

{
    IFS=$'\n' read -r -d '' CLIENT_OUTPUT
    IFS=$'\n' read -r -d '' CLIENT_OUTPUT_STDOUT
} < <((printf '\0%s\0' "$(PORT=tap1 make term)" 1>&2) 2>&1)

echo $CLIENT_OUTPUT_STDOUT
echo $CLIENT_OUTPUT

cd ../../../

# get pids
server_pid=$(pidof default.elf)

# stop everything
kill $tshark_pid
kill $server_pid

# get result out
sleep 5

BENCHMARK_RESULTS_ARR=($CLIENT_OUTPUT)
RTT_REPLIES_COUNT=${BENCHMARK_RESULTS_ARR[2]}
BENCHMARK_TIME_SUM=${BENCHMARK_RESULTS_ARR[3]}

# analyse tshark capture
FRAME_LEN_SUM=$(tshark -r ${CONFIG_RUNS_COUNT}_runs-${CONFIG_BYTES_COUNT}-bytes-coap-dtls_${GCOAP_ENABLE_DTLS}.pcapng -z io,stat,0,"SUM(frame.len)frame.len && not icmpv6 and (dtls.record.content_type == 22 or dtls.record.content_type == 20) and frame.len != 93" -q | tail -2 | head -1 | cut -d'|' -f 3)

# print results
cat << EOF
================================================================
GCOAP_ENABLE_DTLS: $GCOAP_ENABLE_DTLS
CONFIG_RUNS_COUNT: $CONFIG_RUNS_COUNT
CONFIG_BYTES_COUNT: $CONFIG_BYTES_COUNT
RTT_REPLIES_COUNT: $RTT_REPLIES_COUNT
BENCHMARK_TIME_SUM: $BENCHMARK_TIME_SUM
FRAME_LEN_SUM: $FRAME_LEN_SUM
================================================================
EOF

cat >> benchmark_logs.txt << EOF
================================================================
GCOAP_ENABLE_DTLS: $GCOAP_ENABLE_DTLS
CONFIG_RUNS_COUNT: $CONFIG_RUNS_COUNT
CONFIG_BYTES_COUNT: $CONFIG_BYTES_COUNT
RTT_REPLIES_COUNT: $RTT_REPLIES_COUNT
BENCHMARK_TIME_SUM: $BENCHMARK_TIME_SUM
FRAME_LEN_SUM: $FRAME_LEN_SUM
================================================================
EOF

cat >> special_benchmark_logs.csv << EOF
$GCOAP_ENABLE_DTLS	$CONFIG_RUNS_COUNT	$CONFIG_BYTES_COUNT	$RTT_REPLIES_COUNT	$BENCHMARK_TIME_SUM	$FRAME_LEN_SUM
EOF

