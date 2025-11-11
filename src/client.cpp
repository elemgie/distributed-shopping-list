#include <zmq.hpp>
#include <iostream>
#include <string>

using namespace std;

int main() {
    zmq::context_t ctx{1};
    zmq::socket_t sock{ctx, zmq::socket_type::req};

    sock.connect("tcp://localhost:5555");
    cout << "Connected to server.\n";
    cout << "Commands:\n"
         << "  CREATE\n"
         << "  ADD <list_id> <item text>\n"
         << "  GET <list_id>\n"
         << "Type EXIT to quit.\n\n";

    while (true) {
        cout << "> ";
        string line;
        getline(cin, line);

        if (line == "EXIT" || cin.eof()) break;
        if (line.empty()) continue;

        sock.send(zmq::buffer(line), zmq::send_flags::none);

        zmq::message_t reply;
        auto ok = sock.recv(reply);
        if (!ok) continue;

        string rep_str(static_cast<char*>(reply.data()), reply.size());
        cout << rep_str << "\n";
    }

    return 0;
}
