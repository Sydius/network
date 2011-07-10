#include <iostream>
#include <string>
#include "shared.h"
#include "connection.h"

int main(int argc, char * argv[])
{
    try {
        Connection::IOService ioService;
        Connection::pointer connection = Connection::connect(ioService, RPCMethods(), "localhost", "2000");
        connection->execute(CLIENT_RPC(foo), 5);

        ioService.run();
    } catch (const std::exception & e) {
        std::cerr << e.what() << std::endl;
    }

    return 0;
}
