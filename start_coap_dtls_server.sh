#!/bin/bash

./setup_iface.sh

cd src/coap/server
make clean
GCOAP_ENABLE_DTLS=1 make all -j$(nproc)
GCOAP_ENABLE_DTLS=1 make all -j$(nproc)
PORT=tap0 make term
