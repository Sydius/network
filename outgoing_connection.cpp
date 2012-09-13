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
#include "outgoing_connection.h"

namespace SydNet {

/******************
 * Factory methods
 ******************/

Connection::Pointer OutgoingConnection::create(const RPCInvoker & invoker, IOService & ioService,
        const std::string & hostname, unsigned short port)
{
    OutgoingConnection * real{new OutgoingConnection{invoker, ioService}};
    Connection::Pointer ptr{real};
    real->connect(hostname, port);
    return ptr;
}


/******************
* Private methods
******************/

void OutgoingConnection::connect(const std::string & hostname, unsigned short port)
{
    using boost::asio::ip::tcp;

    tcp::resolver resolver{socket().io_service()};
    tcp::resolver::query query{hostname, "0"}; // The port is set later, directly
    tcp::resolver::iterator end;
    tcp::endpoint endPoint;

    boost::system::error_code error{boost::asio::error::host_not_found};
    for (auto endpointsIter = resolver.resolve(query); error && endpointsIter != end; endpointsIter++) {
        socket().close();
        endPoint = *endpointsIter;
        endPoint.port(port);
        LOG_INFO("Connection attempt: ", endPoint);
        socket().connect(endPoint, error);
    }

    if (error) {
        lastErrorCode(error);
        disconnect();
        return;
    }

    LOG_NOTICE("Connected: ", endPoint);

    read();
}

}
