#!/bin/bash

./setup_iface.sh

cd src/coap/client
make clean
GCOAP_ENABLE_DTLS=0 make all -j$(nproc)
GCOAP_ENABLE_DTLS=0 make all -j$(nproc)
PORT=tap1 make term
