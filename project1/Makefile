p2p: peer.o registration.o main.o
	clang++ -std=c++17 -g -o p2p.out RegistrationServer.o Peer.o main.o

main.o: RegistrationServer.h Peer.h main.cpp
	clang++ -std=c++17 -g -c main.cpp

registration.o: RegistrationServer.h RegistrationServer.cpp
	clang++ -std=c++17 -g -c RegistrationServer.cpp

peer.o: Peer.h Peer.cpp
	clang++ -std=c++17 -g -c Peer.cpp

clean:
	rm *.o *.out