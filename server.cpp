#include <iostream>
#include <string>
#include <boost/asio.hpp>
#include "shared.h"
#include "invoke.h"
#include "connection.h"

class Server
{
    public:
        Server(boost::asio::io_service & ioService, invoke::Invoker & invoker)
            : _acceptor(ioService, boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(), 2000))
            , _invoker(invoker)
        {
            startAccept();
        }

    private:
        void startAccept()
        {
            Connection::pointer newConnection = Connection::create(_acceptor.io_service(), _invoker);
            _acceptor.async_accept(newConnection->socket(),
                std::bind(&Server::handleAccept, this, newConnection, std::placeholders::_1));
        }

        void handleAccept(Connection::pointer newConnection, const boost::system::error_code & error)
        {
            if (!error) {
                newConnection->beginReading();
                startAccept();
            }
        }

        boost::asio::ip::tcp::acceptor _acceptor;
        invoke::Invoker _invoker;
};

int main(int argc, char * argv[])
{
    try {
        invoke::Invoker invoker;
        invoker.registerFunction("foo", foo);
        
        boost::asio::io_service ioService;

        Server server(ioService, invoker);
        ioService.run();
    } catch (std::exception & e) {
        std::cerr << e.what() << std::endl;
    }
    return 0;
}
