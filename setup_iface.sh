#!/bin/bash
exit
sudo ./RIOT/dist/tools/tapsetup/tapsetup -c 2
sudo ip link set address 00:00:00:00:00:aa dev tap0
sudo ip link set address 00:00:00:00:00:bb dev tap1
