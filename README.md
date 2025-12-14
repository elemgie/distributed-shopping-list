# SDLE Second Assignment

SDLE Second Assignment of group T1G06.

Group members:

1. Mateusz Gieroba (up202501728@fe.up.pt)
2. Jakub Popiel    (up202501735@fe.up.pt)
3. &lt;first name&gt; &lt;family name&gt; (&lt;email address&gt;)

## Description

This is the local-first application for creating simple shopping lists. It consists of web client along with local server to handle its requests, as well as a cloud component of distributed servers that allow for concurrent work on these lists.

**TO BE ELABORATED ON**

## Requirements

To properly compile and run this project you will need libraries like `pistache`, `cppzmq`, `sqlite3` and `msgpack-c`.

## Running application

### Local client

To compile local client run in repository's main catalogue ```make client```

Then run `client.out [port]` specifying the listening port for the browser UI (for example 9080) and open `web/index.html` in your browser.

Sanitization options for the compiler are required because there is some (yet) undiscovered bug with Pistache's use in this code that leads to stack smashing. Oddly, with these options, the bug doesn't crash the program.

### Cloud server

To compile src/node/cluster.cpp (a simple cluster with two shards and a replication factor of three) run (in main catalogue): ```make cluster```

Then run `cluster.out`

## Interactive cloud server

Use

```
g++ -g -O0 -fsanitize=address -fno-omit-frame-pointer --std=c++20 src/model/shopping_item.cpp src/model/shopping_list.cpp   src/persistence/sqlite_db.cpp src/node/node.cpp src/util.cpp src/message/message.cpp src/cluster.cpp -Isrc -Imsgpack-c/include -lzmq -lsqlite3 -pthread -o backend
```

to compile, then run `backend`

### Cleaning

To remove effects of previous compilations run ```make clean```.
If you also want to erase state of the app use ```make clean-all```.