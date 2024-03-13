CXXFLAGS=-std=c++20 -Wall -Werror -Wpedantic
.PHONY: run nyx clean

server: server.cc server_helpers.o filetype.o http_request.o http_response.o http_method.o log.o url.o url.h http.h

server_helpers.o: log.o log.h server.h
url.o: log.o log.h
http_request.o: log.o log.h
http_response.o: log.o log.h url.o url.h


run: server
	./env-local.bash
	./server.bash

nyx: server
	./env-nyx.bash
	./server.bash

clean:
	rm -f out.txt
	rm -f *.o
	rm -f out.txt.prev
	rm -f server
