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
#pragma once

#include "server.h"

#include <unordered_map>
#include "incoming_connection.h"

namespace SydNet {

class RealServer: public Server
{
    public:
        /**
         * Create a server instance and start accepting connections.
         *
         * @param invoker   RPC method invoker to use with this server
         * @param ioService IO service to use for this server
         * @param port      port to listen on
         */
        RealServer(const Connection::RPCInvoker & invoker, IOService & ioService, unsigned short port)
            : Server{invoker}
            , _acceptor{ioService, boost::asio::ip::tcp::endpoint{boost::asio::ip::tcp::v4(), port}}
            , _uuidGen{}
            , _connectionMap{}
        {
            startAccept(); // Start accepting connections immediately
            LOG_NOTICE("Accepting connections at ", _acceptor.local_endpoint());
        }

        Connection::ConnectionMap & clients()
        {
            return _connectionMap;
        }

    private:
        void startAccept()
        {
            // Prepare a new connection to accept onto
            Connection::Pointer newConnection = IncomingConnection::create(invoker(), _acceptor.io_service(), _uuidGen(), &_connectionMap);

            // Wait for one to accept (will call handleAccept)
            _acceptor.async_accept(std::static_pointer_cast<IncomingConnection>(newConnection)->socket(),
                std::bind(&RealServer::handleAccept, this, newConnection, std::placeholders::_1));
        }

        void handleAccept(Connection::Pointer newConnection, const boost::system::error_code & error)
        {
            if (error) {
                throw boost::system::system_error{error};
            }

            LOG_NOTICE("Client connected: ", std::static_pointer_cast<IncomingConnection>(newConnection)->socket().remote_endpoint(), " ", newConnection->uuid());
            _connectionMap[newConnection->uuid()] = newConnection;

            // Begin reading on the new connection
            std::static_pointer_cast<IncomingConnection>(newConnection)->beginReading(
                    std::bind(&RealServer::handleDisconnect, this, newConnection, std::placeholders::_1));

            // Wait for the next connection
            startAccept();
        }

        void handleDisconnect(Connection::Pointer connection, const boost::system::error_code & error)
        {
            LOG_NOTICE("Client disconnected: ", std::static_pointer_cast<IncomingConnection>(connection)->socket().remote_endpoint(), " ", connection->uuid());
            _connectionMap.erase(connection->uuid());
        }

        boost::asio::ip::tcp::acceptor _acceptor;
        boost::uuids::random_generator _uuidGen;
        Connection::ConnectionMap _connectionMap;
};

}
