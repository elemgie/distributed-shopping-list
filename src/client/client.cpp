#include "pistache/endpoint.h"
#include "pistache/router.h"
#include "request_handler.cpp"
#include "api.hpp"

#include <csignal>

using namespace Pistache;
using namespace Pistache::Rest;

volatile std::sig_atomic_t stop_flag = 0;

void sigHandler(int signum) {
    stop_flag = 1;
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "No communication port provided" << std::endl;
        return 1;
    }
    signal(SIGINT, sigHandler);
    signal(SIGTERM, sigHandler);

    std::string host = "127.0.0.1";
    int port = std::stoi(argv[1]);
    Address addr(host, Port(port));
    auto opts = Http::Endpoint::options().threads(1);

    SqliteDb db;
    db.init_db("db/shopping" + std::to_string(port) + ".db");

    API api(&db, host + ":" + std::to_string(port), 150);

    Router router;

    Http::Endpoint server(addr);
    server.init(opts);
    server.setHandler(Http::make_handler<RequestHandler>(api, router));
    server.serveThreaded();

    thread gossipThread = thread([&api]() {
        while (!stop_flag) {
            std::this_thread::sleep_for(std::chrono::seconds(3));
            api.gossipState();
        }
    });

    thread nodeUpdateThread = thread([&api]() {
        while (!stop_flag) {
            std::this_thread::sleep_for(std::chrono::seconds(5));
            api.updateCloudNodes();
        }
    });

    std::cout << "Server is running at " << addr.host() << ":" << addr.port() << std::endl;\

    while (!stop_flag) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    std::cout << "\nShutting down server..." << std::endl;
    if (gossipThread.joinable()) {
        gossipThread.join();
    }
    if (nodeUpdateThread.joinable()) {
        nodeUpdateThread.join();
    }
    server.shutdown();
    std::cout << "Server stopped." << std::endl;
    return 0;
}