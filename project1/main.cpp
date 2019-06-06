#include "Peer.h"
#include "RegistrationServer.h"
#include <iostream>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <functional>
//using namespace std;

/*
class ThreadPool
{
public:
    
    ThreadPool (int threads) : shutdown_ (false)
    {
        // Create the specified number of threads
        threads_.reserve (threads);
        // emplace takes an rvalue
        for (int i = 0; i < threads; ++i)
            threads_.emplace_back (std::bind (&ThreadPool::threadEntry, this, i));
    }
    
    ~ThreadPool ()
    {
        {
            // Unblock any threads and tell them to stop
            std::unique_lock <std::mutex> l (lock_);
            
            shutdown_ = true;
            condVar_.notify_all();
        }
        
        // Wait for all threads to stop
        std::cerr << "Joining threads" << std::endl;
        for (auto& thread : threads_)
            thread.join();
    }
    
    void doJob (std::function <void (void)> func)
    {
        // Place a job on the queu and unblock a thread
        std::unique_lock <std::mutex> l (lock_);
        
        jobs_.emplace (std::move (func));
        condVar_.notify_one();
    }
    
protected:
    // this is the function the thread is executing, this function executes the job, don't need join because of while(1)
    void threadEntry (int i)
    {
        std::function <void (void)> job;
        
        while (1)
        {
            {
                std::unique_lock <std::mutex> l (lock_);
                
                while (! shutdown_ && jobs_.empty())
                    condVar_.wait (l);
                
                if (jobs_.empty ())
                {
                    // No jobs to do and we are shutting down
                    std::cerr << "Thread " << i << " terminates" << std::endl;
                    return;
                }
                
                std::cerr << "Thread " << i << " does a job" << std::endl;
                job = std::move (jobs_.front ());
                jobs_.pop();
            }
            
            // Do the job without holding any locks
            job ();
        }
        
    }
    
    std::mutex lock_;
    std::condition_variable condVar_;
    bool shutdown_;
    std::queue <std::function <void (void)>> jobs_;
    std::vector <std::thread> threads_;
};
 */

void test_task() {
    // initialize directories and files for this task
    Peer::move_rfc_files("test");
    
    // construct peer objects
    Peer *a = new Peer("peer_a");
    Peer::RFC_Client &a_client = a->get_rfc_client();
    Peer::RFC_Server &a_server = a->get_rfc_server();
    Peer *b = new Peer("peer_b");
    Peer::RFC_Client &b_client = b->get_rfc_client();
    Peer::RFC_Server &b_server = b->get_rfc_server();
    
    //start peer servers
    std::thread th_a_server(&Peer::RFC_Server::start, a_server);
    std::thread th_b_server(&Peer::RFC_Server::start, b_server);
    
    // register peer servers
    std::unordered_map<std::string, std::string> arg_map;
    std::thread th_a_client(&Peer::RFC_Client::request, a_client, "Register", arg_map);
    std::thread th_b_client(&Peer::RFC_Client::request, b_client, "Register", arg_map);
    th_a_client.join();
    th_b_client.join();
    
    // query for peer list
    th_a_client = std::thread(&Peer::RFC_Client::request, a_client, "Pquery", arg_map);
    th_a_client.join();
    std::vector<Remote_Peer> peers = a->get_peer_index();
    //std::cout << "peer a index size " << peers.size() << "\n";
    
    // query peer for rfc list
    int remote_peer_port = peers[0].port;
    arg_map["PORT"] = std::to_string(remote_peer_port);
    th_a_client = std::thread(&Peer::RFC_Client::request, a_client, "Rfcquery", arg_map);
    th_a_client.join();
    std::vector<RFC_Record> rfc_list = a->get_rfc_index();
    //std::cout << "peer a rfc index size " << rfc_list.size() << "\n";
    
    // download rfc
    std::string title = rfc_list[0].title;
    arg_map["title"] = title;
    th_a_client = std::thread(&Peer::RFC_Client::request, a_client, "Getrfc", arg_map);
    th_a_client.join();
    
    // peer b leaves
    arg_map.clear();
    th_b_client = std::thread(&Peer::RFC_Client::request, b_client, "Leave", arg_map);
    th_b_client.join();
    
    // peer a query for peer list again
    th_a_client = std::thread(&Peer::RFC_Client::request, a_client, "Pquery", arg_map);
    th_a_client.join();
    
    // stop registration server
    arg_map.clear();
    th_a_client = std::thread(&Peer::RFC_Client::request, a_client, "Stop", arg_map);
    th_a_client.join();
    
    // stop peer A server
    arg_map["PORT"] = std::to_string(a->server_port);
    th_a_client = std::thread(&Peer::RFC_Client::request, a_client, "Stop_Peer", arg_map);
    th_a_client.join();
    th_a_server.join();
    
    // stop peer B server
    arg_map["PORT"] = std::to_string(b->server_port);;
    th_b_client = std::thread(&Peer::RFC_Client::request, b_client, "Stop_Peer", arg_map);
    th_b_client.join();
    th_b_server.join();
    delete a;
    delete b;
}

