#pragma once

#include "connection.h"

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
        Server(Connection::IOService & ioService, const Connection::RPCInvoker & invoker, unsigned short port)
            : _acceptor(ioService, boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(), port))
            , _invoker(invoker)
        {
            startAccept(); // Start accepting connections immediately
            LOG_NOTICE("Accepting connections at ", _acceptor.local_endpoint());
        }

    private:
        void startAccept()
        {
            // Prepare a new connection to accept onto
            Connection::pointer newConnection = Connection::create(_acceptor.io_service(), _invoker, _uuidGen());

            // Wait for one to accept (will call handleAccept)
            _acceptor.async_accept(newConnection->socket(),
                std::bind(&Server::handleAccept, this, newConnection, std::placeholders::_1));
        }

        void handleAccept(Connection::pointer newConnection, const boost::system::error_code & error)
        {
            if (error) {
                throw boost::system::system_error(error);
            }

            LOG_NOTICE("Client connected: ", newConnection->socket().remote_endpoint(), " ", newConnection->uuid());

            // Begin reading on the new connection
            newConnection->beginReading(std::bind(&Server::handleDisconnect, this, newConnection, std::placeholders::_1));

            // Wait for the next connection
            startAccept();
        }

        void handleDisconnect(Connection::pointer connection, const boost::system::error_code & error)
        {
            LOG_NOTICE("Client disconnected: ", connection->socket().remote_endpoint(), " ", connection->uuid());
        }

        boost::asio::ip::tcp::acceptor _acceptor;
        Connection::RPCInvoker _invoker;
        boost::uuids::random_generator _uuidGen;
};
