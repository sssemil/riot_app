#!/bin/bash

sudo ./RIOT/dist/tools/tapsetup/tapsetup -c 2

cd client
make all
PORT=tap1 make term