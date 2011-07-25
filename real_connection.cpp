#include "real_connection.h"

namespace SydNet {

/*****************
 * Public methods
 *****************/

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

void RealConnection::remoteExecute(const std::string & name, const std::string & params)
{
    std::ostream outgoingStream{&_outgoing};
    outgoingStream << name << PACKET_END;
    uint32_t size{params.length()};
    outgoingStream.write(reinterpret_cast<char *>(&size), sizeof(size));
    outgoingStream << params;

    if (!_writing) {
        _writing = true;
        write();
    }
}

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

    std::istream inputStream(&_incoming);
    std::string name;
    std::getline(inputStream, name, PACKET_END);
    inputStream.ignore(sizeof(uint32_t));

    LOG_DEBUG("Local RPC executed: ", name);

    std::stringstream result;
    invoker().invoke(name, inputStream, result, shared_from_this());

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

}
