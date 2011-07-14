#include <iostream>
#include "shared.h"
#include "server.h"
#include "connection.h"

const PAN_CHAR_T PANTHEIOS_FE_PROCESS_IDENTITY[] = "game";

int main(int argc, char * argv[])
{
    bool runServer = false;
    bool connectToServer = false;

    LOG_DEBUG("Entering program");

    switch (atoi(argv[1])) {
        case 0:
            LOG_DEBUG("Dedicated server mode chosen");
            runServer = true;
            break;
        case 1:
            LOG_DEBUG("Dedicated client mode chosen");
            connectToServer = true;
            break;
        case 2:
            LOG_DEBUG("Client and server mode chosen");
            runServer = true;
            connectToServer = true;
            break;
        default:
            LOG_DEBUG("No connection mode chosen");
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
            connection->execute(SERVER_RPC(sendMessage), "FOO!");
        }
        
        if (!runServer && !connectToServer) {
            Connection::pointer connection{Connection::fake(ioService, rpcInvoker)};
            connection->execute(SERVER_RPC(sendMessage), "FIRST!");
        }

        ioService.run();
    } catch (const boost::system::system_error & e) {
        LOG_CRITICAL("System error (", e.code(), "): ", e.what());
    } catch (const std::exception & e) {
        LOG_ALERT("Exception: ", e.what());
    } catch (...) {
        LOG_EMERGENCY("Unknown exception");
    }

    LOG_DEBUG("Exiting program");

    return 0;
}
