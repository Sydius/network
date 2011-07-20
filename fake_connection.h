#pragma once

#include "connection.h"

class FakeConnection: public Connection
{
    public:
        /**
         * Create a fake connection for single-player use
         *
         * @param ioService IOService to use (not used if never connected)
         * @param invoker   RPC invoker to use with this connection
         * @return          A shared Pointer to a new connection object
         */
        static Pointer create(IOService & ioService, const RPCInvoker & invoker);

        ConnectionMap & peers()
        {
            if (_peers.empty()) {
                _peers[uuid()] = shared_from_this();
            }
            return _peers;
        }

    private:
        FakeConnection(IOService & ioService,
                       const RPCInvoker & invoker)
            : Connection{ioService, invoker}
            , _peers{}
        {
        }

        ConnectionMap _peers;
};
