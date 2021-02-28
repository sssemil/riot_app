#!/bin/bash

./setup_iface.sh

cd src/coap/server
../../../conditional_clean.sh
GCOAP_ENABLE_DTLS=1 make all -j$(nproc)
PORT=tap0 make term
