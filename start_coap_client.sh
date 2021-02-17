#!/bin/bash

./setup_iface.sh

cd src/coap/client
make all -j$(nproc)
PORT=tap1 make term