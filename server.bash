#!/bin/bash
server_address="10.0.0.69"
port=8080
# this was used in the server side to use port 8080 instead
# https://serverfault.com/questions/112795/how-to-run-a-server-on-port-80-as-a-normal-user-on-linux

./server $server_address $port
exit 0
