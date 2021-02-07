#!/bin/bash

./setup_iface.sh

cd src/coap/client
make all
PORT=tap1 make term