#!/bin/bash

./setup_iface.sh

cd src/coap/client
../../../conditional_clean.sh
GCOAP_ENABLE_DTLS=1 make all -j$(nproc)
PORT=tap1 make term
