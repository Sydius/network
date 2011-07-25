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

void RealConnection::read(size_t size)
{
    _connected = true;

    if (size >= sizeof(uint16_t)) {
        handleReadCommandHeader(boost::system::error_code{}, size);
    } else {
        boost::asio::async_read(_socket, _incoming, boost::asio::transfer_at_least(sizeof(uint16_t) - size),
            std::bind(&RealConnection::handleReadCommandHeader, getDerivedPointer(),
                std::placeholders::_1,
                std::placeholders::_2));
    }
}


/******************
* Private methods
******************/

void RealConnection::remoteExecute(const std::string & name, const std::string & params)
{
    std::ostream outgoingStream{&_outgoing};
    uint16_t size{name.length() + params.length() + sizeof(PACKET_END)};
    outgoingStream.write(reinterpret_cast<char *>(&size), sizeof(size));
    outgoingStream << name << PACKET_END;
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

void RealConnection::handleReadCommandHeader(const boost::system::error_code & error, size_t size)
{
    if (error) {
        _lastErrorCode = error;
        disconnect();
        return;
    }

    std::istream inputStream(&_incoming);

    uint16_t commandSize;
    inputStream.read(reinterpret_cast<char *>(&commandSize), sizeof(uint16_t));
    size -= sizeof(uint16_t);

    if (size >= commandSize) {
        handleReadCommand(error, size, commandSize);
    } else {
        boost::asio::async_read(_socket, _incoming, boost::asio::transfer_at_least(commandSize - size),
            std::bind(&RealConnection::handleReadCommand, getDerivedPointer(),
                std::placeholders::_1,
                std::placeholders::_2,
                commandSize));
    }
}

void RealConnection::handleReadCommand(const boost::system::error_code & error, size_t size, uint16_t commandSize)
{
    if (error) {
        _lastErrorCode = error;
        disconnect();
        return;
    }

    std::istream inputStream(&_incoming);

    std::string name;
    std::getline(inputStream, name, PACKET_END);

    //LOG_DEBUG("Local RPC executed: ", name);

    std::stringstream result;
    invoker().invoke(name, inputStream, result, shared_from_this());

    if (_connected) {
        size -= commandSize;
        read(size);
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
