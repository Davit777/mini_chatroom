#include "common/net.hpp"

#include <iostream>

enum class CustomMessageTypes : uint32_t
{
    FireBullet,
    MovePlayer
};

class CustomClient : public net::client_interface<CustomMessageTypes>
{
public:
    bool fireBullet(const float x, const float y)
    {
        net::message<CustomMessageTypes> message;
        message.header.id = CustomMessageTypes::FireBullet;
        message << x << y;
        send(message);
    }
};

int main()
{
    CustomClient client;
    client.connect("example.game.com", 60000);
    client.fireBullet(1.2f, 3.4f):

    return 0;
}

