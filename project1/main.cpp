#include "Peer.h"
#include "RegistrationServer.h"
#include <iostream>
using namespace std;

int main() {
    cout << "entered main\n";
    //cout.flush();
    //printf("entered main\n");
    RegistrationServer rs = RegistrationServer();
    cout << "started registration server\n";
    cout.flush();
    Peer p = Peer();
    cout << "started peer\n";
    cout.flush();
    return 0;
}