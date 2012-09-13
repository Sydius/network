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
#pragma once

#include <deque>
#include <boost/asio.hpp>

#include "io_service.h"
#include "connection.h"

namespace SydNet {

class RealConnection: public Connection
{
    public:
        /**
         * Get the socket used by this connection.
         *
         * @return  Socket used by this connection
         */
        virtual boost::asio::ip::tcp::socket & socket()
        {
            return _socket;
        }

        virtual void disconnect();

        virtual ConnectionMap & peers();

        virtual ~RealConnection() {}

        RealConnection & operator=(const RealConnection &) = delete;
        RealConnection(const RealConnection &) = delete;

    protected:
        RealConnection(Type type,
                       const RPCInvoker & invoker,
                       IOService & ioService,
                       const boost::uuids::uuid & uuid = boost::uuids::nil_uuid(),
                       ConnectionMap * peers = NULL);

        void read(size_t size=0);

        boost::system::error_code lastErrorCode() const
        {
            return _lastErrorCode;
        }

        void lastErrorCode(boost::system::error_code error)
        {
            _lastErrorCode = error;
        }

    private:
        typedef uint16_t CommandSize;

        void remoteExecute(const std::string & name, const std::string & params, RequestID=0);
        void remoteExecute(const std::string & name, const std::string & params, RemoteExecuteCallback callback);

        std::shared_ptr<RealConnection> getDerivedPointer()
        {
            return std::static_pointer_cast<RealConnection>(shared_from_this());
        }

        void write();

        void handleReadCommandHeader(const boost::system::error_code & error, size_t size);

        void handleReadCommand(const boost::system::error_code & error, size_t size, CommandSize commandSize);

        void handleWrite(const boost::system::error_code & error, size_t);

        boost::asio::streambuf _incoming; // For incoming data; must stay valid while reading
        boost::asio::streambuf _outgoing; // For outgoing data; must stay valid while writing
        bool _writing; // True if it's already sending data
        boost::asio::ip::tcp::socket _socket;
        bool _connected;
        boost::system::error_code _lastErrorCode;
        ConnectionMap * _peers; // Peer connections

        typedef std::pair<RequestID, RemoteExecuteCallback> RequestCallbackPair;
        typedef std::deque<RequestCallbackPair> RequestCallbacks;
        RequestCallbacks _requestCallbacks;
        RequestID _nextRequestID;

        static const char PACKET_END = '\0';
};

}
