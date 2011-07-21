#include "outgoing_connection.h"

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
