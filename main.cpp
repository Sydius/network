#include <iostream>
#include <string>
#include "shared.h"
#include "connection.h"
#include "server.h"

int main(int argc, char * argv[])
{
    bool runServer = false;
    bool connectToServer = false;

    switch (atoi(argv[1])) {
        case 0:
            runServer = true;
            break;
        case 1:
            connectToServer = true;
            break;
        case 2:
            runServer = true;
            connectToServer = true;
            break;
    }

    try {
        Connection::IOService ioService;
        std::shared_ptr<Server> server;

        if (runServer) {
            server = std::shared_ptr<Server>{new Server(ioService, RPCMethods())};
        }

        if (connectToServer) {
            Connection::pointer connection = Connection::connect(ioService, RPCMethods(), "localhost", "2000");
            connection->execute(CLIENT_RPC(foo), 5);
        }

        ioService.run();
    } catch (const std::exception & e) {
        std::cerr << e.what() << std::endl;
    }

    return 0;
}
