#include <iostream>
#include <string>
#include <boost/asio.hpp>
#include "shared.h"
#include "invoke.h"
#include "connection.h"

int main(int argc, char * argv[])
{
    try {
        invoke::Invoker invoker;
        invoker.registerFunction("foo", foo);

        boost::asio::io_service ioService;
        Connection::pointer connection = Connection::create(ioService, invoker);
        connection->connect("localhost", "2000");
        connection->write(invoke::serialize("foo", foo, 5));

        ioService.run();
    } catch (const std::exception & e) {
        std::cerr << e.what() << std::endl;
    }

    return 0;
}
