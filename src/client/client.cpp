#include "pistache/endpoint.h"
#include "pistache/router.h"
#include "request_handler.cpp"
#include "api.hpp"

#include <csignal>

using namespace Pistache;
using namespace Pistache::Rest;

volatile std::sig_atomic_t stop_flag = 0;

void sigHandler(int signum) {
    std::cout << "Received interruption signal: " << signum << std::endl;
    stop_flag = 1;
}

int main() {
    signal(SIGINT, sigHandler);
    signal(SIGTERM, sigHandler);

    Address addr(Ipv4::any(), Port(9080));
    auto opts = Http::Endpoint::options().threads(1);
    SqliteDb db;
    db.init_db("shopping.db");
    API api(&db);

    Router router;

    Http::Endpoint server(addr);
    server.init(opts);
    server.setHandler(Http::make_handler<RequestHandler>(api, router));
    server.serveThreaded();

    std::cout << "Server is running at " << addr.host() << ":" << addr.port() << std::endl;\

    while (!stop_flag) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    std::cout << "Shutting down server..." << std::endl;
    server.shutdown();
    std::cout << "Server stopped." << std::endl;
    return 0;
}