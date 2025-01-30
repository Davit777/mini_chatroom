#include <boost/asio.hpp>
#include <boost/asio/ts/buffer.hpp>
#include <boost/asio/ts/internet.hpp>
#include <iostream>

int main()
{
    boost::system::error_code ec;
    boost::asio::io_context context;
    boost::asio::ip::tcp::endpoint endpoint(boost::asio::ip::make_address("51.38.81.49", ec), 80);
    boost::asio::ip::tcp::socket socket(context);
    socket.connect(endpoint, ec);
    if (!ec) {
        std::cout << "Conected!" << std::endl;
    } else {
        std::cout << "Failed to connect to address: " << ec.message() << std::endl;
    }
    if (socket.is_open()) {
        const std::string httpHeader =
              "GET /index.html HTTP/1.1\r\n"
              "Host: example.com\r\n"
              "Connection: close\r\n\r\n";

        socket.write_some(boost::asio::buffer(httpHeader.data(), httpHeader.size()), ec);

        socket.wait(socket.wait_read);

        const std::size_t bytes = socket.available();
        std::cout << "Bytes Available: " << bytes << std::endl;

        if (bytes > 0) {
            std::vector<char> buffer(bytes);
            socket.read_some(boost::asio::buffer(buffer.data(), buffer.size()), ec);
            for (char c : buffer) {
                std::cout << c;
            }
        }

    }
    return 0;
}

