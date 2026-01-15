# Local-first distributed shopping app

This project was created as a group final assignment for the Large-Scale Distributed Systems course at FEUP. The group members were Mateusz Gieroba (me), Jakub Popiel and Rodrigo Tsou. It has been developed by me since then.

## Description

This is the local-first application for creating simple shopping lists. It consists of web client along with local server to handle its requests, as well as a cloud component of distributed servers that allow for concurrent work on these lists. It focuses on data consistency and changes visibility on different machines aiming at eventual consistency for many nodes and users. Most of the work was focused on the backend distributed part, therefore the UI component is quite simple.

As for the network design system employs separate fully-connected networks for each shard (this is not the best solution though and will be changed). Each list-changing request coming to a node is propagated to some other nodes. Additionally, nodes randomly gossip their known lists to some other node in the network to achieve eventual consistency.

Nodes also maintain a list of other nodes in the network and dynamically manage it in case some node is disconnected or added to the network.

## Requirements

To properly compile and run this project you will need libraries like `pistache`, `cppzmq`, `sqlite3` and `msgpack-c`.

## Running application

### Local client

To compile local client run in repository's main catalogue ```make client```.

Then run `client.out [port]` specifying the listening port for the browser UI (for example 9080) and open `web/index.html` in your browser (remember to set the same port in the appropriate field in UI).

Sanitization options for the compiler are required because there is some (yet) undiscovered bug with Pistache's use in this code that leads to stack smashing. Oddly, with these options, the bug doesn't crash the program.

### Cloud server

To compile src/node/cluster.cpp (a simple cluster with two shards and a replication factor of three) run (in main catalogue): ```make cluster```.

Then run `cluster.out`

### Interactive cloud server

If you want to run an interactive version of the cloud servers allowing you to add nodes, remove them or simulate the network partition use ```make backend```.

Then run `backend.out` and work with the commands presented to you on the screen.

### Cleaning

To remove effects of previous compilations run ```make clean```.
If you also want to erase state of the app use ```make clean-all```.