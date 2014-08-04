HOSTNAME=$(shell hostname)
ifeq (${HOSTNAME},swappys-linux)
CC:=g++
else
CC:=/util/bin/g++
endif

all: swapnesh_server.o server_helper.o
	${CC} -o server swapnesh_server.o server_helper.o -g -lrt
	
swapnesh_server.o: swapnesh_server.cpp server_helper.cpp server.h
	${CC} -c swapnesh_server.cpp -o swapnesh_server.o -g
	
server_helper.o: server_helper.cpp server.h
	${CC} -c server_helper.cpp -o server_helper.o -g
	 
clean:
	rm -f *.o
	rm server