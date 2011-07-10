#include <iostream>
#include <string>
#include "shared.h"
#include "connection.h"

class Server
{
    public:
        Server(Connection::IOService & ioService, const Connection::RPCInvoker & invoker)
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
        Connection::RPCInvoker _invoker;
};

int main(int argc, char * argv[])
{
    try {
        Connection::IOService ioService;
        Server server(ioService, RPCMethods());
        ioService.run();
    } catch (std::exception & e) {
        std::cerr << e.what() << std::endl;
    }
    return 0;
}
