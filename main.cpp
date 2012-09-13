/*
Copyright 2011 Christopher Allen Ogden. All rights reserved.

Redistribution and use in source and binary forms, with or without modification, are
permitted provided that the following conditions are met:

   1. Redistributions of source code must retain the above copyright notice, this list of
      conditions and the following disclaimer.

   2. Redistributions in binary form must reproduce the above copyright notice, this list
      of conditions and the following disclaimer in the documentation and/or other materials
      provided with the distribution.

THIS SOFTWARE IS PROVIDED BY CHRISTOPHER ALLEN OGDEN ``AS IS'' AND ANY EXPRESS OR IMPLIED
WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND
FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL CHRISTOPHER ALLEN OGDEN OR
CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

The views and conclusions contained in the software and documentation are those of the
authors and should not be interpreted as representing official policies, either expressed
or implied, of Christopher Allen Ogden.
*/
#include <iostream>
#include <chrono>

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
        SydNet::IOService ioService;
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

        auto time = std::chrono::monotonic_clock::now();
        while (true) {
            static const auto duration = std::chrono::milliseconds(500);
            if (std::chrono::monotonic_clock::now() - time > duration) {
                time += duration;

                if (server) {
                    for (auto & client: server->clients()) {
                        SydNet::Connection::Pointer{client.second}->execute(CLIENT_RPC(printMessage), "Tick!");
                    }
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
