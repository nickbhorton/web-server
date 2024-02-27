#!/bin/bash
if test -f .env; then
    source .env
else 
    echo "need a .env file"
    exit 1
fi
echo "current .env file:"
cat .env

server_address=$WEBSERVER_ADDRESS
port=$WEBSERVER_PORT
echo "$server_address:$port"

# this was used in the server side to use port 8080 instead
# https://serverfault.com/questions/112795/how-to-run-a-server-on-port-80-as-a-normal-user-on-linux

rm out.txt.prev > /dev/null 2>&1
mv out.txt out.txt.prev > /dev/null 2>&1
./server $server_address $port > out.txt 2>&1 &
server_pid=$!
tail out.txt -f &
tail_pid=$!

# handling signals
function handle_cntl_c {
    kill -9 $server_pid
    kill -9 $tail_pid
    exit 0
}

# set traps
trap handle_cntl_c SIGINT

while [ 1 ] 
do
    sleep 10
done
