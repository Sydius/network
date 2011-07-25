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

int mul(int x, SydNet::Connection::Pointer connection)
{
    return x * 5;
}

static void gotMessageResult(int result, int add)
{
    std::cout << result + add << std::endl;
}

void gotMessage(SydNet::Connection::Pointer connection)
{
    std::cout << "Got message" << std::endl;
    connection->executeCallback(CLIENT_RPC(mul), std::bind(gotMessageResult, std::placeholders::_1, 2), 3);
}

SydNet::Connection::RPCInvoker RPCMethods()
{
    SydNet::Connection::RPCInvoker invoker;
    invoker.registerFunction(CLIENT_RPC(printMessage));
    invoker.registerFunction(SERVER_RPC(sendMessage));
    invoker.registerFunction(SERVER_RPC(gotMessage));
    invoker.registerFunction(CLIENT_RPC(mul));
    return invoker;
}
