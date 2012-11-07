#!/bin/bash
#
# SO2 - Networking Lab (#11)
#
# Test script for exercise #3
#

set -x

# insert module (run in background, it waits for a connection)
insmod tcp_sock.ko &

# wait for module to start listening
sleep 1

# list all currently listening servers and active connections
# for both TCP and UDP, and don't resolve hostnames
netstat -tuan

# connect to localhost, port 60000, starting a connection using local
# port number 600001; quit after EOF on stdin and delay of 1 second
echo "Should connect." | netcat localhost 60000 -p 60001 -q 1

# remove module
rmmod tcp_sock || exit 1
