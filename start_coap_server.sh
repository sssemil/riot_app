#!/bin/bash

./setup_iface.sh

cd src/coap/server
make all
PORT=tap0 make term