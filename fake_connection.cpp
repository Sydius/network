#include "fake_connection.h"

/******************
 * Factory methods
 ******************/

Connection::Pointer FakeConnection::create(const RPCInvoker & invoker)
{
    return Pointer{new FakeConnection{invoker}};
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
