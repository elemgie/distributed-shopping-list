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
        ENSURE_LIST = 1, // used both for create and update
        DELETE_LIST = 2,
        GET_LIST = 3,
        LIST_RESPONSE = 4, // field is filled with valid data
        NO_LIST_RESPONSE = 5, // used if list not found or deleted
        GOSSIP_LISTS = 6,
    };

    struct Message
    {
        OpType op;
        std::string origin;
        uint64_t ts;
        std::vector<ShoppingList> lists;

        MSGPACK_DEFINE(op, origin, ts, lists);

        static Message ensure_list(const std::string& origin, uint64_t ts,
                                   const ShoppingList& list);

        static Message delete_list(const std::string& origin, uint64_t ts,
                                   const string& listUrl);

        static Message get_list(const std::string &origin, uint64_t ts,
                                const string& listUrl);

        static Message list_response(bool isValidList, const std::string &origin, uint64_t ts,
                                     const ShoppingList &list);

        static Message gossip_lists(const std::string& origin, uint64_t ts,
                        const std::vector<ShoppingList>& lists);

        zmq::message_t to_zmq() const;
        static Message from_zmq(const zmq::message_t &frame);
    };

}

MSGPACK_ADD_ENUM(message::OpType);

#endif
