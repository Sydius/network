#include "incoming_connection.h"

namespace SydNet {

/******************
 * Factory methods
 ******************/

Connection::Pointer IncomingConnection::create(const RPCInvoker & invoker, IOService & ioService,
        const boost::uuids::uuid & uuid, ConnectionMap * peers)
{
    return Pointer{new IncomingConnection{invoker, ioService, uuid, peers}};
}


/*****************
 * Public methods
 *****************/

void IncomingConnection::beginReading(const DisconnectHandler & disconnectHandler)
{
    _disconnectHandler = disconnectHandler;
    read();
}

void IncomingConnection::disconnect()
{
    if (_disconnectHandler) {
        LOG_DEBUG("Disconnect handler being called");
        _disconnectHandler(lastErrorCode());
        _disconnectHandler = DisconnectHandler{};
    }
    RealConnection::disconnect();
}

}
