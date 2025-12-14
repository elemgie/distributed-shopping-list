client:
	g++ -g -O0 -fsanitize=address -fno-omit-frame-pointer --std=c++20   src/client/client.cpp src/client/api.cpp src/client/request_handler.cpp   src/model/shopping_item.cpp src/model/shopping_list.cpp   src/persistence/sqlite_db.cpp src/util.cpp src/message/message.cpp -Isrc -Imsgpack-c/include -lpistache -lzmq -lsqlite3 -pthread -o client.out

cluster:
	g++ -g -O0 -fsanitize=address -fno-omit-frame-pointer --std=c++20 src/model/shopping_item.cpp src/model/shopping_list.cpp   src/persistence/sqlite_db.cpp src/node/node.cpp src/util.cpp src/message/message.cpp src/node/cluster.cpp -Isrc -Imsgpack-c/include -lzmq -lsqlite3 -pthread -o cluster.out

clean:
	rm -f *.out

clean-all:
	rm -f *.out
	rm -r db/*.db