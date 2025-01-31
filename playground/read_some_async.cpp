#include <boost/asio.hpp>
#include <boost/asio/ts/buffer.hpp>
#include <boost/asio/ts/internet.hpp>

#include <iostream>
#include <chrono>
#include <vector>
#include <thread>

std::vector<char> buffer(20 * 1024);

void grabSomeData(boost::asio::ip::tcp::socket& socket)
{
    socket.async_read_some(boost::asio::buffer(buffer.data(), buffer.size()),
        [&](std::error_code ec, std::size_t length) {
            if (!ec) {
                std::cout << "\n\nRead " << length << " bytes\n\n";
                for (std::size_t i = 0; i < length; ++i) {
                    std::cout << buffer[i];
                }
                grabSomeData(socket);
            }
        });
}

int main()
{
    boost::system::error_code ec;
    boost::asio::io_context context;
    boost::asio::io_context::work idleWork(context); 
    std::thread contextThread = std::thread([&] { context.run(); });
    boost::asio::ip::tcp::endpoint endpoint(boost::asio::ip::make_address("51.38.81.49", ec), 80);
    boost::asio::ip::tcp::socket socket(context);
    socket.connect(endpoint, ec);
    if (!ec) {
        std::cout << "Conected!" << std::endl;
    } else {
        std::cout << "Failed to connect to address: " << ec.message() << std::endl;
    }
    if (socket.is_open()) {
        grabSomeData(socket);
        const std::string httpHeader =
              "GET /index.html HTTP/1.1\r\n"
              "Host: example.com\r\n"
              "Connection: close\r\n\r\n";

        socket.write_some(boost::asio::buffer(httpHeader.data(), httpHeader.size()), ec);

        using namespace std::chrono_literals;
        std::this_thread::sleep_for(20000ms);

        context.stop();
        if (contextThread.joinable()) {
            contextThread.join();
        }
    }
    return 0;
}

