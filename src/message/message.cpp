#include "message.hpp"
#include <msgpack.hpp>
#include <cstring>

namespace message
{
    Message Message::ensure_list(const std::string& origin, uint64_t ts, const ShoppingList& list)
    {
        Message m;
        m.op = OpType::ENSURE_LIST;
        m.origin = origin;
        m.ts = ts;
        m.lists = {list};
        return m;
    }

    Message Message::delete_list(const std::string& origin, uint64_t ts, const string& list_uid)
    {
        Message m;
        m.op = OpType::DELETE_LIST;
        m.origin = origin;
        m.ts = ts;
        m.lists = {ShoppingList(list_uid, "")};
        return m;
    }

    Message Message::get_list(const std::string& origin, uint64_t ts, const std::string& list_uid)
    {
        Message m;
        m.op = OpType::GET_LIST;
        m.origin = origin;
        m.ts = ts;
        m.lists = {ShoppingList(list_uid, "")};
        return m;
    }

    Message Message::list_response(bool isValidList, const std::string& origin, uint64_t ts,
                                   const ShoppingList& list)
    {
        Message m;
        m.op = isValidList ? OpType::LIST_RESPONSE : OpType::NO_LIST_RESPONSE;
        m.origin = origin;
        m.ts = ts;
        m.lists = {list};
        return m;
    }

    Message Message::gossip_lists(const std::string& origin, uint64_t ts,
                            const std::vector<ShoppingList>& lists)
    {
        Message m;
        m.op = OpType::GOSSIP_LISTS;
        m.origin = origin;
        m.ts = ts;
        m.lists = lists;
        return m;
    }

    Message Message::gossip_nodes(const std::string& origin, uint64_t ts,
                                  const std::vector<NodeInfo>& nodes)
    {
        Message m;
        m.op = OpType::GOSSIP_NODES;
        m.origin = origin;
        m.ts = ts;
        m.nodes = nodes;
        return m;
    }

    Message Message::get_nodes(const std::string& origin, uint64_t ts)
    {
        Message m;
        m.op = OpType::GET_NODES;
        m.origin = origin;
        m.ts = ts;
        return m;
    }

    Message Message::nodes_response(const std::string& origin, uint64_t ts,
                                    const std::vector<NodeInfo>& nodes)
    {
        Message m;
        m.op = OpType::NODES_RESPONSE;
        m.origin = origin;
        m.ts = ts;
        m.nodes = nodes;
        return m;
    }

    zmq::message_t Message::to_zmq() const
    {
        msgpack::sbuffer buf;
        msgpack::pack(buf, *this);
        zmq::message_t fm(buf.size());
        std::memcpy(fm.data(), buf.data(), buf.size());
        return fm;
    }

    Message Message::from_zmq(const zmq::message_t &frame)
    {
        auto oh = msgpack::unpack(static_cast<const char *>(frame.data()), frame.size());
        Message m;
        oh.get().convert(m);
        return m;
    }

}
