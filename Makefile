server: server.cc

run: server
	./server.bash > out.txt 2>&1
	cat out.txt
