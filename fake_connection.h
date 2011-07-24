#pragma once

#include "connection.h"

namespace SydNet {

class FakeConnection: public Connection
{
    public:
        /**
         * Create a fake connection for single-player use
         *
         * @param invoker   RPC invoker to use with this connection
         * @return          A shared Pointer to a new connection object
         */
        static Pointer create(const RPCInvoker & invoker);

        ConnectionMap & peers();

    private:
        FakeConnection(const RPCInvoker & invoker)
            : Connection{Fake, invoker}
            , _peers{}
        {
        }

        ConnectionMap _peers;
};

}
