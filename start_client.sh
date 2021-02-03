#!/bin/bash

./setup_iface.sh

cd client
make all
PORT=tap1 make term