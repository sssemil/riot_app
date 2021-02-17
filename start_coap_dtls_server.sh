#!/bin/bash

./setup_iface.sh

cd src/coap_dtls/server
make all -j$(nproc)
PORT=tap0 make term