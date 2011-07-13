#include "shared.h"

#include <iostream>

void printMessage(const std::string & message, Connection::pointer connection)
{
    std::cout << message << std::endl;
}

void sendMessage(const std::string & message, Connection::pointer connection)
{
    for (auto & peer: connection->peers()) {
        peer.second->execute(CLIENT_RPC(printMessage), boost::lexical_cast<std::string>(connection->uuid()) + ": " + message);
    }
}

Connection::RPCInvoker RPCMethods()
{
    Connection::RPCInvoker invoker;
    invoker.registerFunction(CLIENT_RPC(printMessage));
    invoker.registerFunction(SERVER_RPC(sendMessage));
    return invoker;
}
