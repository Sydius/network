#include <iostream>
#include <pantheios/pantheios.hpp>
#include <pantheios/frontends/stock.h>
#include <pantheios/inserters/args.hpp>
#include <pantheios/inserters/integer.hpp>
#include <pantheios/inserters/exception.hpp>
#include "shared.h"
#include "connection.h"
#include "server.h"

const PAN_CHAR_T PANTHEIOS_FE_PROCESS_IDENTITY[] = "game";

int main(int argc, char * argv[])
{
    bool runServer = false;
    bool connectToServer = false;

    pantheios::log_DEBUG("Entering program (", pantheios::args(argc, argv, pantheios::args::arg0FileOnly), ")");

    switch (atoi(argv[1])) {
        case 0:
            pantheios::log_DEBUG("Dedicated server mode chosen");
            runServer = true;
            break;
        case 1:
            pantheios::log_DEBUG("Dedicated client mode chosen");
            connectToServer = true;
            break;
        case 2:
            pantheios::log_DEBUG("Client and server mode chosen");
            runServer = true;
            connectToServer = true;
            break;
        default:
            pantheios::log_DEBUG("No connection mode chosen");
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
        pantheios::log_ALERT("System error (", e.code().category().name(), ":", pantheios::integer(e.code().value()), "): ", pantheios::exception(e));
    } catch (const std::exception & e) {
        pantheios::log_CRITICAL("Exception: ", pantheios::exception(e));
    } catch (...) {
        pantheios::log_EMERGENCY("Unknown exception");
    }

    pantheios::log_DEBUG("Exiting program");

    return 0;
}
