#!/bin/bash

sudo ./RIOT/dist/tools/tapsetup/tapsetup -c 2

make all
sudo make term