all: p2mpclient p2mpserver

p2mpserver: receiver.o main_receiver.o
	clang++ -std=c++17 -g -o p2mpserver.out Receiver.o main_receiver.o

p2mpclient: sender.o main_sender.o
	clang++ -std=c++17 -g -o p2mpclient.out Sender.o main_sender.o

main_receiver.o: Receiver.h main_receiver.cpp
	clang++ -std=c++17 -g -c main_receiver.cpp

main_sender.o: Sender.h main_sender.cpp
	clang++ -std=c++17 -g -c main_sender.cpp

receiver.o: Receiver.h Receiver.cpp
	clang++ -std=c++17 -g -c Receiver.cpp

sender.o: Sender.h Sender.cpp
	clang++ -std=c++17 -g -c Sender.cpp

clean:
	rm *.o *.out