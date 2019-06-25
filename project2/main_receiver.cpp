#include "Receiver.h"
#include <string>

using std::string;

int main(int argc, char** argv) {
    double loss_prob = std::stod(argv[argc-1]);
    string file_name = argv[argc-2];
    Receiver rec(file_name, loss_prob);
    rec.download_file();
    return 0;
}