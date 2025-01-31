#ifndef __NET_MESSAGE_HPP__
#define __NET_MESSAGE_HPP__

#include "net_common.hpp"

namespace net
{

template <typename T>
struct message_header
{
    T id {};
    uint32_t size = 0;
};

template <typename T>
struct message
{
    message_header<T> header {};
    std::vector<uint8_t> body;

    [[nodiscard]] std::size_t size() const noexcept { return sizeof(message_header<T>) + body.size(); }

    friend std::ostream& operator<<(std::ostream& os, const message<T>& msg)
    {
        os << "ID: " << int(msg.header.id) << " Size: " << msg.header.size;
        return os;
    }

    template <typename DataType>
    friend message<T>& operator<<(message<T>& msg, const DataType& data)
    {
        static_assert(std::is_standard_layout_v<DataType>, "Data is too complex to be pushed into vector.");
        const std::size_t old_size = msg.body.size();
        msg.body.resize(old_size + sizeof(DataType));
        std::memcpy(msg.body.data() + old_size, &data, sizeof(DataType));
        msg.header.size = msg.size();
        return msg;
    }

    template <typename DataType>
    friend message<T>& operator>>(message<T>& msg, DataType& data)
    {
        static_assert(std::is_standard_layout_v<DataType>, "Data is too complex to be pulled from vector.");
        const std::size_t new_size = msg.body.size() - sizeof(DataType);
        std::memcpy(&data, msg.body.data() + new_size, sizeof(DataType));
        msg.body.resize(new_size);
        msg.header.size = msg.size();
        return msg;
    }
};

template <typename T> class connection;

template <typename T>
struct owned_message
{
    std::shared_ptr<connection<T>> remote = nullptr;
    message<T> msg;

    friend std::ostream& operator<<(std::ostream& os, const owned_message<T>& msg)
    {
        oss << msg;
        return os;
    }
};

} // namespace net

#endif // __NET_MESSAGE_HPP__

