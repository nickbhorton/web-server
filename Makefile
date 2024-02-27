CXXFLAGS=-std=c++20 -Wall -Werror -Wpedantic
.PHONY: run nyx clean

server: server.cc

run: server
	./env-local.bash
	./server.bash

nyx: server
	./env-nyx.bash
	./server.bash

clean:
	rm -f out.txt
	rm -f out.txt.prev
	rm -f server
	rm -f .env