/*
void download_files(ThreadPool *pool, int num_files, std::vector<RFC_Record> &rfc_list, Peer::RFC_Client &client, int port) {
    std::unordered_map<std::string, std::string> arg_map;
    std::string title;
    arg_map["PORT"] = std::to_string(port);
    for(int i = 0; i < num_files + 1; i++) {
        title = rfc_list[i].title;
        arg_map["title"] = title;
        pool->doJob (std::bind (&Peer::RFC_Client::request, client, "Getrfc", arg_map));
    }
}
 

void download_files(int num_files, std::vector<RFC_Record> &rfc_list, Peer::RFC_Client &client, int port) {
    std::unordered_map<std::string, std::string> arg_map;
    std::string title;
    arg_map["PORT"] = std::to_string(port);
    for(int i = 0; i < num_files + 1; i++) {
        title = rfc_list[i].title;
        arg_map["title"] = title;
        client.request("Getrfc", arg_map);
        //pool->doJob (std::bind (&Peer::RFC_Client::request, client, "Getrfc", arg_map));
    }
}
 */

void write_download_times(std::string peer_name, std::map<int, long> &times) {
    std::ofstream stream(peer_name + "_times");
    std::string s;
    for(auto& kv : times) {
        s = std::to_string(kv.second);
        stream << s << '\n';
        // Add '\n' character  ^^^^
    }
    stream.close();
}

