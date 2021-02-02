#!/bin/bash

cd client

sudo ./RIOT/dist/tools/tapsetup/tapsetup -c 2

make all
PORT=tap1 make term