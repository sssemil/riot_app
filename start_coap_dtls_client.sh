#!/bin/bash

./setup_iface.sh

cd src/coap_dtls/client
make all
PORT=tap1 make term