void task1() {
    // initialize directories and files for this task
    Peer::move_rfc_files("task1");
    
    // construct peer objects
    Peer *p0 = new Peer("peer_0");
    Peer::RFC_Client &p0_client = p0->get_rfc_client();
    Peer::RFC_Server &p0_server = p0->get_rfc_server();
    Peer *p1 = new Peer("peer_1");
    Peer::RFC_Client &p1_client = p1->get_rfc_client();
    Peer::RFC_Server &p1_server = p1->get_rfc_server();
    Peer *p2 = new Peer("peer_2");
    Peer::RFC_Client &p2_client = p2->get_rfc_client();
    Peer::RFC_Server &p2_server = p2->get_rfc_server();
    Peer *p3 = new Peer("peer_3");
    Peer::RFC_Client &p3_client = p3->get_rfc_client();
    Peer::RFC_Server &p3_server = p3->get_rfc_server();
    Peer *p4 = new Peer("peer_4");
    Peer::RFC_Client &p4_client = p4->get_rfc_client();
    Peer::RFC_Server &p4_server = p4->get_rfc_server();
    Peer *p5 = new Peer("peer_5");
    Peer::RFC_Client &p5_client = p5->get_rfc_client();
    Peer::RFC_Server &p5_server = p5->get_rfc_server();
    
    //start peer servers
    std::thread th_p0_server(&Peer::RFC_Server::start, p0_server);
    std::thread th_p1_server(&Peer::RFC_Server::start, p1_server);
    std::thread th_p2_server(&Peer::RFC_Server::start, p2_server);
    std::thread th_p3_server(&Peer::RFC_Server::start, p3_server);
    std::thread th_p4_server(&Peer::RFC_Server::start, p4_server);
    std::thread th_p5_server(&Peer::RFC_Server::start, p5_server);
    
    
    // register peer servers
    std::unordered_map<std::string, std::string> arg_map;
    std::thread th_p0_client(&Peer::RFC_Client::request, p0_client, "Register", arg_map);
    std::thread th_p1_client(&Peer::RFC_Client::request, p1_client, "Register", arg_map);
    std::thread th_p2_client(&Peer::RFC_Client::request, p2_client, "Register", arg_map);
    std::thread th_p3_client(&Peer::RFC_Client::request, p3_client, "Register", arg_map);
    std::thread th_p4_client(&Peer::RFC_Client::request, p4_client, "Register", arg_map);
    std::thread th_p5_client(&Peer::RFC_Client::request, p5_client, "Register", arg_map);
    th_p0_client.join();
    th_p1_client.join();
    th_p2_client.join();
    th_p3_client.join();
    th_p4_client.join();
    th_p5_client.join();
    
    // get peer list
    th_p0_client = std::thread(&Peer::RFC_Client::request, p0_client, "Pquery", arg_map);
    th_p1_client = std::thread(&Peer::RFC_Client::request, p1_client, "Pquery", arg_map);
    th_p2_client = std::thread(&Peer::RFC_Client::request, p2_client, "Pquery", arg_map);
    th_p3_client = std::thread(&Peer::RFC_Client::request, p3_client, "Pquery", arg_map);
    th_p4_client = std::thread(&Peer::RFC_Client::request, p4_client, "Pquery", arg_map);
    th_p5_client = std::thread(&Peer::RFC_Client::request, p5_client, "Pquery", arg_map);
    th_p0_client.join();
    th_p1_client.join();
    th_p2_client.join();
    th_p3_client.join();
    th_p4_client.join();
    th_p5_client.join();
    
    // get rfc index from peer 0
    int p0_port = p0->server_port;
    arg_map["PORT"] = std::to_string(p0_port);
    th_p1_client = std::thread(&Peer::RFC_Client::request, p1_client, "Rfcquery", arg_map);
    th_p2_client = std::thread(&Peer::RFC_Client::request, p2_client, "Rfcquery", arg_map);
    th_p3_client = std::thread(&Peer::RFC_Client::request, p3_client, "Rfcquery", arg_map);
    th_p4_client = std::thread(&Peer::RFC_Client::request, p4_client, "Rfcquery", arg_map);
    th_p5_client = std::thread(&Peer::RFC_Client::request, p5_client, "Rfcquery", arg_map);
    th_p1_client.join();
    th_p2_client.join();
    th_p3_client.join();
    th_p4_client.join();
    th_p5_client.join();
    
    // each peer download 50 files from peer 0
    // create thread pool for clients
    /*
    ThreadPool pool(60);
    pool.doJob(std::bind(download_files, &pool, 50, p1->get_rfc_index(), p1_client, p0_port));
    pool.doJob(std::bind(download_files, &pool, 50, p2->get_rfc_index(), p2_client, p0_port));
    pool.doJob(std::bind(download_files, &pool, 50, p3->get_rfc_index(), p3_client, p0_port));
    pool.doJob(std::bind(download_files, &pool, 50, p4->get_rfc_index(), p4_client, p0_port));
    pool.doJob(std::bind(download_files, &pool, 50, p5->get_rfc_index(), p5_client, p0_port));
     
    th_p1_client = std::thread(download_files, p1_client, 50, p1->get_rfc_index(), p1_client, p0_port);
    th_p2_client = std::thread(download_files, p1_client, 50, p2->get_rfc_index(), p2_client, p0_port);
    th_p3_client = std::thread(download_files, p1_client, 50, p3->get_rfc_index(), p3_client, p0_port);
    th_p4_client = std::thread(download_files, p1_client, 50, p4->get_rfc_index(), p4_client, p0_port);
    th_p5_client = std::thread(download_files, p1_client, 50, p5->get_rfc_index(), p5_client, p0_port);
     */
    arg_map["num_files"] = "50";
    th_p1_client = std::thread(&Peer::RFC_Client::download_files, p1_client, arg_map);
    th_p2_client = std::thread(&Peer::RFC_Client::download_files, p2_client, arg_map);
    th_p3_client = std::thread(&Peer::RFC_Client::download_files, p3_client, arg_map);
    th_p4_client = std::thread(&Peer::RFC_Client::download_files, p4_client, arg_map);
    th_p5_client = std::thread(&Peer::RFC_Client::download_files, p5_client, arg_map);
    th_p1_client.join();
    th_p2_client.join();
    th_p3_client.join();
    th_p4_client.join();
    th_p5_client.join();
    
    // stop registration server
    arg_map.clear();
    th_p1_client = std::thread(&Peer::RFC_Client::request, p1_client, "Stop", arg_map);
    th_p1_client.join();
    
    // stop peer servers
    arg_map["PORT"] = std::to_string(p0->server_port);
    th_p0_client = std::thread(&Peer::RFC_Client::request, p0_client, "Stop_Peer", arg_map);
    th_p0_server.join();
    th_p0_client.join();
    arg_map["PORT"] = std::to_string(p1->server_port);
    th_p1_client = std::thread(&Peer::RFC_Client::request, p1_client, "Stop_Peer", arg_map);
    th_p1_server.join();
    th_p1_client.join();
    arg_map["PORT"] = std::to_string(p2->server_port);
    th_p2_client = std::thread(&Peer::RFC_Client::request, p2_client, "Stop_Peer", arg_map);
    th_p2_server.join();
    th_p2_client.join();
    arg_map["PORT"] = std::to_string(p3->server_port);
    th_p3_client = std::thread(&Peer::RFC_Client::request, p3_client, "Stop_Peer", arg_map);
    th_p3_server.join();
    th_p3_client.join();
    arg_map["PORT"] = std::to_string(p4->server_port);
    th_p4_client = std::thread(&Peer::RFC_Client::request, p4_client, "Stop_Peer", arg_map);
    th_p4_server.join();
    th_p4_client.join();
    arg_map["PORT"] = std::to_string(p5->server_port);
    th_p5_client = std::thread(&Peer::RFC_Client::request, p5_client, "Stop_Peer", arg_map);
    th_p5_server.join();
    th_p5_client.join();
    
    write_download_times(p1->peer_name, p1->get_download_times());
    write_download_times(p2->peer_name, p2->get_download_times());
    write_download_times(p3->peer_name, p3->get_download_times());
    write_download_times(p4->peer_name, p4->get_download_times());
    write_download_times(p5->peer_name, p5->get_download_times());
    /*
    std::cout << "peer 1 times size " << p1->get_download_times().size() << "\n";
    std::cout << "peer 2 times size " << p2->get_download_times().size() << "\n";
    std::cout << "peer 3 times size " << p3->get_download_times().size() << "\n";
    std::cout << "peer 4 times size " << p4->get_download_times().size() << "\n";
    std::cout << "peer 5 times size " << p5->get_download_times().size() << "\n";
    */
    delete p0;
    delete p1;
    delete p2;
    delete p3;
    delete p4;
    delete p5;
}

