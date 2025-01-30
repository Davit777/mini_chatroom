#include <boost/asio.hpp>
#include <boost/asio/ts/buffer.hpp>
#include <boost/asio/ts/internet.hpp>
#include <iostream>

int main()
{
    boost::system::error_code ec;
    boost::asio::io_context context;
    boost::asio::ip::tcp::endpoint endpoint(boost::asio::ip::make_address("93.184.216.34", ec), 80);
    boost::asio::ip::tcp::socket socket(context);
    socket.connect(endpoint, ec);
    if (!ec) {
        std::cout << "Conected!" << std::endl;
    } else {
        std::cout << "Failed to connect to address: " << ec.message() << std::endl;
    }
    return 0;
}

