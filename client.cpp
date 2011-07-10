#include <iostream>
#include <string>
#include "shared.h"
#include "connection.h"

int main(int argc, char * argv[])
{
    try {
        Connection::RPCInvoker invoker;
        invoker.registerFunction("foo", foo);

        boost::asio::io_service ioService;
        Connection::pointer connection = Connection::connect(ioService, invoker, "localhost", "2000");
        connection->execute("foo", foo, 5);

        ioService.run();
    } catch (const std::exception & e) {
        std::cerr << e.what() << std::endl;
    }

    return 0;
}
