#pragma once

#include <unordered_map>
#include "real_connection.h"

class Server
{
    public:
        /**
         * Create a server instance and start accepting connections.
         *
         * @param ioService IO service to use for this server
         * @param invoker   RPC method invoker to use with this server
         * @param port      port to listen on
         */
        Server(RealConnection::IOService & ioService, const Connection::RPCInvoker & invoker, unsigned short port)
            : _acceptor{ioService, boost::asio::ip::tcp::endpoint{boost::asio::ip::tcp::v4(), port}}
            , _invoker{invoker}
            , _uuidGen{}
            , _connections{}
        {
            startAccept(); // Start accepting connections immediately
            LOG_NOTICE("Accepting connections at ", _acceptor.local_endpoint());
        }

    private:
        void startAccept()
        {
            // Prepare a new connection to accept onto
            Connection::Pointer newConnection = RealConnection::incoming(_acceptor.io_service(), _invoker, _uuidGen(), &_connections);

            // Wait for one to accept (will call handleAccept)
            _acceptor.async_accept(std::static_pointer_cast<RealConnection>(newConnection)->socket(),
                std::bind(&Server::handleAccept, this, newConnection, std::placeholders::_1));
        }

        void handleAccept(Connection::Pointer newConnection, const boost::system::error_code & error)
        {
            if (error) {
                throw boost::system::system_error{error};
            }

            LOG_NOTICE("Client connected: ", std::static_pointer_cast<RealConnection>(newConnection)->socket().remote_endpoint(), " ", newConnection->uuid());
            _connections[newConnection->uuid()] = newConnection;

            // Begin reading on the new connection
            std::static_pointer_cast<RealConnection>(newConnection)->beginReading(std::bind(&Server::handleDisconnect, this, newConnection, std::placeholders::_1));

            // Wait for the next connection
            startAccept();
        }

        void handleDisconnect(Connection::Pointer connection, const boost::system::error_code & error)
        {
            LOG_NOTICE("Client disconnected: ", std::static_pointer_cast<RealConnection>(connection)->socket().remote_endpoint(), " ", connection->uuid());
            _connections.erase(connection->uuid());
        }

        boost::asio::ip::tcp::acceptor _acceptor;
        Connection::RPCInvoker _invoker;
        boost::uuids::random_generator _uuidGen;
        Connection::ConnectionMap _connections;
};
