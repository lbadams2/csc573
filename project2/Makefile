all: p2mpclient p2mpserver

p2mpserver: receiver.o main_receiver.o
	g++ -std=c++17 -g -o p2mpserver.out Receiver.o main_receiver.o

p2mpclient: main_sender.cpp
	g++ -std=c++17 -pthread -fsanitize=address -g -o p2mpclient.out main_sender.cpp

main_receiver.o: Receiver.h main_receiver.cpp
	g++ -std=c++17 -g -c main_receiver.cpp

receiver.o: Receiver.h Receiver.cpp
	g++ -std=c++17 -g -c Receiver.cpp

clean:
	rm *.o *.out
