#!/bin/bash
#
# SO2 - Networking Lab (#11)
#
# Test script for exercise #1
#

set -x

# insert module
insmod filter.ko || exit 1

# listen for connections on localhost, port 60000 (run in background)
netcat -l -p 60000 &

# wait for netcat to start listening
sleep 1

# connect to localhost, port 60000, starting a connection using local
# port number 600001; quit after EOF on stdin and delay of 1 second
echo "Should show up in filter." | netcat localhost 60000 -p 60001 -q 1

# remove module
rmmod filter || exit 1
