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

# disable DTLS by default
GCOAP_ENABLE_DTLS=0

# configure benchmark
CONFIG_RUNS_COUNT=${CONFIG_RUNS_COUNT:-10000}
CONFIG_BYTES_COUNT=${CONFIG_BYTES_COUNT:-4}
echo "CONFIG_RUNS_COUNT: $CONFIG_RUNS_COUNT"
echo "CONFIG_BYTES_COUNT: $CONFIG_BYTES_COUNT"

# build

cd src/coap_oscore/plugtest-server
make clean
make all -j$(nproc)
cd ../../../

# capture
tshark -i tapbr0 -w ${CONFIG_RUNS_COUNT}_runs-${CONFIG_BYTES_COUNT}-bytes-coap-oscore.pcapng &
tshark_pid=$!

# start server
cd src/coap_oscore/plugtest-server
(PORT=tap0 gnome-terminal -- make term) &
cd ../../../

# wait for run
sleep 2

# start client
cd aiocoap

printf "18\n${CONFIG_RUNS_COUNT}\n${CONFIG_BYTES_COUNT}\nq" | ./contrib/oscore-plugtest/plugtest-client '[fe80::200:ff:fe00:ab%tapbr0]' /tmp/clientctx

cd ../

# get pids
server_pid=$(pidof test_plugtest_server.elf)

# stop everything
kill $tshark_pid
kill $server_pid

# get result out
sleep 5

# analyse tshark capture
FRAME_LEN_SUM=$(tshark -r ${CONFIG_RUNS_COUNT}_runs-${CONFIG_BYTES_COUNT}-bytes-coap-oscore.pcapng -z io,stat,0,"SUM(frame.len)frame.len && not icmpv6" -q | tail -2 | head -1 | cut -d'|' -f 3)

# print results
cat << EOF
================================================================
GCOAP_ENABLE_DTLS: 2
CONFIG_RUNS_COUNT: $CONFIG_RUNS_COUNT
CONFIG_BYTES_COUNT: $CONFIG_BYTES_COUNT
RTT_REPLIES_COUNT: $RTT_REPLIES_COUNT
BENCHMARK_TIME_SUM: $BENCHMARK_TIME_SUM
FRAME_LEN_SUM: $FRAME_LEN_SUM
================================================================
EOF

cat >> benchmark_logs.txt << EOF
================================================================
GCOAP_ENABLE_DTLS: 2
CONFIG_RUNS_COUNT: $CONFIG_RUNS_COUNT
CONFIG_BYTES_COUNT: $CONFIG_BYTES_COUNT
RTT_REPLIES_COUNT: $RTT_REPLIES_COUNT
BENCHMARK_TIME_SUM: $BENCHMARK_TIME_SUM
FRAME_LEN_SUM: $FRAME_LEN_SUM
================================================================
EOF

cat >> benchmark_logs.csv << EOF
2	$CONFIG_RUNS_COUNT	$CONFIG_BYTES_COUNT	$RTT_REPLIES_COUNT	$BENCHMARK_TIME_SUM	$FRAME_LEN_SUM
EOF

