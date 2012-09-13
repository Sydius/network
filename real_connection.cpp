/*
Copyright 2011 Christopher Allen Ogden. All rights reserved.

Redistribution and use in source and binary forms, with or without modification, are
permitted provided that the following conditions are met:

   1. Redistributions of source code must retain the above copyright notice, this list of
      conditions and the following disclaimer.

   2. Redistributions in binary form must reproduce the above copyright notice, this list
      of conditions and the following disclaimer in the documentation and/or other materials
      provided with the distribution.

THIS SOFTWARE IS PROVIDED BY CHRISTOPHER ALLEN OGDEN ``AS IS'' AND ANY EXPRESS OR IMPLIED
WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND
FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL CHRISTOPHER ALLEN OGDEN OR
CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

The views and conclusions contained in the software and documentation are those of the
authors and should not be interpreted as representing official policies, either expressed
or implied, of Christopher Allen Ogden.
*/
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
    , _requestCallbacks{}
    , _nextRequestID{1}
{
}

void RealConnection::read(size_t size)
{
    _connected = true;

    if (size >= sizeof(CommandSize)) {
        handleReadCommandHeader(boost::system::error_code{}, size);
    } else {
        boost::asio::async_read(_socket, _incoming, boost::asio::transfer_at_least(sizeof(CommandSize) - size),
            std::bind(&RealConnection::handleReadCommandHeader, getDerivedPointer(),
                std::placeholders::_1,
                std::placeholders::_2));
    }
}


/******************
* Private methods
******************/

void RealConnection::remoteExecute(const std::string & name, const std::string & params, RequestID requestID)
{
    std::ostream outgoingStream{&_outgoing};
    CommandSize size{name.length() + params.length() + sizeof(PACKET_END) + sizeof(RequestID)};
    outgoingStream.write(reinterpret_cast<char *>(&size), sizeof(size));
    outgoingStream.write(reinterpret_cast<char *>(&requestID), sizeof(requestID));
    outgoingStream << name << PACKET_END;
    outgoingStream << params;

    write();
}

void RealConnection::remoteExecute(const std::string & name, const std::string & params, RemoteExecuteCallback callback)
{
    RequestID requestID = _nextRequestID++;
    
    _requestCallbacks.push_back(RequestCallbackPair{requestID, callback});
    remoteExecute(name, params, requestID);

    if (!_nextRequestID || _nextRequestID & REQUEST_ID_RECEIVED_BIT) {
        _nextRequestID = 1;
    }
}

void RealConnection::write()
{
    if (!_writing) {
        _writing = true;

        boost::asio::async_write(_socket, _outgoing,
            std::bind(&RealConnection::handleWrite, getDerivedPointer(),
                std::placeholders::_1,
                std::placeholders::_2));
    }
}

void RealConnection::handleReadCommandHeader(const boost::system::error_code & error, size_t size)
{
    if (error) {
        _lastErrorCode = error;
        disconnect();
        return;
    }

    std::istream inputStream(&_incoming);

    CommandSize commandSize;
    inputStream.read(reinterpret_cast<char *>(&commandSize), sizeof(commandSize));
    size -= sizeof(commandSize);

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

void RealConnection::handleReadCommand(const boost::system::error_code & error, size_t size, CommandSize commandSize)
{
    if (error) {
        _lastErrorCode = error;
        disconnect();
        return;
    }

    std::istream inputStream(&_incoming);

    RequestID requestID;
    inputStream.read(reinterpret_cast<char *>(&requestID), sizeof(requestID));

    if (requestID & REQUEST_ID_RECEIVED_BIT) {
        // Process result
        requestID &= ~REQUEST_ID_RECEIVED_BIT;
        auto iter = _requestCallbacks.begin();
        while (iter != _requestCallbacks.end()) {
            if (iter->first == requestID) {
                iter->second(inputStream);
                _requestCallbacks.erase(iter);
                break;
            }
        }
    } else {
        std::string name;
        std::getline(inputStream, name, PACKET_END);

        std::stringstream result;
        if (invoker().invoke(name, inputStream, result, shared_from_this()) && requestID) {
            // Send result
            std::ostream outgoingStream{&_outgoing};
            CommandSize outgoingSize{sizeof(RequestID) + result.str().length()};
            outgoingStream.write(reinterpret_cast<char *>(&outgoingSize), sizeof(outgoingSize));
            requestID |= REQUEST_ID_RECEIVED_BIT;
            outgoingStream.write(reinterpret_cast<char *>(&requestID), sizeof(requestID));
            outgoingStream << result.str();
            write();
        }
    }

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
    
    _writing = false;
    if (_outgoing.size()) {
        write();
    }
}

}
