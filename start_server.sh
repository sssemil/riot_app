#!/bin/bash

sudo ./RIOT/dist/tools/tapsetup/tapsetup -c 2

cd server
make all
PORT=tap0 make term