#!/bin/bash

./setup_iface.sh

cd src/coap_dtls/server
make all
PORT=tap0 make term