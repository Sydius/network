#include "shared.h"

#include <iostream>
#include <boost/lexical_cast.hpp>

void printMessage(const std::string & message, Connection::Pointer connection)
{
    std::cout << message << std::endl;
}

void sendMessage(const std::string & message, Connection::Pointer connection)
{
    for (auto & peer: connection->peers()) {
        Connection::Pointer{peer.second}->execute(CLIENT_RPC(printMessage), boost::lexical_cast<std::string>(connection->uuid()) + ": " + message);
    }
}

Connection::RPCInvoker RPCMethods()
{
    Connection::RPCInvoker invoker;
    invoker.registerFunction(CLIENT_RPC(printMessage));
    invoker.registerFunction(SERVER_RPC(sendMessage));
    return invoker;
}