void task2() {
    // initialize directories and files for this task
    Peer::move_rfc_files("task2");
    
    // construct peer objects
    Peer *p0 = new Peer("peer_0");
    Peer::RFC_Client &p0_client = p0->get_rfc_client();
    Peer::RFC_Server &p0_server = p0->get_rfc_server();
    Peer *p1 = new Peer("peer_1");
    Peer::RFC_Client &p1_client = p1->get_rfc_client();
    Peer::RFC_Server &p1_server = p1->get_rfc_server();
    Peer *p2 = new Peer("peer_2");
    Peer::RFC_Client &p2_client = p2->get_rfc_client();
    Peer::RFC_Server &p2_server = p2->get_rfc_server();
    Peer *p3 = new Peer("peer_3");
    Peer::RFC_Client &p3_client = p3->get_rfc_client();
    Peer::RFC_Server &p3_server = p3->get_rfc_server();
    Peer *p4 = new Peer("peer_4");
    Peer::RFC_Client &p4_client = p4->get_rfc_client();
    Peer::RFC_Server &p4_server = p4->get_rfc_server();
    Peer *p5 = new Peer("peer_5");
    Peer::RFC_Client &p5_client = p5->get_rfc_client();
    Peer::RFC_Server &p5_server = p5->get_rfc_server();
    
    //start peer servers
    std::thread th_p0_server(&Peer::RFC_Server::start, p0_server);
    std::thread th_p1_server(&Peer::RFC_Server::start, p1_server);
    std::thread th_p2_server(&Peer::RFC_Server::start, p2_server);
    std::thread th_p3_server(&Peer::RFC_Server::start, p3_server);
    std::thread th_p4_server(&Peer::RFC_Server::start, p4_server);
    std::thread th_p5_server(&Peer::RFC_Server::start, p5_server);
    
    
    // register peer servers
    std::unordered_map<std::string, std::string> arg_map;
    std::thread th_p0_client(&Peer::RFC_Client::request, p0_client, "Register", arg_map);
    std::thread th_p1_client(&Peer::RFC_Client::request, p1_client, "Register", arg_map);
    std::thread th_p2_client(&Peer::RFC_Client::request, p2_client, "Register", arg_map);
    std::thread th_p3_client(&Peer::RFC_Client::request, p3_client, "Register", arg_map);
    std::thread th_p4_client(&Peer::RFC_Client::request, p4_client, "Register", arg_map);
    std::thread th_p5_client(&Peer::RFC_Client::request, p5_client, "Register", arg_map);
    th_p0_client.join();
    th_p1_client.join();
    th_p2_client.join();
    th_p3_client.join();
    th_p4_client.join();
    th_p5_client.join();
    
    // get peer list
    th_p0_client = std::thread(&Peer::RFC_Client::request, p0_client, "Pquery", arg_map);
    th_p1_client = std::thread(&Peer::RFC_Client::request, p1_client, "Pquery", arg_map);
    th_p2_client = std::thread(&Peer::RFC_Client::request, p2_client, "Pquery", arg_map);
    th_p3_client = std::thread(&Peer::RFC_Client::request, p3_client, "Pquery", arg_map);
    th_p4_client = std::thread(&Peer::RFC_Client::request, p4_client, "Pquery", arg_map);
    th_p5_client = std::thread(&Peer::RFC_Client::request, p5_client, "Pquery", arg_map);
    th_p0_client.join();
    th_p1_client.join();
    th_p2_client.join();
    th_p3_client.join();
    th_p4_client.join();
    th_p5_client.join();
    
    int p0_port = p0->server_port;
    arg_map["PORT"] = std::to_string(p0_port);
    th_p1_client = std::thread(&Peer::RFC_Client::request, p1_client, "Rfcquery", arg_map);
    th_p2_client = std::thread(&Peer::RFC_Client::request, p2_client, "Rfcquery", arg_map);
    th_p3_client = std::thread(&Peer::RFC_Client::request, p3_client, "Rfcquery", arg_map);
    th_p4_client = std::thread(&Peer::RFC_Client::request, p4_client, "Rfcquery", arg_map);
    th_p5_client = std::thread(&Peer::RFC_Client::request, p5_client, "Rfcquery", arg_map);
    th_p1_client.join();
    th_p2_client.join();
    th_p3_client.join();
    th_p4_client.join();
    th_p5_client.join();
     
    
    int p1_port = p1->server_port;
    arg_map["PORT"] = std::to_string(p1_port);
    th_p0_client = std::thread(&Peer::RFC_Client::request, p0_client, "Rfcquery", arg_map);
    th_p2_client = std::thread(&Peer::RFC_Client::request, p2_client, "Rfcquery", arg_map);
    th_p3_client = std::thread(&Peer::RFC_Client::request, p3_client, "Rfcquery", arg_map);
    th_p4_client = std::thread(&Peer::RFC_Client::request, p4_client, "Rfcquery", arg_map);
    th_p5_client = std::thread(&Peer::RFC_Client::request, p5_client, "Rfcquery", arg_map);
    th_p0_client.join();
    th_p2_client.join();
    th_p3_client.join();
    th_p4_client.join();
    th_p5_client.join();
    
    int p2_port = p2->server_port;
    arg_map["PORT"] = std::to_string(p2_port);
    th_p0_client = std::thread(&Peer::RFC_Client::request, p0_client, "Rfcquery", arg_map);
    th_p1_client = std::thread(&Peer::RFC_Client::request, p1_client, "Rfcquery", arg_map);
    th_p3_client = std::thread(&Peer::RFC_Client::request, p3_client, "Rfcquery", arg_map);
    th_p4_client = std::thread(&Peer::RFC_Client::request, p4_client, "Rfcquery", arg_map);
    th_p5_client = std::thread(&Peer::RFC_Client::request, p5_client, "Rfcquery", arg_map);
    th_p0_client.join();
    th_p1_client.join();
    th_p3_client.join();
    th_p4_client.join();
    th_p5_client.join();
    
    int p3_port = p3->server_port;
    arg_map["PORT"] = std::to_string(p3_port);
    th_p0_client = std::thread(&Peer::RFC_Client::request, p0_client, "Rfcquery", arg_map);
    th_p1_client = std::thread(&Peer::RFC_Client::request, p1_client, "Rfcquery", arg_map);
    th_p2_client = std::thread(&Peer::RFC_Client::request, p2_client, "Rfcquery", arg_map);
    th_p4_client = std::thread(&Peer::RFC_Client::request, p4_client, "Rfcquery", arg_map);
    th_p5_client = std::thread(&Peer::RFC_Client::request, p5_client, "Rfcquery", arg_map);
    th_p0_client.join();
    th_p1_client.join();
    th_p2_client.join();
    th_p4_client.join();
    th_p5_client.join();
    
    int p4_port = p4->server_port;
    arg_map["PORT"] = std::to_string(p4_port);
    th_p0_client = std::thread(&Peer::RFC_Client::request, p0_client, "Rfcquery", arg_map);
    th_p1_client = std::thread(&Peer::RFC_Client::request, p1_client, "Rfcquery", arg_map);
    th_p2_client = std::thread(&Peer::RFC_Client::request, p2_client, "Rfcquery", arg_map);
    th_p3_client = std::thread(&Peer::RFC_Client::request, p3_client, "Rfcquery", arg_map);
    th_p5_client = std::thread(&Peer::RFC_Client::request, p5_client, "Rfcquery", arg_map);
    th_p0_client.join();
    th_p1_client.join();
    th_p2_client.join();
    th_p3_client.join();
    th_p5_client.join();
    
    int p5_port = p5->server_port;
    arg_map["PORT"] = std::to_string(p5_port);
    th_p0_client = std::thread(&Peer::RFC_Client::request, p0_client, "Rfcquery", arg_map);
    th_p1_client = std::thread(&Peer::RFC_Client::request, p1_client, "Rfcquery", arg_map);
    th_p2_client = std::thread(&Peer::RFC_Client::request, p2_client, "Rfcquery", arg_map);
    th_p3_client = std::thread(&Peer::RFC_Client::request, p3_client, "Rfcquery", arg_map);
    th_p4_client = std::thread(&Peer::RFC_Client::request, p4_client, "Rfcquery", arg_map);
    th_p0_client.join();
    th_p1_client.join();
    th_p2_client.join();
    th_p3_client.join();
    th_p4_client.join();
    
    std::vector<std::string> peer_names0;
    peer_names0.push_back("peer_1");
    peer_names0.push_back("peer_2");
    peer_names0.push_back("peer_3");
    peer_names0.push_back("peer_4");
    peer_names0.push_back("peer_5");
    std::vector<std::string> peer_names1;
    peer_names1.push_back("peer_2");
    peer_names1.push_back("peer_3");
    peer_names1.push_back("peer_4");
    peer_names1.push_back("peer_5");
    peer_names1.push_back("peer_0");
    std::vector<std::string> peer_names2;
    peer_names2.push_back("peer_3");
    peer_names2.push_back("peer_4");
    peer_names2.push_back("peer_5");
    peer_names2.push_back("peer_0");
    peer_names2.push_back("peer_1");
    std::vector<std::string> peer_names3;
    peer_names3.push_back("peer_4");
    peer_names3.push_back("peer_5");
    peer_names3.push_back("peer_0");
    peer_names3.push_back("peer_1");
    peer_names3.push_back("peer_2");
    std::vector<std::string> peer_names4;
    peer_names4.push_back("peer_5");
    peer_names4.push_back("peer_0");
    peer_names4.push_back("peer_1");
    peer_names4.push_back("peer_2");
    peer_names4.push_back("peer_3");
    std::vector<std::string> peer_names5;
    peer_names5.push_back("peer_0");
    peer_names5.push_back("peer_1");
    peer_names5.push_back("peer_2");
    peer_names5.push_back("peer_3");
    peer_names5.push_back("peer_4");

    //worst vectors
    std::vector<std::string> worst_peer_names;
    worst_peer_names.push_back("peer_0");
    worst_peer_names.push_back("peer_1");
    worst_peer_names.push_back("peer_2");
    worst_peer_names.push_back("peer_3");
    worst_peer_names.push_back("peer_4");
    worst_peer_names.push_back("peer_5");
    std::vector<std::string> worst_peer_names0;
    worst_peer_names0.push_back("peer_5");
    worst_peer_names0.push_back("peer_4");
    worst_peer_names0.push_back("peer_3");
    worst_peer_names0.push_back("peer_2");
    worst_peer_names0.push_back("peer_1");
    worst_peer_names0.push_back("peer_0");
    th_p0_client = std::thread(&Peer::RFC_Client::download_files_2, p0_client, arg_map, peer_names0);
    th_p1_client = std::thread(&Peer::RFC_Client::download_files_2, p1_client, arg_map, peer_names1);
    th_p2_client = std::thread(&Peer::RFC_Client::download_files_2, p2_client, arg_map, peer_names2);
    th_p3_client = std::thread(&Peer::RFC_Client::download_files_2, p3_client, arg_map, peer_names3);
    th_p4_client = std::thread(&Peer::RFC_Client::download_files_2, p4_client, arg_map, peer_names4);
    th_p5_client = std::thread(&Peer::RFC_Client::download_files_2, p5_client, arg_map, peer_names5);
    th_p0_client.join();
    th_p1_client.join();
    th_p2_client.join();
    th_p3_client.join();
    th_p4_client.join();
    th_p5_client.join();
    //std::cout << "download size " << p1->get_download_times().size() << "\n";
    //std::cout << "done\n";
    
    // stop registration server
    arg_map.clear();
    th_p1_client = std::thread(&Peer::RFC_Client::request, p1_client, "Stop", arg_map);
    th_p1_client.join();
    
    // stop peer servers
    arg_map["PORT"] = std::to_string(p0->server_port);
    th_p0_client = std::thread(&Peer::RFC_Client::request, p0_client, "Stop_Peer", arg_map);
    th_p0_server.join();
    th_p0_client.join();
    arg_map["PORT"] = std::to_string(p1->server_port);
    th_p1_client = std::thread(&Peer::RFC_Client::request, p1_client, "Stop_Peer", arg_map);
    th_p1_server.join();
    th_p1_client.join();
    arg_map["PORT"] = std::to_string(p2->server_port);
    th_p2_client = std::thread(&Peer::RFC_Client::request, p2_client, "Stop_Peer", arg_map);
    th_p2_server.join();
    th_p2_client.join();
    arg_map["PORT"] = std::to_string(p3->server_port);
    th_p3_client = std::thread(&Peer::RFC_Client::request, p3_client, "Stop_Peer", arg_map);
    th_p3_server.join();
    th_p3_client.join();
    arg_map["PORT"] = std::to_string(p4->server_port);
    th_p4_client = std::thread(&Peer::RFC_Client::request, p4_client, "Stop_Peer", arg_map);
    th_p4_server.join();
    th_p4_client.join();
    arg_map["PORT"] = std::to_string(p5->server_port);
    th_p5_client = std::thread(&Peer::RFC_Client::request, p5_client, "Stop_Peer", arg_map);
    th_p5_server.join();
    th_p5_client.join();
    
    write_download_times(p0->peer_name, p0->get_download_times());
    write_download_times(p1->peer_name, p1->get_download_times());
    write_download_times(p2->peer_name, p2->get_download_times());
    write_download_times(p3->peer_name, p3->get_download_times());
    write_download_times(p4->peer_name, p4->get_download_times());
    write_download_times(p5->peer_name, p5->get_download_times());
    
    delete p0;
    delete p1;
    delete p2;
    delete p3;
    delete p4;
    delete p5;
}

// /Users/liam_adams/my_repos/csc573/project1/rfc_files/rfc8598.txt
int main() {
    RegistrationServer *rs = new RegistrationServer();
    std::thread rs_thread(&RegistrationServer::start_server, rs);
    test_task();
    //task1();
    //task2();
    
    rs_thread.join();
    delete rs;
    return 0;
}
