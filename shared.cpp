#include "shared.h"

#include <iostream>

void ping(int x, Connection::pointer connection)
{
    std::cout << "Ping: " << x << std::endl;
    if (x < 10) {
        connection->execute(SERVER_RPC(pong), x+1, "HELLO");
    } else {
        //connection->disconnect();
    }
}

void pong(int x, const std::string & message, Connection::pointer connection)
{
    std::cout << "Pong: " << x << ":" << message << std::endl;
    connection->execute(CLIENT_RPC(ping), x+1);

    for (auto & other: connection->connections()) {
        other.second->execute(CLIENT_RPC(ping), x+1);
    }
}

Connection::RPCInvoker RPCMethods()
{
    Connection::RPCInvoker invoker;
    invoker.registerFunction(CLIENT_RPC(ping));
    invoker.registerFunction(SERVER_RPC(pong));
    return invoker;
}
