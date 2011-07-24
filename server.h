#pragma once

#include "connection.h"

class Server
{
    public:
        Server(const Connection::RPCInvoker & invoker)
            : _invoker(invoker)
        {
        }

        /**
         * Get the invoker used by the server
         *
         * @return  Invoker
         */
        Connection::RPCInvoker & invoker()
        {
            return _invoker;
        }

        /**
         * Get a map of the connections
         *
         * @return  Map containing the connections
         */
        virtual Connection::ConnectionMap & clients() = 0;

        virtual ~Server() {}
    private:
        Connection::RPCInvoker _invoker;
};
