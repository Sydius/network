#pragma once

#include "real_connection.h"

namespace SydNet {

class IncomingConnection: public RealConnection
{
    public:
        typedef std::function<void (boost::system::error_code)> DisconnectHandler;

        /**
         * Create a new connection object
         *
         * @param invoker   RPC invoker to use with this connection
         * @param ioService IOService to use (not used if never connected)
         * @param uuid      UUID for the connection
         * @param peers     A map of peer connections
         * @return          A shared Pointer to a new connection object
         */
        static Pointer create(const RPCInvoker & invoker, IOService & ioService,
                const boost::uuids::uuid & uuid, ConnectionMap * peers);

        /**
         * Begin reading on this connection
         *
         * @param disconnectHandler Function to call when this connection disconnects
         */
        virtual void beginReading(const DisconnectHandler & disconnectHandler);

        virtual void disconnect();

    private:
        IncomingConnection(const RPCInvoker & invoker, IOService & ioService,
                const boost::uuids::uuid & uuid, ConnectionMap * peers)
            : RealConnection{Incoming, invoker, ioService, uuid, peers}
            , _disconnectHandler{} {}

        DisconnectHandler _disconnectHandler;
};

}
