#include <iostream>
#include <string>
#include <boost/asio.hpp>
#include "shared.h"
#include "invoke.h"
#include "connection.h"

int main(int argc, char * argv[])
{
    invoke::Invoker invoker;
    invoker.registerFunction("foo", foo);
    std::string message = invoke::serialize("foo", foo, 5) + '\n';

    boost::asio::io_service ioService;
    boost::asio::ip::tcp::resolver resolver(ioService);
    boost::asio::ip::tcp::resolver::query query("localhost", "2000");
    boost::asio::ip::tcp::resolver::iterator endpointsIter = resolver.resolve(query);
    boost::asio::ip::tcp::resolver::iterator end;

    boost::asio::ip::tcp::socket socket(ioService);
    boost::system::error_code error = boost::asio::error::host_not_found;
    while (error && endpointsIter != end) {
        socket.close();
        socket.connect(*endpointsIter++, error);
    }
    if (error)
        return 1;

    while(true) {
        boost::asio::write(socket, boost::asio::buffer(message));
    }

    return 0;
}
