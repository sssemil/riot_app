#!/bin/bash

./setup_iface.sh

cd server
make all
PORT=tap0 make term