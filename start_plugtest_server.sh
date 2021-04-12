#!/bin/bash

./setup_iface.sh

cd src/coap_oscore/plugtest-server
make clean
GCOAP_ENABLE_DTLS=0 make all -j$(nproc)
PORT=tap0 make term
