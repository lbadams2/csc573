#include "Sender.h"
#include <thread>
#include <mutex>
#include <string>
#include <vector>
#include <iostream>

using std::cout;

int main(int argc, char** argv) {
    int mss = atoi(argv[argc-1]);
    std::string file_name = argv[argc-2];
    int port = atoi(argv[argc-3]);
    std::vector<std::string> hosts;
    for(int i = 1; i < argc-3; i++)
        hosts.push_back(argv[i]);
    
    Sender sender(hosts, port, file_name, mss, 2);
    //std::mutex mrun;
    std::vector<std::thread> threads;
    for(string host: hosts) {
        std::thread t(&Sender::send_file, &sender, host.c_str());
        threads.push_back(std::move(t));
    }
    std::vector<int> files_sent = sender.get_files_sent();
    while(files_sent.size() < hosts.size())
    {
        {
            std::lock_guard<std::mutex> lock(sender.mrun);
            sender.worker_count = hosts.size();
            ++sender.current_iteration;
        }
        sender.main_ready.notify_all();
        {
            std::unique_lock<std::mutex> lock(sender.mrun);
            sender.iteration_complete.wait(lock, [&sender]{return sender.worker_count == 0; });
        }
    }
    for(auto& t: threads)
        t.join();
}
