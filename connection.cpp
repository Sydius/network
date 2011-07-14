#include "connection.h"

Connection::pointer Connection::create(Connection::IOService & ioService, const RPCInvoker & invoker,
        const boost::uuids::uuid & uuid, ConnectionMap * connections)
{
    return pointer(new Connection(ioService, invoker, uuid, connections));
}

Connection::pointer Connection::connect(Connection::IOService & ioService, const RPCInvoker & invoker,
        const std::string & hostname, unsigned short port)
{
    pointer ptr = create(ioService, invoker);
    ptr->connect(hostname, port);
    return ptr;
}

void Connection::beginReading(const DisconnectHandler & disconnectHandler)
{
    _disconnectHandler = disconnectHandler;
    _shouldCallDisconnectHandler = true;
    read();
}

void Connection::disconnect()
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

    if (_shouldCallDisconnectHandler) {
        LOG_DEBUG("Disconnect handler being called");
        _disconnectHandler(_lastErrorCode);
    }

    if (_lastErrorCode && _lastErrorCode != boost::asio::error::eof) {
        throw boost::system::system_error(_lastErrorCode);
    }

    LOG_DEBUG("Connection disconnected");
}

Connection::ConnectionMap & Connection::peers()
{
    if (!_connections) {
        throw std::logic_error("An attempt to walk connections when there are none was made");
    }
    return *_connections;
}

/******************
* Private methods
******************/

Connection::Connection(Connection::IOService & ioService, const RPCInvoker & invoker, const boost::uuids::uuid & uuid, ConnectionMap * connections)
    : _socket(ioService)
    , _invoker(invoker)
    , _connected(false)
    , _shouldCallDisconnectHandler(false)
    , _uuid(uuid)
    , _connections(connections)
{
    LOG_DEBUG("Connection created");
}

void Connection::connect(const std::string & hostname, unsigned short port)
{
    using boost::asio::ip::tcp;

    tcp::resolver resolver(_socket.io_service());
    tcp::resolver::query query(hostname, "0"); // The port is set later, directly
    tcp::resolver::iterator end;
    tcp::endpoint endPoint;

    boost::system::error_code error = boost::asio::error::host_not_found;
    for (auto endpointsIter = resolver.resolve(query); error && endpointsIter != end; endpointsIter++) {
        _socket.close();
        endPoint = *endpointsIter;
        endPoint.port(port);
        LOG_INFO("Connection attempt: ", endPoint);
        _socket.connect(endPoint, error);
    }

    if (error) {
        _lastErrorCode = error;
        disconnect();
        return;
    }

    LOG_NOTICE("Connected: ", endPoint);

    read();
}

void Connection::read()
{
    _connected = true;
    boost::asio::async_read_until(_socket, _incoming, PACKET_END,
        std::bind(&Connection::handleRead, shared_from_this(),
            std::placeholders::_1,
            std::placeholders::_2));
}

void Connection::handleRead(const boost::system::error_code & error, size_t size)
{
    if (error) {
        _lastErrorCode = error;
        disconnect();
        return;
    }

    std::istream is(&_incoming);
    std::string line;
    std::getline(is, line, PACKET_END);

    LOG_DEBUG("Local RPC executed: ", _invoker.extractName(line));

    _invoker.invoke(line, shared_from_this());

    if (_connected) {
        read();
    }
}

void Connection::handleWrite(const boost::system::error_code & error, size_t)
{
    if (error) {
        _lastErrorCode = error;
        disconnect();
        return;
    }
}
