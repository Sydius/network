#pragma once

#include "real_connection.h"

class OutgoingConnection: public RealConnection
{

    public:
        /**
         * Create a new connection to a remote server
         *
         * @param invoker   RPC invoker to use with this connection
         * @param ioService IOService to use
         * @param hostname  Host name to connect to
         * @param port      Port to connect to
         * @return          A shared Pointer to a new connection object
         */
        static Connection::Pointer create(const RPCInvoker & invoker, IOService & ioService,
                const std::string & hostname, unsigned short port);

    private:
        OutgoingConnection(const RPCInvoker & invoker, IOService & ioService)
            : RealConnection{Outgoing, invoker, ioService} {}

        void connect(const std::string & hostname, unsigned short port);
};
