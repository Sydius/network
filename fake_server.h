#pragma once

#include "server.h"
#include "fake_connection.h"

class FakeServer: public Server
{
    public:
        FakeServer(const Connection::RPCInvoker & invoker)
            : Server{invoker}
            , _connection{FakeConnection::create(invoker)}
            , _connectionMap{}
        {
            _connectionMap[_connection->uuid()] = _connection;
        }

        Connection::ConnectionMap & clients()
        {
            return _connectionMap;
        }

    private:
        Connection::Pointer _connection;
        Connection::ConnectionMap _connectionMap;
};
