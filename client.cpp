#include <iostream>
#include <string>
#include <boost/asio.hpp>
#include "shared.h"
#include "invoke.h"
#include "connection.h"

int main(int argc, char * argv[])
{
    invoke::Invoker invoker;
    invoker.registerFunction("foo", foo);
    std::string message = invoke::serialize("foo", foo, 5) + '\n';

    boost::asio::io_service ioService;
    Connection::pointer connection = Connection::create(ioService, invoker);
    connection->connect("localhost", "2000");
    connection->write(message);

    ioService.run();

    return 0;
}
