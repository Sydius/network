#include "fake_connection.h"

/******************
 * Factory methods
 ******************/

Connection::Pointer FakeConnection::create(Connection::IOService & ioService, const RPCInvoker & invoker)
{
    return Pointer{new FakeConnection{ioService, invoker}};
}


/*****************
 * Public methods
 *****************/

Connection::ConnectionMap & FakeConnection::peers()
{
    if (_peers.empty()) {
        _peers[uuid()] = shared_from_this();
    }
    return _peers;
}
