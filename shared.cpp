#include "shared.h"

#include <iostream>

void foo(int x, Connection::pointer connection)
{
    std::cout << x << std::endl;
    if (x < 100) {
        connection->execute(CLIENT_RPC(foo), x+1);
    }
}

Connection::RPCInvoker RPCMethods()
{
    Connection::RPCInvoker invoker;
    invoker.registerFunction(CLIENT_RPC(foo));
    return invoker;
}
