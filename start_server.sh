#!/bin/bash

cd server

sudo ./RIOT/dist/tools/tapsetup/tapsetup -c 2

make all
PORT=tap0 make term