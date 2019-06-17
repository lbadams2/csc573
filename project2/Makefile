p2mp: sender.o main.o
	clang++ -std=c++17 -g -o p2p.out Sender.o main.o

main.o: Sender.h main.cpp
	clang++ -std=c++17 -g -c main.cpp

sender.o: Sender.h Sender.cpp
	clang++ -std=c++17 -g -c Sender.cpp

clean:
	rm *.o *.out