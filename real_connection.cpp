#include "real_connection.h"

/******************
 * Factory methods
 ******************/

Connection::Pointer RealConnection::incoming(const RPCInvoker & invoker, IOService & ioService,
        const boost::uuids::uuid & uuid, ConnectionMap * peers)
{
    return Pointer{new RealConnection{Incoming, invoker, ioService, uuid, peers}};
}


/*****************
 * Public methods
 *****************/

void RealConnection::beginReading(const DisconnectHandler & disconnectHandler)
{
    _disconnectHandler = disconnectHandler;
    read();
}

void RealConnection::disconnect()
{
    _connected = false;
    if (!_lastErrorCode) {
        LOG_DEBUG("Shutting down socket");
        _socket.shutdown(boost::asio::ip::tcp::socket::shutdown_both, _lastErrorCode);
    }

    if (!_lastErrorCode) {
        LOG_DEBUG("Closing socket");
        _socket.close(_lastErrorCode);
    }

    if (_disconnectHandler) {
        LOG_DEBUG("Disconnect handler being called");
        _disconnectHandler(_lastErrorCode);
        _disconnectHandler = DisconnectHandler{};
    }

    if (_lastErrorCode && _lastErrorCode != boost::asio::error::eof) {
        throw boost::system::system_error(_lastErrorCode);
    }

    LOG_DEBUG("Connection disconnected");
}

Connection::ConnectionMap & RealConnection::peers()
{
    if (!_peers) {
        throw std::logic_error("An attempt to walk connections when there are none was made");
    }
    return *_peers;
}


/********************
 * Protected methods
 ********************/

RealConnection::RealConnection(Type type,
                               const RPCInvoker & invoker,
                               IOService & ioService,
                               const boost::uuids::uuid & uuid,
                               ConnectionMap * peers)
    : Connection{type, invoker, uuid}
    , _incoming{}
    , _outgoing{}
    , _writing{false}
    , _socket{ioService}
    , _connected{false}
    , _disconnectHandler{}
    , _lastErrorCode{}
    , _peers{peers}
{
}

void RealConnection::read()
{
    _connected = true;
    boost::asio::async_read_until(_socket, _incoming, PACKET_END,
        std::bind(&RealConnection::handleRead, getDerivedPointer(),
            std::placeholders::_1,
            std::placeholders::_2));
}


/******************
* Private methods
******************/

void RealConnection::write()
{
    boost::asio::async_write(_socket, _outgoing,
        std::bind(&RealConnection::handleWrite, getDerivedPointer(),
            std::placeholders::_1,
            std::placeholders::_2));
}

void RealConnection::handleRead(const boost::system::error_code & error, size_t size)
{
    if (error) {
        _lastErrorCode = error;
        disconnect();
        return;
    }

    std::istream is(&_incoming);
    std::string name, command;
    std::getline(is, name, PACKET_END);
    std::getline(is, command, PACKET_END);

    LOG_DEBUG("Local RPC executed: ", name);

    invoker().invoke(name, command, shared_from_this());

    if (_connected) {
        read();
    }
}

void RealConnection::handleWrite(const boost::system::error_code & error, size_t)
{
    if (error) {
        _lastErrorCode = error;
        disconnect();
        return;
    }
    
    if (_outgoing.size()) {
        write();
    } else {
        _writing = false;
    }
}
