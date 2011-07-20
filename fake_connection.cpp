#include "fake_connection.h"

/******************
 * Factory methods
 ******************/

Connection::Pointer FakeConnection::create(Connection::IOService & ioService, const RPCInvoker & invoker)
{
    return Pointer{new FakeConnection{ioService, invoker}};
}
