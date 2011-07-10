#include <iostream>
#include <string>
#include <boost/asio.hpp>
#include "shared.h"
#include "invoke.h"
#include "connection.h"

int main(int argc, char * argv[])
{
    try {
        invoke::Invoker<> invoker;
        invoker.registerFunction("foo", foo);

        boost::asio::io_service ioService;
        Connection::pointer connection = Connection::connect(ioService, invoker, "localhost", "2000");
        connection->write(invoker.serialize("foo", foo, 5));

        ioService.run();
    } catch (const std::exception & e) {
        std::cerr << e.what() << std::endl;
    }

    return 0;
}
