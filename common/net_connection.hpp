#ifndef __NET_CONNECTION_HPP__
#define __NET_CONNECTION_HPP__

#include "net_common.hpp"
#include "net_tsqueue.hpp"
#include "net_message.hpp"

namespace net
{

template <typename T>
class connection : std::enable_shared_from_this<connection<T>>
{
public:
    connection() {}

    virtual ~connection() {}

    bool connect_to_server();
    bool disconnect();
    bool is_connected() const;

    bool send(const message<T>& msg);

private:
    boost::asio::ip::tcp::socket socket_;
    boost::asio::io_context& context_;

    tsqueue<message<T>> out_message_queue_;
    tsqueue<owned_message<T>>& in_message_queue_;
};

} // namespace net

#endif // __NET_CONNECTION_HPP__

