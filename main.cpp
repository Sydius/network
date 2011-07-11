#include <iostream>
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
        Connection::RPCInvoker rpcInvoker{RPCMethods()};
        std::shared_ptr<Server> server;

        if (runServer) {
            server = std::shared_ptr<Server>{new Server(ioService, rpcInvoker, 2000)};
        }

        if (connectToServer) {
            Connection::pointer connection{Connection::connect(ioService, rpcInvoker, "localhost", 2000)};
            connection->execute(SERVER_RPC(pong), 5, "FOO!");
        }
        
        if (!runServer && !connectToServer) {
            Connection::pointer connection{Connection::create(ioService, rpcInvoker)};
            connection->execute(SERVER_RPC(pong), 3, "FIRST!");
        }

        ioService.run();
    } catch (const boost::system::system_error & e) {
        std::cerr << "System error (" << e.code() << "): " << e.what() << std::endl;
    } catch (const std::exception & e) {
        std::cerr << "Uncaught exception: " << e.what() << std::endl;
    }

    return 0;
}
