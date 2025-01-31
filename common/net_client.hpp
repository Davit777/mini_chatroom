#ifndef __NET_CLIENT_HPP__
#define __NET_CLIENT_HPP__

#include "net_common.hpp"
#include "net_tsqueue.hpp"
#include "net_message.hpp"
#include "net_connection.hpp"

namespace net
{

template <typename T>
class client_interface
{
   client_interface() : socket_(context_) {}
   virtual ~client_interface() { disconnect(); }

public:
    bool connect(const std::string& host, const uint16_t port)
    {
        try {
            connection_ = std::make_unique<connection<T>>();
            boost::asio::ip::resolver resolver(context_);
            endpoitns_ = resolver.resolve(host, std::to_string(port));
            connection_->connect_to_server(endpoints_);
            context_thread_ = std::thread([this] { context_.start(); });
        } catch (std::exception& ex) {
            std::cerr << "Client exception: " << ex.what() << std::endl;
            return false;
        }
        return true;
    }

    void disconnect()
    {
        if (is_connected()) {
            connection_->disconnect();
        }
        context_.stop();
        if (context_thread_.joinable()) {
            context_thread_.join();
        }
        connection_.release();
    }

    bool is_connected()
    {
        return connection_ ? connection_->is_connected() : false;
    }

    tsqueue<owned_message<T>>& incomming_messages() { return in_message_queue_; }

protected:
    boost::asio::io_context context_;
    std::thread context_thread_;
    boost::asio::ip::tcp::socket socket_;
    std::unique_ptr<connection<T>> connection_;

private:
    tsqueue<owned_message<T>> in_message_queue_;
};

} // namespace net

#endif // __NET_CLIENT_HPP__

