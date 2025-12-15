#ifndef MESSAGE_HPP
#define MESSAGE_HPP

#include <string>
#include <cstdint>
#include <msgpack.hpp>
#include "../model/shopping_list.hpp"
#include "../model/shopping_item.hpp"
#include <zmq.hpp>

namespace message
{

    enum class OpType : uint8_t
    {
        ENSURE_LIST = 1,
        DELETE_LIST = 2,
        GET_LIST = 3,
        LIST_RESPONSE = 4,
        NO_LIST_RESPONSE = 5,
        GOSSIP_LISTS = 6,
        GOSSIP_NODES = 7,
        GET_NODES = 8,
        NODES_RESPONSE = 9
    };

    struct NodeInfo {
        std::string nodeId;
        std::string host;
        int shardId;
        int clientPort;
        int gossipPullPort;
        int discoveryPullPort;
        uint64_t lastSeenTs = 0;

        MSGPACK_DEFINE(nodeId, host, shardId, clientPort, gossipPullPort, discoveryPullPort, lastSeenTs);
    };

    struct Message
    {
        OpType op;
        std::string origin;
        uint64_t ts;
        std::vector<ShoppingList> lists;
        std::vector<NodeInfo> nodes;

        MSGPACK_DEFINE(op, origin, ts, lists, nodes);

        static Message ensure_list(const std::string& origin, uint64_t ts,
                                   const ShoppingList& list);

        static Message delete_list(const std::string& origin, uint64_t ts,
                                   const std::string& listUrl);

        static Message get_list(const std::string &origin, uint64_t ts,
                                const std::string& listUrl);

        static Message list_response(bool isValidList, const std::string &origin, uint64_t ts,
                                     const ShoppingList &list);

        static Message gossip_lists(const std::string& origin, uint64_t ts,
                                    const std::vector<ShoppingList>& lists);

        static Message gossip_nodes(const std::string& origin, uint64_t ts, const std::vector<NodeInfo>& nodes);

        static Message get_nodes(const std::string& origin, uint64_t ts);

        static Message nodes_response(const std::string& origin, uint64_t ts,
                                      const std::vector<NodeInfo>& nodes);

        zmq::message_t to_zmq() const;
        static Message from_zmq(const zmq::message_t &frame);
    };

}

MSGPACK_ADD_ENUM(message::OpType);

#endif
