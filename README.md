# SDLE Second Assignment

SDLE Second Assignment of group T1G06.

Group members:

1. Mateusz Gieroba (up202501728@fe.up.pt)
2. Jakub Popiel    (up202501735@fe.up.pt)
3. &lt;first name&gt; &lt;family name&gt; (&lt;email address&gt;)

## Description

This is the local-first application for creating simple shopping lists. It consists of web client along with local server to handle its requests, as well as a cloud component of distributed servers that allow for concurrent work on these lists.

**TO BE ELABORATED ON**

## Running application

### Local client

To compile local client run in repository's main catalogue ```g++ -g -O0 -fsanitize=address -fno-omit-frame-pointer --std=c++20   src/client/client.cpp src/client/api.cpp src/client/request_handler.cpp   src/model/shopping_item.cpp src/model/shopping_list.cpp   src/persistence/sqlite_db.cpp src/util.cpp -Isrc -Imsgpack-c/include -lpistache -lzmq -lsqlite3 -pthread -o client.out```

Then run `client.out` and open `web/index.html` in your browser.

You may need to install some required libraries like `pistache`, `cppzmq`, `sqlite3` and `msgpack-c`.

Sanitization options for the compiler are required because there is some (yet) undiscovered bug with Pistache's use in this code that leads to stack smashing. Oddly, with these options, the bug doesn't crash the program.

### Cloud server

To compile src/node/cluster.cpp (a simple cluster with two shards and a replication factor of three) run (in main catalogue):

```g++ -g -O0 -fsanitize=address -fno-omit-frame-pointer --std=c++20 src/model/shopping_item.cpp src/model/shopping_list.cpp   src/persistence/sqlite_db.cpp src/node/node.cpp src/util.cpp src/message/message.cpp src/node/cluster.cpp -Isrc -Imsgpack-c/include -lzmq -lsqlite3 -pthread -o cluster.out```

Then run `cluster.out`
