#include <iostream>
#include "shared.h"
#include "real_server.h"
#include "fake_server.h"
#include "fake_connection.h"
#include "outgoing_connection.h"

#ifdef USE_PANTHEIOS
const PAN_CHAR_T PANTHEIOS_FE_PROCESS_IDENTITY[] = "game";
#endif

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
        SydNet::RealConnection::IOService ioService;
        SydNet::Connection::RPCInvoker rpcInvoker{RPCMethods()};
        std::shared_ptr<SydNet::Server> server;
        SydNet::Connection::Pointer connection;

        if (runServer) {
            server = std::shared_ptr<SydNet::Server>{new SydNet::RealServer(rpcInvoker, ioService, 2000)};
        } else if (!connectToServer) {
            server = std::shared_ptr<SydNet::Server>{new SydNet::FakeServer(rpcInvoker)};
        }

        if (connectToServer) {
            connection = SydNet::OutgoingConnection::create(rpcInvoker, ioService, "localhost", 2000);
        }

        int iTicks = 0;
        while (true) {
            if (iTicks++ % 1000000 == 0 && server) {
                for (auto & client: server->clients()) {
                    SydNet::Connection::Pointer{client.second}->execute(CLIENT_RPC(printMessage), "Tick!");
                }
            }
            ioService.poll();
        }
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
