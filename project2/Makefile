p2mp: sender.o main.o
	clang++ -std=c++17 -g -o p2mp.out Sender.o main.o

main.o: Sender.h main.cpp
	clang++ -std=c++17 -g -c main.cpp

receiver.o: Receiver.h Receiver.cpp
	clang++ -std=c++17 -g -c Receiver.cpp

sender.o: Sender.h Sender.cpp
	clang++ -std=c++17 -g -c Sender.cpp

clean:
	rm *.o *.out