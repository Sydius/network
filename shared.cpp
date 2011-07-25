#include "shared.h"

#include <iostream>
#include <boost/lexical_cast.hpp>

void printMessage(const std::string & message, SydNet::Connection::Pointer connection)
{
    std::cout << message << std::endl;
    if (message == "Tick!") {
        connection->execute(SERVER_RPC(gotMessage));
    }
}

void sendMessage(const std::string & message, SydNet::Connection::Pointer connection)
{
    for (auto & peer: connection->peers()) {
        SydNet::Connection::Pointer{peer.second}->execute(CLIENT_RPC(printMessage), boost::lexical_cast<std::string>(connection->uuid()) + ": " + message);
    }
}

void gotMessage(SydNet::Connection::Pointer connection)
{
    std::cout << "Got message" << std::endl;
    connection->execute(CLIENT_RPC(printMessage), "OK!");
    connection->execute(CLIENT_RPC(printMessage), "ONE!");
    connection->execute(CLIENT_RPC(printMessage), "TWO!");
    connection->execute(CLIENT_RPC(printMessage), "THREE!");
}

SydNet::Connection::RPCInvoker RPCMethods()
{
    SydNet::Connection::RPCInvoker invoker;
    invoker.registerFunction(CLIENT_RPC(printMessage));
    invoker.registerFunction(SERVER_RPC(sendMessage));
    invoker.registerFunction(SERVER_RPC(gotMessage));
    return invoker;
}
