#!/bin/bash

set -o xtrace

./setup_iface.sh

# make sure that no other instances are running
killall default.elf

# build

if [ -n "$GCOAP_ENABLE_DTLS" ]; then
    GCOAP_ENABLE_DTLS=1
fi

cd src/coap/client
../../../conditional_clean.sh
CONFIG_RUNS_COUNT=10000 CONFIG_BYTES_COUNT=6 make all -j$(nproc)
cd ../../../

cd src/coap/server
../../../conditional_clean.sh
make all -j$(nproc)
cd ../../../

# start server
cd src/coap/server
(PORT=tap0 make term) &
cd ../../../

# wait for run
sleep 3

# start client
cd src/coap/client
(PORT=tap1 make term) &
cd ../../../

# wait for run
sleep 1

# get pids
elf_pids=($(pidof default.elf))
echo $elf_pids
server_pid=${elf_pids[1]}
client_pid=${elf_pids[0]}

if [ -z "$server_pid" ] || [ -z "$client_pid" ]; then
    echo "RIOT-OS start failed! Killing any survivors..."
    kill $client_pid
    kill $server_pid
fi

# wait for run
echo "server PID:" $server_pid
echo "client PID:" $client_pid

# capture
tshark -i tapbr0 -w 10000_coap_dtls.pcapng &
tshark_pid=$!

# wait for client to exit
echo "waiting for client to finsh..."
while ps | grep "$client_pid"; do
    sleep 5
done

# stop everything
kill $tshark_pid
kill $server_pid
